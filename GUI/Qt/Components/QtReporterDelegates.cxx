#include "QtReporterDelegates.h"
#include "UIReporterDelegates.h"
#include "SNAPQtCommon.h"
#include <QResizeEvent>
#include <QProgressDialog>
#include <QCoreApplication>

#include <QPainter>
#include <QPixmap>
#include <QFile>
#include <QImage>
#include <QWindow>
#include "itkImage.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkRGBAPixel.h"
#include "Registry.h"
#include <sstream>

#if QT_VERSION >= 0x050000
#  include <QStandardPaths>
#endif

#include <QUrl>


#include "SNAPQtCommon.h"


QtViewportReporter::QtViewportReporter()
{
  m_ClientWidget = NULL;
  m_Filter = new EventFilter();
  m_Filter->m_Owner = this;
}

QtViewportReporter::~QtViewportReporter()
{
  if (m_ClientWidget)
    m_ClientWidget->removeEventFilter(m_Filter);

  delete m_Filter;
}

void
QtViewportReporter::SetClientWidget(QWidget *widget)
{
  // In case we are changing widgets, make sure the filter is cleared
  if (m_ClientWidget)
    m_ClientWidget->removeEventFilter(m_Filter);

  // Store the widget
  m_ClientWidget = widget;

  // Capture events from the widget
  m_ClientWidget->installEventFilter(m_Filter);
}

bool
QtViewportReporter::CanReportSize()
{
  return m_ClientWidget != NULL;
}

#if QT_VERSION >= 0x050000

Vector2ui
QtViewportReporter::GetViewportSize()
{
  // For retina displays, this method reports size in actual pixels, not abstract pixels
  return Vector2ui(m_ClientWidget->width() * m_ClientWidget->devicePixelRatio(),
                   m_ClientWidget->height() * m_ClientWidget->devicePixelRatio());
}

float
QtViewportReporter::GetViewportPixelRatio()
{
  return m_ClientWidget->devicePixelRatio();
}

