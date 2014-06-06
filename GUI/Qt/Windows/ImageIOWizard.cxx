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

#include <QtCursorOverride.h>

#include "IRISException.h"
#include "ImageIOWizardModel.h"
#include "MetaDataAccess.h"
#include "SNAPQtCommon.h"

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
  QLabel *label = new QLabel("Image file &name:");
  m_InFilename = new QLineEdit();
  m_InFilename->setObjectName("inFilename");

  // Set up completer (why does it not work?)
  QCompleter *completer = new QCompleter(this);
  QFileSystemModel *fsm = new QFileSystemModel(completer);
  fsm->setResolveSymlinks(true);
  completer->setModel(fsm);
  completer->setCompletionMode(QCompleter::InlineCompletion);

  m_InFilename->setCompleter(completer);
  label->setBuddy(m_InFilename);

  lo->addWidget(label);
  lo->addWidget(m_InFilename);

  // Label for filename error messages
  m_OutFilenameError = new QLabel("");
  m_OutFilenameError->setStyleSheet("font-size:10px; color: #7f0000;");
  m_OutFilenameError->setWordWrap(true);

  // A couple of buttons
  QWidget *btnPanel = new QWidget();
  QHBoxLayout *lobtn = new QHBoxLayout(btnPanel);
  lobtn->setContentsMargins(0,0,0,0);

  m_BtnBrowse = new QPushButton("&Browse...");
  m_BtnBrowse->setObjectName("btnBrowse");

  m_BtnHistory = new QPushButton("&History");
  m_BtnHistory->setObjectName("btnHistory");

  m_HistoryMenu = new QMenu("History", m_BtnHistory);
  m_HistoryMenu->setStyleSheet("font-size: 12px;");
  m_BtnHistory->setMenu(m_HistoryMenu);

  lobtn->addWidget(m_OutFilenameError,1,Qt::AlignLeft);
  lobtn->addSpacing(10);
  lobtn->addWidget(m_BtnBrowse);
  lobtn->addWidget(m_BtnHistory);

  lo->addWidget(btnPanel);
  lo->addSpacing(15);

  // The format combo box
  m_InFormat = new QComboBox();
  m_InFormat->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  m_InFormat->setMaxVisibleItems(16);
  m_InFormat->setObjectName("inFormat");

  QLabel *label2 = new QLabel("Image file &format:");
  label2->setBuddy(m_InFormat);

  lo->addWidget(label2);
  lo->addWidget(m_InFormat);
  lo->addStretch(1);
  lo->addWidget(m_OutMessage);

  // Initialize the format model
  m_FormatModel = new QStandardItemModel(m_InFormat);
  m_InFormat->setModel(m_FormatModel);

  // Register the fields
  this->registerField("Filename*", m_InFilename);
  this->registerField("Format*", m_InFormat);

  // Create the browse dialog
  m_BrowseDialog = new QFileDialog(this);

  // Connect slots
  QMetaObject::connectSlotsByName(this);
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

  // Clear the combo box model
  m_FormatModel->clear();

  // Populate the combo box
  for(unsigned int i = 0; i < GuidedNativeImageIO::FORMAT_COUNT; i++)
    {
    // Create an appropriate description
    GuidedNativeImageIO::FileFormat fmt = static_cast<GuidedNativeImageIO::FileFormat>(i);
    GuidedNativeImageIO::FileFormatDescriptor fd =
      GuidedNativeImageIO::GetFileFormatDescriptor(fmt);

    std::string text = fd.name;

    QStandardItem *item = new QStandardItem(text.c_str());
    item->setData(QVariant(fmt), Qt::UserRole);
    if(!m_Model->CanHandleFileFormat(fmt))
      item->setEnabled(false);
    m_FormatModel->appendRow(item);
    }

  // Initialize the combo box based on the current selection
  m_InFormat->setCurrentIndex(
        m_InFormat->findData(QVariant(m_Model->GetSelectedFormat())));

  // Populate the history button
  PopulateHistoryMenu(m_HistoryMenu, this, SLOT(onHistorySelection()),
                      m_Model->GetParent(),
                      from_utf8(m_Model->GetHistoryName()));
  m_BtnHistory->setEnabled(m_HistoryMenu->actions().size() > 0);

  // Start with the suggested filename
  m_InFilename->setText(from_utf8(m_Model->GetSuggestedFilename()));
}

