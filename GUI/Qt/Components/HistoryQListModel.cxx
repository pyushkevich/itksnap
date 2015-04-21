#include "HistoryQListModel.h"
#include "HistoryManager.h"
#include "SystemInterface.h"
#include "IRISApplication.h"
#include "GlobalUIModel.h"
#include <itksys/SystemTools.hxx>
#include <QIcon>
#include "SNAPQtCommon.h"
#include "LatentITKEventNotifier.h"
#include "itkImageFileReader.h"

#include <QWaitCondition>
#include <QThread>
#include <QString>
#include <SNAPQtCommon.h>
#include <QRunnable>
#include <QThreadPool>


QMutex HistoryThumbnailLoader::mutex;

void HistoryThumbnailLoader::run()
{
  QMutexLocker locker(&mutex);
  std::string filename = to_utf8(m_FileName);

  QThread::currentThread()->sleep(1);

  // Load the icon using ITK to avoid this really annoying warning
  // from the PNG library. The only problem is that QIcon caches
  typedef itk::RGBAPixel<unsigned char> PNGPixelType;
  typedef itk::Image<PNGPixelType, 2> PNGSliceType;
  typedef itk::ImageFileReader<PNGSliceType> PNGReaderType;
  SmartPtr<PNGReaderType> reader = PNGReaderType::New();
  reader->SetFileName(filename.c_str());
  std::cout << filename << std::endl;
  reader->Update();

  // Need to load the icon
  SmartPtr<PNGSliceType> slice = reader->GetOutput();
  int w = slice->GetBufferedRegion().GetSize()[0];
  int h = slice->GetBufferedRegion().GetSize()[1];
  QImage image(w, h, QImage::Format_ARGB32);
  PNGPixelType *input = slice->GetBufferPointer();
  QRgb *output = reinterpret_cast<QRgb*>(image.bits());
  for(int i = 0; i < slice->GetPixelContainer()->Size(); i++)
    {
    *output++ = qRgba(input[i].GetRed(), input[i].GetGreen(), input[i].GetBlue(), input[i].GetAlpha());
    }

  emit dataLoaded(m_HistoryItem, image);
}





HistoryQListModel::HistoryQListModel(QObject *parent) :
  QStandardItemModel(parent)
{
  m_Model = NULL;

  QPixmap pixmap(128,128);
  pixmap.fill(QColor(Qt::lightGray));
  m_DummyIcon = QIcon(pixmap);
}

#include <QPixmapCache>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QFileInfo>
#include <QIcon>

void HistoryQListItem::setItem(
    GlobalUIModel *model,
    const QString &history_entry)
{
  // Set the name to the short name
  QString short_name = QFileInfo(history_entry).fileName();
  this->setText(short_name);

  // Set the filename
  this->setToolTip(history_entry);
  this->setData(history_entry, Qt::UserRole);
  QPixmap dummy(128, 128);
  dummy.fill(Qt::gray);
  this->setIcon(QIcon(dummy)) ;

  // Deal with the icon later
  std::string hist_str = to_utf8(history_entry);
  std::string thumbnail =
      model->GetDriver()->GetSystemInterface()->GetThumbnailAssociatedWithFile(hist_str.c_str());

  m_IconFilename = from_utf8(thumbnail);

  QTimer::singleShot(0, this, SLOT(onTimer()));
}

