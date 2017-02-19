#include "QtReporterDelegates.h"
#include "UIReporterDelegates.h"
#include <QResizeEvent>
#include <QProgressDialog>
#include <QCoreApplication>

#include <QPainter>
#include <QPixmap>

#include <QImage>
#include "itkImage.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "Registry.h"
#include <sstream>

#include "SNAPQtCommon.h"


QtViewportReporter::QtViewportReporter()
{
  m_ClientWidget = NULL;
  m_Filter = new EventFilter();
  m_Filter->m_Owner = this;
}

QtViewportReporter::~QtViewportReporter()
{
  if(m_ClientWidget)
    m_ClientWidget->removeEventFilter(m_Filter);

  delete m_Filter;
}

void QtViewportReporter::SetClientWidget(QWidget *widget)
{
  // In case we are changing widgets, make sure the filter is cleared
  if(m_ClientWidget)
    m_ClientWidget->removeEventFilter(m_Filter);

  // Store the widget
  m_ClientWidget = widget;

  // Capture events from the widget
  m_ClientWidget->installEventFilter(m_Filter);
}

bool QtViewportReporter::CanReportSize()
{
  return m_ClientWidget != NULL;
}

#if QT_VERSION >= 0x050000

Vector2ui QtViewportReporter::GetViewportSize()
{
  // For retina displays, this method reports size in actual pixels, not abstract pixels
  return Vector2ui(m_ClientWidget->width() * m_ClientWidget->devicePixelRatio(),
                   m_ClientWidget->height() * m_ClientWidget->devicePixelRatio());
}

float QtViewportReporter::GetViewportPixelRatio()
{
  return m_ClientWidget->devicePixelRatio();
}

#include <QStandardPaths>
std::string QtSystemInfoDelegate::GetApplicationPermanentDataLocation()
{
  return to_utf8(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
}


#else

Vector2ui QtViewportReporter::GetViewportSize()
{
  // For retina displays, this method reports size in actual pixels, not abstract pixels
  return Vector2ui(m_ClientWidget->width(), m_ClientWidget->height());
}

float QtViewportReporter::GetViewportPixelRatio()
{
  return 1.0f;
}

#include <QDesktopServices>
std::string QtSystemInfoDelegate::GetApplicationPermanentDataLocation()
{
  return to_utf8(QDesktopServices::storageLocation(QDesktopServices::DataLocation));

}


#endif

Vector2ui QtViewportReporter::GetLogicalViewportSize()
{
  return Vector2ui(m_ClientWidget->width(), m_ClientWidget->height());
}

bool
QtViewportReporter::EventFilter
::eventFilter(QObject *object, QEvent *event)
{
  if(object == m_Owner->m_ClientWidget && event->type() == QEvent::Resize)
    {
    m_Owner->InvokeEvent(ViewportResizeEvent());
    }
  return QObject::eventFilter(object, event);
}

QtProgressReporterDelegate::QtProgressReporterDelegate()
{
  m_Dialog = NULL;
}

void QtProgressReporterDelegate::SetProgressDialog(QProgressDialog *dialog)
{
  m_Dialog = dialog;
  m_Dialog->setMinimum(0);
  m_Dialog->setMaximum(1000);
  m_Dialog->setWindowModality(Qt::WindowModal);
  m_Dialog->setLabelText("ITK-SNAP progress");
  m_Dialog->reset();
}

#include <QDebug>
#include <QAction>
void QtProgressReporterDelegate::SetProgressValue(double value)
{
  m_Dialog->setValue((int) (1000 * value));
  // qDebug() << "Progress: " << value;
  // QCoreApplication::processEvents();
}


std::string QtSystemInfoDelegate::GetApplicationDirectory()
{
  return to_utf8(QCoreApplication::applicationDirPath());
}

std::string QtSystemInfoDelegate::GetApplicationFile()
{
  return to_utf8(QCoreApplication::applicationFilePath());
}

void QtSystemInfoDelegate
::LoadResourceAsImage2D(std::string tag, GrayscaleImage *image)
{
  // Load the image using Qt
  QImage iq(QString(":/snapres/snapres/%1").arg(from_utf8(tag)));

  // Initialize the itk image
  itk::ImageRegion<2> region;
  region.SetSize(0, iq.width());
  region.SetSize(1, iq.height());
  image->SetRegions(region);
  image->Allocate();

  GrayscaleImage::SpacingType spacing;
  spacing.Fill(1.0);
  image->SetSpacing(spacing);

  GrayscaleImage::DirectionType direction;
  direction.SetIdentity();
  image->SetDirection(direction);

  // Fill the image buffer
  for(itk::ImageRegionIteratorWithIndex<GrayscaleImage> it(image, region);
      !it.IsAtEnd(); ++it)
    {
    it.Set(qGray(iq.pixel(it.GetIndex()[0],it.GetIndex()[1])));
    }
}

void QtSystemInfoDelegate::LoadResourceAsRegistry(std::string tag, Registry &reg)
{
  // Read the file into a byte array
  QFile file(QString(":/snapres/snapres/%1").arg(from_utf8(tag)));
  if(file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
    QByteArray ba = file.readAll();

    // Read the registry contents
    std::stringstream ss(ba.data());
    reg.ReadFromStream(ss);
    }
}