void SelectFilePage::onHistorySelection()
{
  QAction *action = static_cast<QAction *>(this->sender());
  m_InFilename->setText(action->text());
}



bool SelectFilePage::validatePage()
{
  // Clear error state
  m_OutMessage->clear();

  // This is where the work of the page gets done
  assert(m_InFormat->currentIndex() >= 0);
  ImageIOWizardModel::FileFormat fmt =
      static_cast<ImageIOWizardModel::FileFormat>(
        m_InFormat->itemData(m_InFormat->currentIndex()).toInt());

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
    {
    // Change cursor until this object moves out of scope
    QtCursorOverride curse(Qt::WaitCursor);
    try
      {
      m_Model->ProcessDicomDirectory(to_utf8(m_InFilename->text()));
      return true;
      }
    catch(IRISException &exc)
      {
      return ErrorMessage(exc);
      }
    }

  // Save or load the image
  try
    {
    QtCursorOverride curse(Qt::WaitCursor);
    m_Model->SetSelectedFormat(fmt);
    if(m_Model->IsLoadMode())
      {
      m_Model->LoadImage(to_utf8(m_InFilename->text()));
      }
    else
      {
      m_Model->SaveImage(to_utf8(m_InFilename->text()));
      }
    }
  catch(IRISException &exc)
    {
    return ErrorMessage(exc);
    }

  return true;
}

int SelectFilePage::nextId() const
{
  if(m_Model->IsSaveMode())
    return -1;

  if(m_InFormat->currentIndex() < 0)
    return ImageIOWizard::Page_Summary;

  // Get the currently selected format
  ImageIOWizardModel::FileFormat fmt =
      static_cast<ImageIOWizardModel::FileFormat>(
        m_InFormat->itemData(m_InFormat->currentIndex()).toInt());

  // Depending on the format, return the next page
  if(fmt == GuidedNativeImageIO::FORMAT_RAW)
    return ImageIOWizard::Page_Raw;
  else if(fmt == GuidedNativeImageIO::FORMAT_DICOM_DIR)
    return ImageIOWizard::Page_DICOM;
  else
    return ImageIOWizard::Page_Summary;
}

void SelectFilePage::on_btnBrowse_pressed()
{
  // Initialize the dialog with what's in the filebox
  std::string file = to_utf8(m_InFilename->text());

  // Set the dialog properties
  if(m_Model->IsLoadMode())
    {
    // Get the file name
    QString sel =
        GetOpenFileNameBugFix(this, "Open Image File", m_InFilename->text());

    if(sel.length())
      m_InFilename->setText(sel);
    }
  else
    {
    QFileInfo fi(from_utf8(file));
    QString sel = QFileDialog::getSaveFileName(this, "Save Image File", fi.absoluteFilePath());
    if(sel.length())
      m_InFilename->setText(sel);
    }
}

void SelectFilePage::on_inFilename_textChanged(const QString &text)
{
  bool file_exists = false;

  // The file format for the checkbox
  GuidedNativeImageIO::FileFormat fmt =
      m_Model->GuessFileFormat(to_utf8(text), file_exists);

  // Select the appropriate entry in the combo box
  m_InFormat->setCurrentIndex(m_InFormat->findData(QVariant(fmt)));

  // Add some messages to help the user
  if(text.length() == 0)
    {
    m_OutFilenameError->setText(tr(""));
    }
  else if(m_Model->IsLoadMode() && !file_exists)
    {
    m_OutFilenameError->setText(tr("The file does not exist."));
    }
  else if(fmt == GuidedNativeImageIO::FORMAT_COUNT)
    {
    m_OutFilenameError->setText(tr("The format can not be determined from the file name."));
    }
  else if(!m_Model->CanHandleFileFormat(fmt))
    {
    m_OutFilenameError->setText(tr("The format is not supported for this operation."));
    }
  else
    {
    m_OutFilenameError->setText(tr(""));
    }
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
  m_Table = new QTableWidget();
  QVBoxLayout *lo = new QVBoxLayout(this);
  lo->addWidget(m_Table);
  lo->addWidget(m_OutMessage);

  m_Table->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_Table->setSelectionMode(QAbstractItemView::SingleSelection);
  m_Table->setAlternatingRowColors(true);
  m_Table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_Table->verticalHeader()->hide();

  connect(m_Table->selectionModel(),
          SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
          SIGNAL(completeChanged()));
}