void HistoryQListItem::onTimer()
{
  // Construct a string from the filenane and the timestamp
  QString key = QString("%1::%2")
                .arg(m_IconFilename)
                .arg(QFileInfo(m_IconFilename).lastModified().toString());
  QPixmap *pixmap = QPixmapCache::find(key);
  if(pixmap)
    this->setIcon(QIcon(*pixmap));
  else
    {
    // Load the icon using ITK to avoid this really annoying warning
    // from the PNG library. The only problem is that QIcon caches
    typedef itk::RGBAPixel<unsigned char> PNGPixelType;
    typedef itk::Image<PNGPixelType, 2> PNGSliceType;
    typedef itk::ImageFileReader<PNGSliceType> PNGReaderType;
    SmartPtr<PNGReaderType> reader = PNGReaderType::New();
    reader->SetFileName(to_utf8(m_IconFilename).c_str());
    reader->Update();

    // Need to load the icon
    SmartPtr<PNGSliceType> slice = reader->GetOutput();
    int w = slice->GetBufferedRegion().GetSize()[0];
    int h = slice->GetBufferedRegion().GetSize()[1];
    QImage image(w, h, QImage::Format_ARGB32);
    PNGPixelType *input = slice->GetBufferPointer();
    QRgb *output = reinterpret_cast<QRgb*>(image.bits());
    for(int i = 0; i < slice->GetPixelContainer()->Size(); i++)
      {
      *output++ = qRgba(input[i].GetRed(), input[i].GetGreen(), input[i].GetBlue(), input[i].GetAlpha());
      }

    QPixmap load_pixmap = QPixmap::fromImage(image);
    this->setIcon(QIcon(load_pixmap));
    QPixmapCache::insert(key, load_pixmap);
    }
}

void HistoryQListModel::rebuildModel()
{
  HistoryManager::AbstractHistoryModel *hmodel =
      m_Model->GetDriver()->GetHistoryManager()->GetGlobalHistoryModel(m_HistoryName);
  std::vector<std::string> history = hmodel->GetValue();

  this->setColumnCount(1);
  this->setRowCount(history.size());
  for(int i = 0; i < history.size(); i++)
    {
    // Create a standard item to hold this
    HistoryQListItem *si = new HistoryQListItem();
    si->setItem(m_Model, from_utf8(history[i]));
    this->setItem(i, 0, si);
    }
}

void HistoryQListModel::Initialize(
    GlobalUIModel *model, const std::string &category)
{
  m_Model = model;
  m_HistoryName = category;

  // Get the property models for the local and global histories
  HistoryManager::AbstractHistoryModel *hmodel =
      m_Model->GetDriver()->GetHistoryManager()->GetGlobalHistoryModel(category);

  // Listen for updates from the history model
  LatentITKEventNotifier::connect(hmodel, ValueChangedEvent(),
                                  this, SLOT(onModelUpdate(EventBucket)));

  // Cache the history
  this->rebuildModel();
}


void HistoryQListModel::onModelUpdate(const EventBucket &bucket)
{
  this->beginResetModel();

  // When history changes, we update
  this->rebuildModel();

  this->endResetModel();
}



