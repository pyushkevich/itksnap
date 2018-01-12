#include "ImageIOWizard.h"

#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QFileDialog>
#include <QCompleter>
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QFileInfo>
#include <QMenu>
#include <QTreeWidget>
#include <QMessageBox>
#include <QTableWidget>
#include <QApplication>
#include <QHeaderView>
#include <QGridLayout>
#include <QSpinBox>
#include <QFrame>
#include <QTimer>


#include <QtCursorOverride.h>

#include <QtComboBoxCoupling.h>
#include <QtWidgetCoupling.h>
#include "QtVTKRenderWindowBox.h"

#include "IRISException.h"
#include "ImageIOWizardModel.h"
#include "MetaDataAccess.h"
#include "SNAPQtCommon.h"
#include "FileChooserPanelWithHistory.h"

#include "ImageIOWizard/OverlayRolePage.h"

#include "DICOMListingTable.h"


namespace imageiowiz {

const QString AbstractPage::m_HtmlTemplate =
    "<tr><td width=40><img src=\":/root/%1.png\" /></td>"
    "<td><p>%2</p></td></tr>";

AbstractPage::AbstractPage(QWidget *parent)
  : QWizardPage(parent)
{
  this->m_Model = NULL;

  m_OutMessage = new QLabel(this);
  m_OutMessage->setWordWrap(true);
}

bool
AbstractPage::ErrorMessage(const IRISException &exc)
{
  QString text = QString::fromUtf8(exc.what());
  QString head = text.section(".",0,0);
  QString tail = text.section(".", 1);

  QString html = QString("<table>%1</table>").arg(
        QString(m_HtmlTemplate).arg(
          "dlg_error_32", QString("<b>%1.</b> %2").arg(head, tail)));

  m_OutMessage->setText(html);
  return false;
}

void
AbstractPage::WarningMessage(const IRISWarningList &wl)
{
  if(wl.size())
    {
    QString html;
    for(size_t i = 0; i < wl.size(); i++)
      {
      QString text = QString::fromUtf8(wl[i].what());
      QString head = text.section(".",0,0);
      QString tail = text.section(".", 1);
      html += QString(m_HtmlTemplate).arg(
            "dlg_warning_32", QString("<b>%1.</b> %2").arg(head, tail));
      }
    html = QString("<table>%1</table>").arg(html);
    m_OutMessage->setText(html);
    }
}

bool AbstractPage::PerformIO()
{
  // Get the selected format
  QString format = this->field("Format").toString();
  QString filename = this->field("Filename").toString();

  ImageIOWizardModel::FileFormat fmt = m_Model->GetFileFormatByName(to_utf8(format));

  try
    {
    QtCursorOverride curse(Qt::WaitCursor);
    m_Model->SetSelectedFormat(fmt);
    if(m_Model->IsLoadMode())
      {
      m_Model->LoadImage(to_utf8(filename));
      }
    else
      {
      m_Model->SaveImage(to_utf8(filename));
      }
    }
  catch(IRISException &exc)
    {
    return ErrorMessage(exc);
    }

  return true;
}

bool
AbstractPage::ErrorMessage(const char *subject, const char *detail)
{
  QString html = QString(
        "<html><body><ul>"
        "<li><b>%1</b>%2</li>"
        "</ul></body></html>").arg(QString::fromUtf8(subject),
                                   QString::fromUtf8(detail));

  m_OutMessage->setText(html);

  return false;
}

bool
AbstractPage::ConditionalError(bool rc, const char *subject, const char *detail)
{
  if(!rc)
    {
    return ErrorMessage(subject, detail);
    }
  return true;
}



SelectFilePage::SelectFilePage(QWidget *parent)
  : AbstractPage(parent)
{
  // setTitle("Select 3D Image to Load");

  // Lay out the page
  QVBoxLayout *lo = new QVBoxLayout(this);

  // File input
  m_FilePanel = new FileChooserPanelWithHistory(this);
  m_FilePanel->setSizePolicy(QSizePolicy(QSizePolicy::Preferred,QSizePolicy::Minimum));
  lo->addWidget(m_FilePanel);
  //lo->addSpacing(15);

  // The output message
  lo->addStretch(1);
  lo->addWidget(m_OutMessage);

  // Register the fields
  this->registerField("Filename*", m_FilePanel, "absoluteFilename", SIGNAL(absoluteFilenameChanged(QString)));
  this->registerField("Format*", m_FilePanel, "activeFormat", SIGNAL(activeFormatChanged(QString)));

  // Connect slots
  QMetaObject::connectSlotsByName(this);

  connect(m_FilePanel, SIGNAL(absoluteFilenameChanged(QString)), this, SLOT(onFilenameChanged(QString)));

  QWizard *wiz = dynamic_cast<QWizard *>(parent);
  connect(wiz, SIGNAL(accepted()), m_FilePanel, SLOT(onFilenameAccept()));
}

void SelectFilePage::SetFilename(
    const std::string &filename,
    GuidedNativeImageIO::FileFormat format)
{
  m_FilePanel->setFilename(from_utf8(filename));
  m_FilePanel->setActiveFormat(from_utf8(m_Model->GetFileFormatName(format)));
}

/*
class QtRegistryTableModel : public QAbstractTableModel
{
  Q_OBJECT

public:

  typedef std::vector<Registry> RegistryArray;

  explicit QtRegistryTableModel(QObject *parent, const RegistryArray &reg);

  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;

protected:

  RegistryArray &m_Array;
};
*/


void SelectFilePage::initializePage()
{
  assert(m_Model);

  // Reset the model
  m_Model->Reset();

  // Set title
  if(m_Model->IsLoadMode())
    setTitle(QString("Open %1").arg(m_Model->GetDisplayName().c_str()));
  else
    setTitle(QString("Save %1").arg(m_Model->GetDisplayName().c_str()));

  // Create a filter for the filename panel
  std::string filter = m_Model->GetFilter("%s (%s)", "*.%s", " ", ";;");

  // Determine the active format to use
  QString activeFormat;

  if(m_Model->IsSaveMode())
    activeFormat = from_utf8(m_Model->GetDefaultFormatForSave());

  if(m_Model->GetSelectedFormat() < GuidedNativeImageIO::FORMAT_COUNT)
    {
    activeFormat = from_utf8(m_Model->GetFileFormatName(m_Model->GetSelectedFormat()));
    }
  else if(m_Model->GetSuggestedFormat() < GuidedNativeImageIO::FORMAT_COUNT)
    {
    activeFormat = from_utf8(m_Model->GetFileFormatName(m_Model->GetSuggestedFormat()));
    }

  // Initialize the file panel
  if(m_Model->IsLoadMode())
    {
    m_FilePanel->initializeForOpenFile(
          m_Model->GetParent(),
          "Image Filename:",
          from_utf8(m_Model->GetHistoryName()),
          from_utf8(filter),
          from_utf8(m_Model->GetSuggestedFilename()),
          activeFormat);
    }
  else
    {
    m_FilePanel->initializeForSaveFile(
          m_Model->GetParent(),
          "Image Filename:",
          from_utf8(m_Model->GetHistoryName()),
          from_utf8(filter),
          false,
          from_utf8(m_Model->GetSuggestedFilename()),
          activeFormat);

    WarningMessage(m_Model->GetWarnings());
    }

  // Provide a callback for determining format from filename
  m_FilePanel->setCustomFormatOracle(this, "customFormatOracle");
}




bool SelectFilePage::validatePage()
{
  // Clear error state
  m_OutMessage->clear();

  // Get the selected format
  QString format = m_FilePanel->activeFormat();
  ImageIOWizardModel::FileFormat fmt = m_Model->GetFileFormatByName(to_utf8(format));

  // If can't handle the format, return false
  if(!m_Model->CanHandleFileFormat(fmt))
    {
    return ErrorMessage("File format is not supported for this operation");
    }

  // If format is RAW, continue to next page
  if(fmt == GuidedNativeImageIO::FORMAT_RAW)
    return true;

  // If format is DICOM, process the DICOM directory
  if(fmt == GuidedNativeImageIO::FORMAT_DICOM_DIR)
    return true;

  // Save or load the image
  return this->PerformIO();
}

void SelectFilePage::onFilenameChanged(QString absoluteFilename)
{
  // Is it a directory?
  if(QFileInfo(absoluteFilename).isDir())
    return;

  emit completeChanged();
}

QString SelectFilePage::customFormatOracle(QString filename)
{
  // Guess the file format using history information
  bool file_exists;
  GuidedNativeImageIO::FileFormat fmt =
      m_Model->GuessFileFormat(to_utf8(filename), file_exists);

  // Return the stinr
  if(fmt != GuidedNativeImageIO::FORMAT_COUNT)
    return from_utf8(m_Model->GetFileFormatName(fmt));

  // Return empty string - we failed
  return QString();
}


SummaryPage::SummaryPage(QWidget *parent) :
  AbstractPage(parent)
{
  // Set up the UI
  m_Tree = new QTreeWidget();
  m_Tree->setColumnCount(2);

  QStringList header;
  header << "Property" << "Value";
  m_Tree->setHeaderLabels(header);

  m_Tree->setAlternatingRowColors(true);

  QVBoxLayout *lo = new QVBoxLayout(this);
  lo->addWidget(m_Tree);

  // Set up the warnings
  lo->addWidget(m_OutMessage);
}



void SummaryPage::AddItem(
    QTreeWidgetItem *parent,
    const char *key,
    ImageIOWizardModel::SummaryItem si)
{
  QTreeWidgetItem *item = new QTreeWidgetItem(parent);
  item->setText(0, key);
  item->setText(1, from_utf8(m_Model->GetSummaryItem(si)));
}

void SummaryPage::AddItem(
    QTreeWidget *parent,
    const char *key,
    ImageIOWizardModel::SummaryItem si)
{
  QTreeWidgetItem *item = new QTreeWidgetItem(parent);
  item->setText(0, key);
  item->setText(1, from_utf8(m_Model->GetSummaryItem(si)));
}




void SummaryPage::initializePage()
{
  // Set the title
  this->setTitle("Image Summary");

  // Fill the tree widget
  m_Tree->clear();

  // Add all the top level items
  AddItem(m_Tree, "File name", ImageIOWizardModel::SI_FILENAME);
  AddItem(m_Tree, "Dimensions", ImageIOWizardModel::SI_DIMS);
  AddItem(m_Tree, "Voxel spacing", ImageIOWizardModel::SI_SPACING);
  AddItem(m_Tree, "Origin", ImageIOWizardModel::SI_ORIGIN);
  AddItem(m_Tree, "Orientation", ImageIOWizardModel::SI_ORIENT);
  AddItem(m_Tree, "Byte order", ImageIOWizardModel::SI_ENDIAN);
  AddItem(m_Tree, "Components/Voxel", ImageIOWizardModel::SI_COMPONENTS);
  AddItem(m_Tree, "Data type", ImageIOWizardModel::SI_DATATYPE);
  AddItem(m_Tree, "File size", ImageIOWizardModel::SI_FILESIZE);

  // Create metadata item
  QTreeWidgetItem *meta = new QTreeWidgetItem(m_Tree);
  meta->setText(0, "Metadata");

  // Add all metadata items
  MetaDataAccess mda(m_Model->GetGuidedIO()->GetNativeImage());
  std::vector<std::string> keys = mda.GetKeysAsArray();
  for(size_t i = 0; i < keys.size(); i++)
    {
    QTreeWidgetItem *item = new QTreeWidgetItem(meta);
    item->setText(0, mda.MapKeyToDICOM(keys[i]).c_str());
    item->setText(1, mda.GetValueAsString(keys[i]).c_str());
    }

  m_Tree->resizeColumnToContents(0);
  m_Tree->resizeColumnToContents(1);

  // Show the warning messages generated so far
  WarningMessage(m_Model->GetWarnings());
}

bool SummaryPage::validatePage()
{
  m_Model->Finalize();
  return true;
}


DICOMPage::DICOMPage(QWidget *parent)
  : AbstractPage(parent)
{
  // Set up a table widget
  m_Table = new DICOMListingTable();
  QVBoxLayout *lo = new QVBoxLayout(this);
  lo->addWidget(m_Table);
  lo->addWidget(m_OutMessage);

  connect(m_Table->selectionModel(),
          SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          SIGNAL(completeChanged()));
}

void DICOMPage::initializePage()
{
  // Set the title, subtitle
  setTitle("Select DICOM series to open");

  // Process the DICOM directory on a timer - so that the GUI shows first
  QTimer::singleShot(0, this, SLOT(processDicomDirectory()));
}

void DICOMPage::cleanupPage()
{
  AbstractPage::cleanupPage();
}

#include "ProcessEventsITKCommand.h"

void DICOMPage::processDicomDirectory()
{
  // Change cursor until this object moves out of scope
  QtCursorOverride curse(Qt::WaitCursor); (void) curse;

  // Disable the buttons until we finish loading
  this->setEnabled(false);

  try
    {
    // Callback that will handle progress of DICOM loading
    SmartPtr<ProcessEventsITKCommand> cmd = ProcessEventsITKCommand::New();

    // Timer that will call a slot at regular intervals to update the table
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), this, SLOT(updateTable()));
    timer.start(100);

