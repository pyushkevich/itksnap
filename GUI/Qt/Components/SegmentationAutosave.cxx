#include "SegmentationAutosave.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "IRISImageData.h"
#include "LabelImageWrapper.h"
#include "LayerIterator.h"
#include "RLERegionOfInterestImageFilter.h"
#include "SNAPQtCommon.h"
#include "itkImageFileWriter.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>
#include <set>

namespace
{

typedef LabelImageWrapper::ImageType   LabelImage3D;
typedef LabelImageWrapper::Image4DType LabelImage4D;

const char *lock_name = "session.lock";
const char *readme_name = "README.txt";

// Deep copy of a run-length encoded image. This is cheap, and it lets the
// worker thread write the image while the user keeps editing the layer.
template <class TImage>
SmartPtr<TImage>
CopyRLEImage(const TImage *image)
{
  typedef itk::RegionOfInterestImageFilter<TImage, TImage> FilterType;
  typename FilterType::Pointer filter = FilterType::New();
  filter->SetInput(image);
  filter->SetRegionOfInterest(image->GetLargestPossibleRegion());
  filter->Update();
  SmartPtr<TImage> copy = filter->GetOutput();
  copy->DisconnectPipeline();
  return copy;
}

// Write a run-length encoded image, as ImageWrapper does for segmentations
template <class TImage>
void
WriteRLEImage(TImage *image, const std::string &filename)
{
  typedef itk::Image<typename TImage::PixelType, TImage::ImageDimension> FlatType;
  typedef itk::RegionOfInterestImageFilter<TImage, FlatType> ConverterType;
  typename ConverterType::Pointer converter = ConverterType::New();
  converter->SetInput(image);
  converter->SetRegionOfInterest(image->GetLargestPossibleRegion());

  typedef itk::ImageFileWriter<FlatType> WriterType;
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetInput(converter->GetOutput());
  writer->SetFileName(filename);
  writer->SetUseCompression(true);
  writer->Update();
}

// The time stamp of the latest change to any time point of the layer
unsigned long
GetLayerModifiedTime(LabelImageWrapper *layer)
{
  unsigned long mtime = layer->GetImage4D()->GetTimeStamp().GetMTime();
  for (unsigned int tp = 0; tp < layer->GetNumberOfTimePoints(); tp++)
    mtime = std::max(mtime, (unsigned long)layer->GetImageByTimePoint(tp)->GetTimeStamp().GetMTime());
  return mtime;
}

} // namespace

// One layer to write: a private copy of its image (3D or 4D) and the file
struct SegmentationAutosave::Job
{
  unsigned long          id;
  SmartPtr<LabelImage3D> image3d;
  SmartPtr<LabelImage4D> image4d;
  std::string            filename;
};

// Runs on a worker thread. Each file is written under a temporary name and
// then renamed, so a crash during the write keeps the previous autosave.
QList<unsigned long>
SegmentationAutosave::WriteJobs(std::vector<Job> jobs)
{
  QList<unsigned long> failed;
  for (auto &job : jobs)
  {
    QString final_fn = QString::fromStdString(job.filename);
    QString temp_fn = final_fn;
    temp_fn.replace(".nii.gz", ".tmp.nii.gz");
    try
    {
      if (job.image4d)
        WriteRLEImage(job.image4d.GetPointer(), temp_fn.toStdString());
      else
        WriteRLEImage(job.image3d.GetPointer(), temp_fn.toStdString());

      QFile::remove(final_fn);
      if (!QFile::rename(temp_fn, final_fn))
        throw std::runtime_error("rename failed");
    }
    catch (std::exception &exc)
    {
      qWarning() << "Autosave of" << final_fn << "failed:" << exc.what();
      QFile::remove(temp_fn);
      failed.push_back(job.id);
    }
  }
  return failed;
}

SegmentationAutosave::SegmentationAutosave(GlobalUIModel *model, QObject *parent)
  : QObject(parent)
  , m_Model(model)
{
  this->setObjectName("segmentationAutosave");

  m_Timer = new QTimer(this);
  m_Timer->setSingleShot(true);
  connect(m_Timer, &QTimer::timeout, this, &SegmentationAutosave::onTimer);

  m_Watcher = new QFutureWatcher<QList<unsigned long>>(this);
  connect(m_Watcher, &QFutureWatcherBase::finished, this, &SegmentationAutosave::onWriteFinished);
}

SegmentationAutosave::~SegmentationAutosave()
{
  m_Watcher->waitForFinished();
  this->RemoveSessionDir();
}

void
SegmentationAutosave::Start(int interval_sec)
{
  m_Interval = interval_sec;
  if (m_Interval > 0)
    m_Timer->start(m_Interval * 1000);
  else
    m_Timer->stop();
}

QString
SegmentationAutosave::GetRecoveryRoot()
{
  QString custom = qEnvironmentVariable("SNAP_CRASH_RECOVERY_DIR");
  if (!custom.isEmpty())
    return custom;

  QString appdata = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  return appdata.isEmpty() ? QString() : QDir(appdata).filePath("CrashRecovery");
}