/*

HistoryQListModel::HistoryQListModel(QObject *parent) :
  QAbstractListModel(parent)
{
  m_Model = NULL;

  QPixmap pixmap(128,128);
  pixmap.fill(QColor(Qt::lightGray));
  m_DummyIcon = QIcon(pixmap);
}

int HistoryQListModel::rowCount(const QModelIndex &parent) const
{
  // Display at most 12 entries in the history
  return std::min((size_t) 12, m_CachedHistory.size());
}

QVariant HistoryQListModel::data(const QModelIndex &index, int role) const
{
  // Get the entry
  std::string item = m_CachedHistory[m_CachedHistory.size() - (1 + index.row())];

  // Display the appropriate item
  if(role == Qt::DisplayRole)
    {
    // Get the shorter filename
    std::string shorty = itksys::SystemTools::GetFilenameName(item.c_str());
    return from_utf8(shorty);
    }
  else if(role == Qt::DecorationRole)
    {
    // Use the cached icon for this file

    HistoryMap::const_iterator it = m_CachedHistoryData.find(item);
    if(it != m_CachedHistoryData.end() && it->second.is_ready)
      {
      return it->second.thumbnail;
      }
    else
      {
      return m_DummyIcon;
      }
    }
  else if(role == Qt::ToolTipRole || role == Qt::UserRole)
    {
    return from_utf8(item.c_str());
    }
  return QVariant();

}

void HistoryQListModel::Initialize(
    GlobalUIModel *model, const std::string &category)
{
  m_Model = model;
  m_HistoryName = category;

  // Get the property models for the local and global histories
  HistoryManager::AbstractHistoryModel *hmodel =
      m_Model->GetDriver()->GetHistoryManager()->GetGlobalHistoryModel(category);

  // Listen for updates from the history model
  LatentITKEventNotifier::connect(hmodel, ValueChangedEvent(),
                                  this, SLOT(onModelUpdate(EventBucket)));

  // Cache the history
  this->updateCache();
}

#include <QFileInfo>

/ *
 *     // Need to get an icon!
    std::string iconfile = m_Model->GetDriver()->GetSystemInterface()
        ->GetThumbnailAssociatedWithFile(item.c_str());

    // Load the icon using ITK to avoid this really annoying warning
    // from the PNG library. The only problem is that QIcon caches
    typedef itk::RGBAPixel<unsigned char> PNGPixelType;
    typedef itk::Image<PNGPixelType, 2> PNGSliceType;
    typedef itk::ImageFileReader<PNGSliceType> PNGReaderType;
    SmartPtr<PNGReaderType> reader = PNGReaderType::New();
    reader->SetFileName(iconfile.c_str());
    reader->Update();

    // Need to load the icon
    SmartPtr<PNGSliceType> slice = reader->GetOutput();
    int w = slice->GetBufferedRegion().GetSize()[0];
    int h = slice->GetBufferedRegion().GetSize()[1];
    QImage image(w, h, QImage::Format_ARGB32);
    PNGPixelType *input = slice->GetBufferPointer();
    QRgb *output = reinterpret_cast<QRgb*>(image.bits());
    for(int i = 0; i < slice->GetPixelContainer()->Size(); i++)
      {
      *output++ = qRgba(input[i].GetRed(), input[i].GetGreen(), input[i].GetBlue(), input[i].GetAlpha());
      }

    return QIcon(QPixmap::fromImage(image)); * /

void HistoryQListModel::updateHistoryEntry(std::string entry, HistoryEntry &entry_data)
{
  // Need to get an icon!
  std::string iconfile = m_Model->GetDriver()->GetSystemInterface()
                         ->GetThumbnailAssociatedWithFile(entry.c_str());

  QFileInfo info(from_utf8(iconfile));

  if(!entry_data.is_ready || entry_data.last_read_time < info.lastModified())
    {
    entry_data.is_ready = false;
    entry_data.last_read_time = info.lastModified();

    HistoryThumbnailLoader *loader = new HistoryThumbnailLoader();
    loader->setFileName(from_utf8(iconfile));
    loader->setHistoryItem(from_utf8(entry));
    connect(loader, SIGNAL(dataLoaded(QString,QImage)), this, SLOT(onIconLoaded(QString,QImage)));

    HistoryThumbnailLoaderRunnable *lrun = new HistoryThumbnailLoaderRunnable(loader);
    QThreadPool::globalInstance()->start(lrun);
    }
}

void HistoryQListModel::updateCache()
{
  // Read the history
  m_CachedHistory =
      m_Model->GetDriver()->GetHistoryManager()->GetGlobalHistory(m_HistoryName);

  // Read the cached history data
  for(int i = 0; i < m_CachedHistory.size(); i++)
    {
    HistoryEntry he = m_CachedHistoryData[m_CachedHistory[i]];
    updateHistoryEntry(m_CachedHistory[i], m_CachedHistoryData[m_CachedHistory[i]]);
    }
}

void HistoryQListModel::onModelUpdate(const EventBucket &bucket)
{
  this->beginResetModel();

  // When history changes, we update
  this->updateCache();

  this->endResetModel();
}

void HistoryQListModel::onIconLoaded(QString hist_item, QImage image)
{
  m_CachedHistoryData[to_utf8(hist_item)].is_ready = true;
  m_CachedHistoryData[to_utf8(hist_item)].thumbnail = QIcon(QPixmap::fromImage(image));

  // TODO: can be more specific!
  emit dataChanged(index(0,0), index(rowCount(), 0));
}
*/