    // Get the DICOM directory contents. The command and the timer make sure
    // that the table of DICOM entries is updated every 100 ms
    m_Model->ProcessDicomDirectory(to_utf8(field("Filename").toString()), cmd);

    // Stop the timer
    timer.stop();

    // Update the data
    this->updateTable();
    }
  catch(IRISException &exc)
    {
    ErrorMessage(exc);
    }

  // Enable the buttons until we finish loading
  this->setEnabled(true);
}

void DICOMPage::updateTable()
{
  // Get the list of existing series
  // TODO: this is a cluge!
  std::list<std::string> series_ids = m_Model->GetFoundDicomSeriesIds();
  std::vector<Registry> reg;
  for(std::list<std::string>::const_iterator it = series_ids.begin();
      it != series_ids.end(); ++it)
    {
    reg.push_back(m_Model->GetFoundDicomSeriesMetaData(*it));
    }

  m_Table->setData(reg);
}

bool DICOMPage::validatePage()
{
  // Clear error state
  m_OutMessage->clear();

  // Get the data associated with the selected row
  int row = m_Table->selectionModel()->selectedRows().front().row();
  std::string series_id =
      to_utf8(m_Table->item(row, 0)->data(Qt::UserRole).toString());

  try
    {
    QtCursorOverride curse(Qt::WaitCursor);
    m_Model->LoadDicomSeries(to_utf8(this->field("Filename").toString()), series_id);
    }
  catch(IRISException &exc)
    {
    return ErrorMessage(exc);
    }

  return true;
}