void
SegmentationAutosave::setSessionDir(const QString &dir)
{
  // The folder can't move once something was written to it
  if (m_Lock)
    return;
  m_SessionDir = dir;
}

int
SegmentationAutosave::unsavedLayerCount() const
{
  int n = 0;
  IRISApplication *driver = m_Model->GetDriver();
  if (driver->IsMainImageLoaded())
    for (LayerIterator it = driver->GetIRISImageData()->GetLayers(LABEL_ROLE); !it.IsAtEnd(); ++it)
      if (it.GetLayer()->HasUnsavedChanges())
        n++;
  return n;
}

QStringList
SegmentationAutosave::savedFiles() const
{
  QStringList files;
  for (auto &it : m_Saved)
    files << it.second.filename;
  return files;
}

bool
SegmentationAutosave::IsBusy() const
{
  // Don't pause the application in the middle of a paint stroke, a dialog or
  // snake mode. The last two may also be changing the layers.
  return m_Watcher->isRunning() || m_Model->GetDriver()->IsSnakeModeActive() ||
         QApplication::activeModalWidget() || QApplication::mouseButtons() != Qt::NoButton;
}

void
SegmentationAutosave::onTimer()
{
  // When busy, try again soon
  if (IsBusy())
  {
    m_Timer->start(10000);
    return;
  }

  this->autosaveNow();
  m_Timer->start(m_Interval * 1000);
}

void
SegmentationAutosave::autosaveNow()
{
  // Wait for the earlier write to finish
  if (m_Watcher->isRunning())
    return;

  std::vector<Job> jobs;
  this->CollectJobs(jobs);
  if (jobs.size())
  {
    m_PendingWrites = (int)jobs.size();
    m_Watcher->setFuture(QtConcurrent::run(&SegmentationAutosave::WriteJobs, jobs));
  }
  else
  {
    this->WriteReadme();
  }
}

void
SegmentationAutosave::CollectJobs(std::vector<Job> &jobs)
{
  IRISApplication        *driver = m_Model->GetDriver();
  std::set<unsigned long> unsaved_ids;
  QString                 now = QDateTime::currentDateTime().toString(Qt::ISODate);

  if (driver->IsMainImageLoaded())
  {
    for (LayerIterator it = driver->GetIRISImageData()->GetLayers(LABEL_ROLE); !it.IsAtEnd(); ++it)
    {
      LabelImageWrapper *layer = dynamic_cast<LabelImageWrapper *>(it.GetLayer());
      if (!layer || !layer->HasUnsavedChanges())
        continue;

      unsigned long id = layer->GetUniqueId();
      unsaved_ids.insert(id);

      // Skip the layer if it didn't change since the last autosave
      unsigned long mtime = GetLayerModifiedTime(layer);
      auto saved = m_Saved.find(id);
      if (saved != m_Saved.end() && saved->second.mtime == mtime)
        continue;

      if (!this->PrepareSessionDir())
        return;

      SavedLayer &rec = m_Saved[id];
      rec.filename = QDir(m_SessionDir).filePath(QString("segmentation_%1.nii.gz").arg(id));
      rec.mtime = mtime;
      rec.nickname = from_utf8(layer->GetNickname());
      rec.source = from_utf8(layer->GetFileName());
      rec.time = now;

      Job job;
      job.id = id;
      job.filename = to_utf8(rec.filename);
      if (layer->GetNumberOfTimePoints() > 1)
        job.image4d = CopyRLEImage(layer->GetImage4D());
      else
        job.image3d = CopyRLEImage(layer->GetImage());
      jobs.push_back(job);
    }
  }

  // Delete the files of layers that were saved by the user or unloaded
  for (auto it = m_Saved.begin(); it != m_Saved.end();)
  {
    if (unsaved_ids.count(it->first))
    {
      ++it;
      continue;
    }
    QFile::remove(it->second.filename);
    it = m_Saved.erase(it);
  }
}

void
SegmentationAutosave::onWriteFinished()
{
  // This was already handled if SaveForCrash() waited for the write
  if (m_PendingWrites)
    this->ApplyWriteResult(m_Watcher->result());
}

void
SegmentationAutosave::ApplyWriteResult(const QList<unsigned long> &failed)
{
  // Layers that failed to write are tried again at the next autosave
  for (unsigned long id : failed)
    m_Saved.erase(id);

  m_WriteCount += m_PendingWrites - (int)failed.size();
  m_PendingWrites = 0;
  this->WriteReadme();
}

QString
SegmentationAutosave::SaveForCrash()
{
  // Finish the write in progress. Its finished signal can't be delivered
  // now, so apply its result here.
  if (m_Watcher->isRunning() || m_PendingWrites)
  {
    m_Watcher->waitForFinished();
    this->ApplyWriteResult(m_Watcher->result());
  }

  // Write whatever changed since, on this thread
  std::vector<Job> jobs;
  this->CollectJobs(jobs);
  m_PendingWrites = (int)jobs.size();
  this->ApplyWriteResult(WriteJobs(jobs));

  if (!m_Lock)
    return QString();

  if (m_Saved.empty())
  {
    this->RemoveSessionDir();
    return QString();
  }

  // Hand the folder over as a recovery folder. Releasing the lock first also
  // keeps the destructor from deleting it.
  m_Lock->unlock();
  m_Lock.reset();
  m_Saved.clear();
  m_SessionDir = RenameToRecovery(m_SessionDir);
  return m_SessionDir;
}

