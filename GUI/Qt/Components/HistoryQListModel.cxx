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
#include <QIcon>
#include <QTimer>

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
  // Set the name to the short name
  QString short_name = QFileInfo(history_entry).fileName();
  this->setText(short_name);

  // Set the filename
  this->setToolTip(history_entry);
  this->setData(history_entry, Qt::UserRole);
  QPixmap dummy(128, 128);
  dummy.fill(Qt::black);
  this->setIcon(QIcon(dummy));

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

  // Cache the history
  this->rebuildModel();
}


void HistoryQListModel::onModelUpdate(const EventBucket &bucket)
{
  this->beginResetModel();
  this->rebuildModel();
  this->endResetModel();
}