bool DICOMPage::isComplete() const
{
  return m_Table->selectionModel()->selectedRows().size() == 1;
}


RawPage::RawPage(QWidget *parent)
  : AbstractPage(parent)
{
  // Create a new layout
  QGridLayout *lo = new QGridLayout(this);

  // Create the header input
  m_HeaderSize = new QSpinBox();
  lo->addWidget(new QLabel("Header size:"), 0, 0, 1, 2);
  lo->addWidget(m_HeaderSize, 0, 2, 1, 1);
  lo->addWidget(new QLabel("bytes"), 0, 3, 1, 4);

  m_HeaderSize->setToolTip(
        "Specify the number of bytes at the beginning of the image that "
        "should be skipped before reading the image data.");

  connect(m_HeaderSize, SIGNAL(valueChanged(int)), SLOT(onHeaderSizeChange()));

  // Create the dimensions input
  for(size_t i = 0; i < 3; i++)
    {
    m_Dims[i] = new QSpinBox();
    connect(m_Dims[i], SIGNAL(valueChanged(int)), SLOT(onHeaderSizeChange()));

    m_Spacing[i] = new QDoubleSpinBox();
    m_Spacing[i]->setValue(1.0);
    }

  lo->addWidget(new QLabel("Image dimensions:"), 1, 0, 1, 1);
  lo->addWidget(new QLabel("x:"), 1, 1, 1, 1);
  lo->addWidget(m_Dims[0], 1, 2, 1, 1);
  lo->addWidget(new QLabel("y:"), 1, 3, 1, 1);
  lo->addWidget(m_Dims[1], 1, 4, 1, 1);
  lo->addWidget(new QLabel("z:"), 1, 5, 1, 1);
  lo->addWidget(m_Dims[2], 1, 6, 1, 1);

  // Voxel representation
  /*
  PIXELTYPE_UCHAR=0, PIXELTYPE_CHAR, PIXELTYPE_USHORT, PIXELTYPE_SHORT,
  PIXELTYPE_UINT, PIXELTYPE_INT, PIXELTYPE_FLOAT, PIXELTYPE_DOUBLE,
  PIXELTYPE_COUNT};
  */

  m_InFormat = new QComboBox();
  m_InFormat->addItem("8 bit unsigned integer (uchar)");
  m_InFormat->addItem("8 bit signed integer (char)");
  m_InFormat->addItem("16 bit unsigned integer (ushort)");
  m_InFormat->addItem("16 bit signed integer (short)");
  m_InFormat->addItem("32 bit unsigned integer (uint)");
  m_InFormat->addItem("32 bit signed integer (int)");
  m_InFormat->addItem("32 bit floating point (float)");
  m_InFormat->addItem("64 bit floating point (double)");

  connect(m_InFormat, SIGNAL(currentIndexChanged(int)), SLOT(onHeaderSizeChange()));

  lo->addWidget(new QLabel("Voxel type:"), 2, 0, 1, 2);
  lo->addWidget(m_InFormat, 2, 2, 1, 5);

  // Endianness
  m_InEndian = new QComboBox();
  m_InEndian->addItem("Big Endian (PowerPC, SPARC)");
  m_InEndian->addItem("Little Endian (x86, x86_64)");

  lo->addWidget(new QLabel("Byte alignment:"), 3, 0, 1, 2);
  lo->addWidget(m_InEndian, 3, 2, 1, 5);

  // Add some space
  lo->setRowMinimumHeight(4,16);

  // File sizes
  m_OutImpliedSize = new QSpinBox();
  m_OutImpliedSize->setReadOnly(true);
  m_OutImpliedSize->setButtonSymbols(QAbstractSpinBox::NoButtons);
  m_OutImpliedSize->setRange(0, 0x7fffffff);
  lo->addWidget(new QLabel("Implied file size:"), 5, 0, 1, 2);
  lo->addWidget(m_OutImpliedSize, 5, 2, 1, 1);

  m_OutActualSize = new QSpinBox();
  m_OutActualSize->setReadOnly(true);
  m_OutActualSize->setButtonSymbols(QAbstractSpinBox::NoButtons);
  m_OutActualSize->setRange(0, 0x7fffffff);
  lo->addWidget(new QLabel("Actual file size:"), 6, 0, 1, 2);
  lo->addWidget(m_OutActualSize, 6, 2, 1, 1);

  QLabel *lbrace = new QLabel("}");
  lbrace->setStyleSheet("font-size: 30px");
  lo->addWidget(lbrace, 5, 3, 2, 1);
  lo->addWidget(new QLabel("should be equal"), 5, 4, 2, 2);

  // Add some space
  lo->setRowMinimumHeight(6,16);

  lo->addWidget(new QLabel("Voxel Spacing:"), 7, 0, 1, 1);
  lo->addWidget(new QLabel("x:"), 7, 1, 1, 1);
  lo->addWidget(m_Spacing[0], 7, 2, 1, 1);
  lo->addWidget(new QLabel("y:"), 7, 3, 1, 1);
  lo->addWidget(m_Spacing[1], 7, 4, 1, 1);
  lo->addWidget(new QLabel("z:"), 7, 5, 1, 1);
  lo->addWidget(m_Spacing[2], 7, 6, 1, 1);


  lo->addWidget(m_OutMessage, 9, 0, 1, 7);

  // The output label
  lo->setColumnMinimumWidth(0, 140);

  // Initialize this field
  m_FileSize = 0;
}