std::string
QtSystemInfoDelegate::GetApplicationPermanentDataLocation()
{
  return to_utf8(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
}

std::string
QtSystemInfoDelegate::GetUserDocumentsLocation()
{
  return to_utf8(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
}

std::string
QtSystemInfoDelegate::EncodeServerURL(const std::string &url_string)
{
  QUrl url(from_utf8(url_string));
  QByteArray ba = url.toEncoded(QUrl::RemoveScheme | QUrl::RemoveUserInfo | QUrl::StripTrailingSlash |
                                QUrl::NormalizePathSegments | QUrl::FullyEncoded);
  return std::string(ba.constData());
}


#else

Vector2ui
QtViewportReporter::GetViewportSize()
{
  // For retina displays, this method reports size in actual pixels, not abstract pixels
  return Vector2ui(m_ClientWidget->width(), m_ClientWidget->height());
}

float
QtViewportReporter::GetViewportPixelRatio()
{
  return 1.0f;
}

#  include <QDesktopServices>
std::string
QtSystemInfoDelegate::GetApplicationPermanentDataLocation()
{
  return to_utf8(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
}

std::string
QtSystemInfoDelegate::GetUserDocumentsLocation()
{
  return to_utf8(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));
}

std::string
QtSystemInfoDelegate::EncodeServerURL(const std::string &url_string)
{
  QUrl url(from_utf8(url_string));
  QByteArray ba = url.toEncoded(QUrl::RemoveScheme | QUrl::RemoveUserInfo | QUrl::StripTrailingSlash);
  return std::string(ba.constData());
}

#endif

Vector2ui
QtViewportReporter::GetLogicalViewportSize()
{
  return Vector2ui(m_ClientWidget->width(), m_ClientWidget->height());
}

bool
QtViewportReporter::EventFilter ::eventFilter(QObject *object, QEvent *event)
{
  if (object == m_Owner->m_ClientWidget && event->type() == QEvent::Resize)
  {
    m_Owner->InvokeEvent(ViewportResizeEvent());
  }
  else if (object == m_Owner->m_ClientWidget && event->type() == QEvent::Show)
  {
    // The viewport repoter also needs to let the model know when the active screen changes
    if(!m_ScreenChangedConnection)
    {
      // Find an upstream window that has a window handle
      QWindow *window = FindUpstreamWindowHandle(m_Owner->m_ClientWidget);
      if (window)
      {
        m_ScreenChangedConnection = connect(window, &QWindow::screenChanged, this, [this, window](QScreen *) {
          m_Owner->InvokeEvent(ViewportResizeEvent());
          connectCurrentScreen();
        });
      }
    }

    // Connect the current screen
    connectCurrentScreen();
  }
  return QObject::eventFilter(object, event);
}

void
QtViewportReporter::EventFilter::connectCurrentScreen()
{
  // Remove existing connections
  for(auto &c : m_ScreenConnections)
    disconnect(c);
  m_ScreenConnections.clear();

  // Connect to changes on the current screen
  QScreen *screen = m_Owner->m_ClientWidget->screen();
  if(screen)
  {
    m_ScreenConnections.push_back(connect(screen, &QScreen::geometryChanged, this, [this](const QRect &) {
      m_Owner->InvokeEvent(ViewportResizeEvent());
    }));
  }
}

QtProgressReporterDelegate::QtProgressReporterDelegate() { m_Dialog = NULL; }

void
QtProgressReporterDelegate::Show(const char *title)
{
  if (m_Dialog)
  {
    m_Dialog->setMinimumDuration(0);
    if (title)
      m_Dialog->setLabelText(QString::fromUtf8(title));
    m_Dialog->show();
    m_Dialog->activateWindow();
    m_Dialog->raise();
    QCoreApplication::processEvents();
  }
}

void
QtProgressReporterDelegate::Hide()
{
  if (m_Dialog)
  {
    m_Dialog->hide();
    QCoreApplication::processEvents();
  }
}

void
QtProgressReporterDelegate::SetProgressDialog(QProgressDialog *dialog)
{
  m_Dialog = dialog;
  m_Dialog->setMinimum(0);
  m_Dialog->setMaximum(1000);
  m_Dialog->setWindowModality(Qt::WindowModal);
  m_Dialog->setLabelText("ITK-SNAP progress");
}

#include <QDebug>
#include <QAction>
void
QtProgressReporterDelegate::SetProgressValue(double value)
{
  m_Dialog->setValue((int)(1000 * value));
  // qDebug() << "Progress: " << value;
  // QCoreApplication::processEvents();
}


std::string
QtSystemInfoDelegate::GetApplicationDirectory()
{
  return to_utf8(QCoreApplication::applicationDirPath());
}

std::string
QtSystemInfoDelegate::GetApplicationFile()
{
  return to_utf8(QCoreApplication::applicationFilePath());
}

void
QtSystemInfoDelegate ::LoadResourceAsImage2D(std::string tag, GrayscaleImage *image)
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
  for (itk::ImageRegionIteratorWithIndex<GrayscaleImage> it(image, region); !it.IsAtEnd(); ++it)
  {
    it.Set(qGray(iq.pixel(it.GetIndex()[0], it.GetIndex()[1])));
  }
}

void
QtSystemInfoDelegate::LoadResourceAsRegistry(std::string tag, Registry &reg)
{
  // Read the file into a byte array
  QFile file(QString(":/snapres/snapres/%1").arg(from_utf8(tag)));
  if (file.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    QByteArray ba = file.readAll();

    // Read the registry contents
    std::stringstream ss(ba.data());
    reg.ReadFromStream(ss);
  }
}

void
QtSystemInfoDelegate::WriteRGBAImage2D(std::string file, RGBAImageType *image)
{
  RGBAImageType::SizeType sz = image->GetBufferedRegion().GetSize();
  QImage                  iq(sz[0], sz[1], QImage::Format_ARGB32);
  typedef itk::ImageRegionConstIteratorWithIndex<RGBAImageType> IterType;
  for (IterType it(image, image->GetBufferedRegion()); !it.IsAtEnd(); ++it)
  {
    const RGBAPixelType &px = it.Value();
    iq.setPixel(it.GetIndex()[0], it.GetIndex()[1], qRgba(px[0], px[1], px[2], px[3]));
  }
  iq.save(from_utf8(file));
}

#include <QSharedMemory>
QtSharedMemorySystemInterface::QtSharedMemorySystemInterface()
{
  m_SharedMem = new QSharedMemory();
}

QtSharedMemorySystemInterface::~QtSharedMemorySystemInterface()
{
  if (m_SharedMem->isAttached())
    m_SharedMem->detach();
  delete m_SharedMem;
}

void
QtSharedMemorySystemInterface::SetKey(const std::string &key)
{
  m_SharedMem->setKey(QString::fromUtf8(key));
}

bool
QtSharedMemorySystemInterface::Attach()
{
  return m_SharedMem->attach();
}

bool
QtSharedMemorySystemInterface::Detach()
{
  return m_SharedMem->detach();
}

bool
QtSharedMemorySystemInterface::Create(unsigned int size)
{
  return m_SharedMem->create(size);
}

bool
QtSharedMemorySystemInterface::IsAttached()
{
  return m_SharedMem->isAttached();
}

std::string
QtSharedMemorySystemInterface::GetErrorMessage()
{
  return m_SharedMem->errorString().toStdString();
}

void *
QtSharedMemorySystemInterface::Data()
{
  return m_SharedMem->data();
}

bool
QtSharedMemorySystemInterface::Lock()
{
  return m_SharedMem->lock();
}

bool
QtSharedMemorySystemInterface::Unlock()
{
  return m_SharedMem->unlock();
}

int
QtSharedMemorySystemInterface::GetProcessID()
{
  return (int)QCoreApplication::applicationPid();
}
