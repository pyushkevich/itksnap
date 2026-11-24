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
#include <QPixmapCache>
#include <QFileInfo>
#include "ImageIORemote.h"
#include "FileExistenceCache.h"
#include <QIcon>
#include <QTimer>
#include <QPainter>
#include <QApplication>

HistoryQListModel::HistoryQListModel(QObject *parent) :
  QStandardItemModel(parent)
{
  m_Model = NULL;

  QPixmap pixmap(128,128);
  pixmap.fill(QColor(Qt::lightGray));
  m_DummyIcon = QIcon(pixmap);
}

void HistoryQListItem::setItem(
    GlobalUIModel *model,
    const QString &history_entry)
{
  // Set the name to the short name. For a URL, this is the name of the file
  // it downloads, without any query or fragment
  QString short_name = QFileInfo(history_entry).fileName();
  m_IsRemote = IsRemoteImageURL(to_utf8(history_entry));
  if(m_IsRemote)
    {
    try { short_name = from_utf8(RemoteFilenameFromURL(to_utf8(history_entry))); }
    catch(IRISException &) {}
    }
  this->setText(short_name);

  // Set the filename
  m_Path = history_entry;
  this->setData(history_entry, Qt::UserRole);
  QPixmap dummy(128, 128);
  dummy.fill(Qt::black);
  m_Thumbnail = dummy;

  // Mark the item if its file is known to be missing, and update it when a
  // check in the background finds it missing (or back)
  FileExistenceCache *fec = FileExistenceCache::instance();
  connect(fec, &FileExistenceCache::statusChanged,
          this, &HistoryQListItem::onFileStatusChanged);
  this->updateMissing();

  // At the moment, these are hard-coded
  this->setSizeHint(QSize(188,144));

  // Deal with the icon later
  std::string hist_str = to_utf8(history_entry);
  std::string thumbnail =
      model->GetDriver()->GetSystemInterface()->GetThumbnailAssociatedWithFile(hist_str.c_str());

  m_IconFilename = from_utf8(thumbnail);

  // TODO: for debugging change 0 to a random number
  QTimer::singleShot(0, this, SLOT(onTimer()));
}

void HistoryQListItem::onTimer()
{
  // Construct a string from the filenane and the timestamp
  QString key = QString("%1::%2")
                .arg(m_IconFilename)
                .arg(QFileInfo(m_IconFilename).lastModified().toString());

  QPixmap pixmap;
  if(QPixmapCache::find(key, &pixmap))
    this->setThumbnail(pixmap);
  else
    {
    try
      {
    /*
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
        } */

      QPixmap load_pixmap(m_IconFilename);
      this->setThumbnail(load_pixmap);
      QPixmapCache::insert(key, load_pixmap);
      }
    catch(itk::ExceptionObject &)
      {
      QPixmap dummy(128, 128);
      dummy.fill(Qt::black);
      this->setThumbnail(dummy);
      QPixmapCache::insert(key, dummy);
      }
    }
}

void HistoryQListItem::onFileStatusChanged(QString path)
{
  if(path == m_Path)
    this->updateMissing();
}

void HistoryQListItem::updateMissing()
{
  m_IsMissing = FileExistenceCache::instance()->isMissing(m_Path);
  this->setData(m_IsMissing, MissingFileRole);
  this->setToolTip(m_IsMissing ? tr("%1 (file not found)").arg(m_Path) : m_Path);
  this->setThumbnail(m_Thumbnail);
}

void HistoryQListItem::setThumbnail(const QPixmap &thumbnail)
{
  m_Thumbnail = thumbnail;
  if(!m_IsRemote && !m_IsMissing)
    {
    this->setIcon(QIcon(thumbnail));
    return;
    }

  // Draw the badge in the top left corner: a missing file's thumbnail is
  // also faded. The result is made at the screen's pixel ratio, so that the
  // badge stays sharp on high-DPI screens
  qreal dpr = qApp->devicePixelRatio();
  QSize size = thumbnail.deviceIndependentSize().toSize();
  QPixmap badged(size * dpr);
  badged.setDevicePixelRatio(dpr);
  badged.fill(Qt::transparent);

  QPainter p(&badged);
  p.setRenderHint(QPainter::SmoothPixmapTransform);
  p.setRenderHint(QPainter::Antialiasing);
  if(m_IsMissing)
    p.setOpacity(0.35);
  p.drawPixmap(QRect(QPoint(0, 0), size), thumbnail);
  p.setOpacity(1.0);
  if(m_IsMissing)
    {
    // A red disc with a white cross
    QRectF disc(4, 4, 20, 20);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(200, 40, 40));
    p.drawEllipse(disc);
    p.setPen(QPen(Qt::white, 2.5, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(disc.center() + QPointF(-4, -4), disc.center() + QPointF(4, 4));
    p.drawLine(disc.center() + QPointF(-4, 4), disc.center() + QPointF(4, -4));
    }
  else
    p.drawPixmap(QRect(4, 4, 20, 20), QPixmap(":/root/remote_badge_20.png"));
  p.end();

  this->setIcon(QIcon(badged));
}

void HistoryQListModel::rebuildModel()
{
  HistoryManager::AbstractHistoryModel *hmodel =
      m_Model->GetDriver()->GetHistoryManager()->GetGlobalHistoryModel(m_HistoryName);
  std::vector<std::string> history = hmodel->GetValue();

  this->setColumnCount(1);
  this->setRowCount(history.size());

  // We need to parse the history in reverse order (but why?)
  for(int i = 0; i < history.size(); i++)
    {
    // Create a standard item to hold this
    HistoryQListItem *si = new HistoryQListItem();
    si->setItem(m_Model, from_utf8(history[history.size() - 1 - i]));
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
      m_Model->GetDriver()->GetHistoryManager()->GetGlobalHistoryModel(m_HistoryName);

  LatentITKEventNotifier::connect(
        hmodel, ValueChangedEvent(),
        this, SLOT(onModelUpdate(EventBucket)));

  // Also watch changes to main image dimensions
  LatentITKEventNotifier::connect(
        model->GetDriver(), MainImageDimensionsChangeEvent(),
        this, SLOT(onModelUpdate(EventBucket)));


  // Cache the history
  this->rebuildModel();
}


void HistoryQListModel::onModelUpdate(const EventBucket &)
{
  this->beginResetModel();
  this->rebuildModel();
  this->endResetModel();
}