void RawPage::onHeaderSizeChange()
{
  // Check the size of the file
  const int nbytes[] = { 1, 1, 2, 2, 4, 4, 4 };
  unsigned long myfs =
      m_HeaderSize->value() +
      m_Dims[0]->value() * m_Dims[1]->value() * m_Dims[2]->value() *
      nbytes[m_InFormat->currentIndex()];

  m_OutImpliedSize->setValue(myfs);

  emit completeChanged();
}

bool RawPage::isComplete() const
{
  // Check if required fields are filled
  if(!AbstractPage::isComplete())
    return false;

  // Check if the sizes match
  return(m_OutImpliedSize->value() == m_OutActualSize->value());
}

void RawPage::initializePage()
{
  setTitle("Image Header Specification:");

  // Get the hints (from previous experience with this file)
  Registry hint = m_Model->GetHints();

  // Get the size of the image in bytes
  m_FileSize = m_Model->GetFileSizeInBytes(to_utf8(field("Filename").toString()));
  m_OutActualSize->setValue(m_FileSize);

  // Assign to the widgets
  m_HeaderSize->setValue(hint["Raw.HeaderSize"][0]);
  m_HeaderSize->setRange(0, m_FileSize);

  // Set the dimensions
  Vector3i dims = hint["Raw.Dimensions"][Vector3i(0)];
  for(size_t i = 0; i < 3; i++)
    {
    m_Dims[i]->setValue(dims[i]);
    m_Dims[i]->setRange(1, m_FileSize);
    }

  // Set the spacing
  Vector3d spc = hint["Raw.Spacing"][Vector3d(1.0)];
  for(size_t i = 0; i < 3; i++)
    {
    m_Spacing[i]->setValue(spc[i]);
    }

  // Set the data type (default to uchar)
  int ipt = (int) (GuidedNativeImageIO::GetPixelType(
        hint, GuidedNativeImageIO::PIXELTYPE_UCHAR) -
                   GuidedNativeImageIO::PIXELTYPE_UCHAR);
  m_InFormat->setCurrentIndex(ipt);

  // Set the endian (default to LE)
  int ien = hint["Raw.BigEndian"][false] ? 0 : 1;
  m_InEndian->setCurrentIndex(ien);
}