void DICOMPage::initializePage()
{
  // Set the title, subtitle
  setTitle("Select DICOM series to open");

  // Populate the DICOM page
  const std::vector<Registry> &reg = m_Model->GetDicomContents();

  m_Table->setRowCount(reg.size());
  m_Table->setColumnCount(4);
  m_Table->setHorizontalHeaderItem(0, new QTableWidgetItem("Series Number"));
  m_Table->setHorizontalHeaderItem(1, new QTableWidgetItem("Description"));
  m_Table->setHorizontalHeaderItem(2, new QTableWidgetItem("Dimensions"));
  m_Table->setHorizontalHeaderItem(3, new QTableWidgetItem("Number of Images"));

  for(size_t i = 0; i < reg.size(); i++)
    {
    Registry r = reg[i];
    m_Table->setItem(i, 0, new QTableWidgetItem(r["SeriesNumber"][""]));
    m_Table->setItem(i, 1, new QTableWidgetItem(r["SeriesDescription"][""]));
    m_Table->setItem(i, 2, new QTableWidgetItem(r["Dimensions"][""]));
    m_Table->setItem(i, 3, new QTableWidgetItem(r["NumberOfImages"][""]));
    }

  m_Table->resizeColumnsToContents();
  m_Table->resizeRowsToContents();

  // If only one sequence selected, pick it
  if(reg.size() == 1)
    {
    m_Table->selectRow(0);
    }

  // Choose the sequence previously loaded
  // TODO:

  /*
  // See if one of the sequences in the registry matches
  StringType last = m_Registry["DICOM.SequenceId"]["NULL"];
  const Fl_Menu_Item *lastpos = m_InDICOMPageSequenceId->find_item(last.c_str());
  if(lastpos)
    m_InDICOMPageSequenceId->value(lastpos);
  else
    m_InDICOMPageSequenceId->value(0);
  */
}

bool DICOMPage::validatePage()
{
  // Clear error state
  m_OutMessage->clear();

  // Add registry entries for the selected DICOM series
  int row = m_Table->selectionModel()->selectedRows().front().row();

  try
    {
    QtCursorOverride curse(Qt::WaitCursor);
    m_Model->LoadDicomSeries(to_utf8(this->field("Filename").toString()), row);
    }
  catch(IRISException &exc)
    {
    return ErrorMessage(exc);
    }

  return true;
}

int DICOMPage::nextId() const
{
  return ImageIOWizard::Page_Summary;
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

  lo->addWidget(m_OutMessage, 7, 0, 1, 7);

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

  // Set the data type (default to uchar)
  int ipt = (int) (GuidedNativeImageIO::GetPixelType(
        hint, GuidedNativeImageIO::PIXELTYPE_UCHAR) -
                   GuidedNativeImageIO::PIXELTYPE_UCHAR);
  m_InFormat->setCurrentIndex(ipt);

  // Set the endian (default to LE)
  int ien = hint["Raw.BigEndian"][false] ? 0 : 1;
  m_InEndian->setCurrentIndex(ien);
}

int RawPage::nextId() const
{
  return ImageIOWizard::Page_Summary;
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

  // Add pages to the wizard
  setPage(Page_File, new SelectFilePage(this));
  setPage(Page_Summary, new SummaryPage(this));
  setPage(Page_DICOM, new DICOMPage(this));
  setPage(Page_Raw, new RawPage(this));

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