QString
SegmentationAutosave::RenameToRecovery(const QString &session_dir)
{
  // Session_<time>_<pid> becomes Recovery_<time>, like the folders that older
  // versions of the crash handler wrote. Other names get the current time.
  QFileInfo fi(session_dir);
  QString   time = fi.fileName().startsWith("Session_")
                     ? fi.fileName().mid(8, 15)
                     : QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
  QDir      parent = fi.absoluteDir();
  QString   name = QString("Recovery_%1").arg(time);
  QString   target = name;
  for (int k = 2; parent.exists(target); k++)
    target = QString("%1_%2").arg(name).arg(k);

  return parent.rename(fi.fileName(), target) ? parent.filePath(target) : fi.absoluteFilePath();
}

bool
SegmentationAutosave::PrepareSessionDir()
{
  if (m_Lock)
    return true;

  if (m_SessionDir.isEmpty())
  {
    QString root = GetRecoveryRoot();
    if (root.isEmpty())
      return false;
    m_SessionDir = QDir(root).filePath(
      QString("Session_%1_%2")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"))
        .arg(QCoreApplication::applicationPid()));
  }

  if (!QDir().mkpath(m_SessionDir))
  {
    qWarning() << "Failed to create autosave folder" << m_SessionDir;
    return false;
  }

  // The lock tells other ITK-SNAP sessions that this folder is in use. When
  // this session crashes, the lock goes stale.
  m_Lock = std::make_unique<QLockFile>(QDir(m_SessionDir).filePath(lock_name));
  m_Lock->setStaleLockTime(0);
  if (!m_Lock->tryLock(0))
  {
    qWarning() << "Failed to lock autosave folder" << m_SessionDir;
    m_Lock.reset();
    return false;
  }

  return true;
}

void
SegmentationAutosave::WriteReadme()
{
  if (!m_Lock)
    return;

  QFile file(QDir(m_SessionDir).filePath(readme_name));
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    return;

  IRISApplication *driver = m_Model->GetDriver();
  QTextStream      out(&file);
  out << "ITK-SNAP autosave of segmentations with unsaved changes\n";
  if (driver->IsMainImageLoaded())
    out << "Main image: " << from_utf8(driver->GetIRISImageData()->GetMain()->GetFileName()) << "\n";
  out << "\n";

  for (auto &it : m_Saved)
  {
    const SavedLayer &rec = it.second;
    out << QFileInfo(rec.filename).fileName() << "\n";
    out << "  Layer:       " << rec.nickname << "\n";
    out << "  Loaded from: " << (rec.source.isEmpty() ? QString("(never saved)") : rec.source) << "\n";
    out << "  Autosaved:   " << rec.time << "\n";
  }
}

void
SegmentationAutosave::RemoveSessionDir()
{
  if (!m_Lock)
    return;

  m_Lock->unlock();
  m_Lock.reset();
  QDir(m_SessionDir).removeRecursively();
  m_Saved.clear();
}

void
SegmentationAutosave::ReportRecoveredSessions(QWidget *parent)
{
  QString root = GetRecoveryRoot();
  if (root.isEmpty() || !QDir(root).exists())
    return;

  QStringList recovered;
  QDir        root_dir(root);
  for (const QFileInfo &fi : root_dir.entryInfoList(QStringList() << "Session_*", QDir::Dirs | QDir::NoDotAndDotDot))
  {
    // A lock we can take is stale: its session ended without cleaning up
    QDir      dir(fi.absoluteFilePath());
    QLockFile lock(dir.filePath(lock_name));
    lock.setStaleLockTime(0);
    if (!lock.tryLock(0))
      continue;
    lock.unlock();

    // A session that had nothing unsaved leaves an empty folder
    if (dir.entryList(QStringList() << "*.nii.gz", QDir::Files).isEmpty())
    {
      dir.removeRecursively();
      continue;
    }

    recovered << RenameToRecovery(fi.absoluteFilePath());
  }

  if (recovered.isEmpty())
    return;

  // Point to the folder itself if there is one, else to the root
  QString folder = recovered.size() == 1 ? recovered.front() : root;

  QMessageBox msgBox(parent);
  msgBox.setIcon(QMessageBox::Warning);
  msgBox.setWindowTitle(tr("Recovered Segmentations"));
  msgBox.setText(tr("An earlier ITK-SNAP session did not close normally."));
  msgBox.setInformativeText(
    tr("Segmentations that had unsaved changes were autosaved to folder '%1'. "
       "The README.txt file in that folder lists the image each one belongs to.")
      .arg(QDir::toNativeSeparators(folder)));
  QPushButton *openFolderButton = msgBox.addButton(tr("Open Folder"), QMessageBox::AcceptRole);
  msgBox.addButton(tr("Close"), QMessageBox::RejectRole);
  msgBox.exec();

  if (msgBox.clickedButton() == openFolderButton)
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}