bool RawPage::validatePage()
{
  // Clear error state
  m_OutMessage->clear();

  // Get the hints (from previous experience with this file)
  Registry &hint = m_Model->GetHints();

  // Set up the registry with the specified values
  hint["Raw.HeaderSize"] << (unsigned int) m_HeaderSize->value();

  // Set the dimensions
  hint["Raw.Dimensions"] << Vector3i(
    m_Dims[0]->value(), m_Dims[1]->value(), m_Dims[2]->value());

  // Set the spacing
  hint["Raw.Spacing"] << Vector3d(
    m_Spacing[0]->value(), m_Spacing[1]->value(), m_Spacing[2]->value());

  // Set the endianness
  hint["Raw.BigEndian"] << ( m_InEndian->currentIndex() == 0 );

  // Set the pixel type
  int iPixType = m_InFormat->currentIndex();
  GuidedNativeImageIO::RawPixelType pixtype = (iPixType < 0)
    ? GuidedNativeImageIO::PIXELTYPE_COUNT
    : (GuidedNativeImageIO::RawPixelType) iPixType;
  GuidedNativeImageIO::SetPixelType(hint, pixtype);

  // Try loading the image
  QtCursorOverride curse(Qt::WaitCursor);
  try
    {
    m_Model->SetSelectedFormat(GuidedNativeImageIO::FORMAT_RAW);
    m_Model->LoadImage(to_utf8(field("Filename").toString()));
    }
  catch(IRISException &exc)
    {
    return ErrorMessage(exc);
    }
  return true;
}











} // namespace

using namespace imageiowiz;

ImageIOWizard::ImageIOWizard(QWidget *parent) :
    QWizard(parent)
{
  // Give ourselves a name
  this->setObjectName("wizImageIO");

  // Set consistent style on all systems
  this->setWizardStyle(QWizard::ClassicStyle);

  // Use parent widget's attributes
  if(parent)
    this->setAttribute(Qt::WA_PaintOnScreen, parent->testAttribute(Qt::WA_PaintOnScreen));

  // Add pages to the wizard
  setPage(Page_File, new SelectFilePage(this));
  setPage(Page_Summary, new SummaryPage(this));
  setPage(Page_DICOM, new DICOMPage(this));
  setPage(Page_Raw, new RawPage(this));
  setPage(Page_OverlayRole, new OverlayRolePage(this));

}

void ImageIOWizard::SetModel(ImageIOWizardModel *model)
{
  // Store the model
  this->m_Model = model;

  // Assign the model to all pages
  QList<int> ids = pageIds();
  for(QList<int>::iterator it=ids.begin();it!=ids.end();it++)
    static_cast<AbstractPage *>(this->page(*it))->SetModel(model);

  // Set the title
  if(model->IsLoadMode())
    this->setWindowTitle("Open Image - ITK-SNAP");
  else
    this->setWindowTitle("Save Image - ITK-SNAP");
}

void ImageIOWizard::SetFilename(const std::string &filename,
                                GuidedNativeImageIO::FileFormat format)
{
  SelectFilePage *pFile = static_cast<SelectFilePage *>(this->page(Page_File));
  pFile->SetFilename(filename, format);
}

int ImageIOWizard::nextId() const
{
  // Determine the sequence of pages based on current state
  std::list<int> pages;

  // Always start with the file page
  pages.push_back(Page_File);

  // All other pages only occur in load mode
  if(m_Model->IsLoadMode())
    {
    // DICOM or RAW page?
    ImageIOWizardModel::FileFormat fmt =
        m_Model->GetFileFormatByName(to_utf8(this->field("Format").toString()));

    if(fmt == GuidedNativeImageIO::FORMAT_RAW)
      pages.push_back(Page_Raw);
    else if(fmt == GuidedNativeImageIO::FORMAT_DICOM_DIR)
      pages.push_back(Page_DICOM);

    // Overlay display page?
    if(m_Model->IsOverlay())
      pages.push_back(Page_OverlayRole);

    // Summary page
    pages.push_back(Page_Summary);
    pages.push_back(-1);
    }
  else
    {
    pages.push_back(-1);
    }

  // Find the page
  for(std::list<int>::const_iterator it = pages.begin(); it != pages.end(); ++it)
    {
    if(*it == this->currentId())
      {
      ++it;
      return *it;
      }
    }

  return -1;
}

int ImageIOWizard::nextPageAfterLoad()
{
  if(m_Model->IsOverlay() && m_Model->IsLoadMode())
    return ImageIOWizard::Page_OverlayRole;
  else
    return ImageIOWizard::Page_Summary;


}


