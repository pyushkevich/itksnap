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

#include "IRISException.h"
#include "ImageIOWizardModel.h"
#include "MetaDataAccess.h"

namespace imageiowiz {

AbstractPage::AbstractPage(QWidget *parent)
  : QWizardPage(parent)
{
  this->m_Model = NULL;
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
  m_OutFilenameError->setStyleSheet("font-size:10pt; color: #7f0000;");
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

  QLabel *label2 = new QLabel("Image file &format:");
  label2->setBuddy(m_InFormat);

  lo->addWidget(label2);
  lo->addWidget(m_InFormat);
  lo->addStretch(1);


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


void SelectFilePage::initializePage()
{
  assert(m_Model);

  // Set title
  if(m_Model->IsLoadMode())
    setTitle("Open 3D Image");
  else
    setTitle("Save 3D Image");

  // Clear the combo box model
  m_FormatModel->clear();

  // Populate the combo box
  for(unsigned int i = 0; i < GuidedNativeImageIO::FORMAT_COUNT; i++)
    {
    // Create an appropriate description
    GuidedNativeImageIO::FileFormat fmt = static_cast<GuidedNativeImageIO::FileFormat>(i);
    GuidedNativeImageIO::FileFormatDescriptor fd =
      GuidedNativeImageIO::GetFileFormatDescriptor(fmt);

    std::string text = fd.name + " File";

    QStandardItem *item = new QStandardItem(text.c_str());
    item->setData(QVariant(fmt), Qt::UserRole);
    if(!m_Model->CanHandleFileFormat(fmt))
      item->setEnabled(false);
    m_FormatModel->appendRow(item);

    // Add a menu option to the save menu, disabling it if it's unsupported
    m_InFormat->addItem(text.c_str(), QVariant(fmt));
    }

  // Populate the history button
  m_HistoryMenu->clear();
  ImageIOWizardModel::HistoryType history = m_Model->GetHistory();
  for(unsigned int i = 0; i < history.size(); i++)
    {
    m_HistoryMenu->addAction(
          history[i].c_str(), this, SLOT(on_HistorySelection()));
    }
  m_BtnHistory->setEnabled(history.size() > 0);
}

void SelectFilePage::on_HistorySelection()
{
  QAction *action = static_cast<QAction *>(this->sender());
  m_InFilename->setText(action->text());
}

bool SelectFilePage::validatePage()
{
  // This is where the work of the page gets done
  assert(m_InFormat->currentIndex() >= 0);
  ImageIOWizardModel::FileFormat fmt =
      static_cast<ImageIOWizardModel::FileFormat>(
        m_InFormat->itemData(m_InFormat->currentIndex()).toInt());

  // If can't handle the format, return false
  if(!m_Model->CanHandleFileFormat(fmt))
    return false;

  // If format is DICOM or RAW, continue to next page
  if(fmt == GuidedNativeImageIO::FORMAT_RAW ||
     fmt == GuidedNativeImageIO::FORMAT_DICOM)
    return true;

  // If load mode, tell the model to try loading the image
  if(m_Model->IsLoadMode())
    {
    try
      {
      m_Model->SetSelectedFormat(fmt);
      m_Model->LoadImage(m_InFilename->text().toStdString());

      // To do: something about the validity check
      }
    catch(IRISException &exc)
      {
      QMessageBox mbox(this);
      mbox.setIcon(QMessageBox::Critical);
      mbox.setText("Error Loading Image");
      mbox.setDetailedText(exc.what());

      }
    }

  return true;
}


int SelectFilePage::nextId()
{
  // Get the currently selected format
  assert(m_InFormat->currentIndex() >= 0);
  ImageIOWizardModel::FileFormat fmt =
      static_cast<ImageIOWizardModel::FileFormat>(
        m_InFormat->itemData(m_InFormat->currentIndex()).toInt());

  // Depending on the format, return the next page
  if(fmt == GuidedNativeImageIO::FORMAT_RAW)
    return ImageIOWizard::Page_Raw;
  else if(fmt == GuidedNativeImageIO::FORMAT_DICOM)
    return ImageIOWizard::Page_DICOM;
  else if(m_Model->IsLoadMode())
    return ImageIOWizard::Page_Summary;
  else return -1;
}

void SelectFilePage::on_btnBrowse_pressed()
{
  // Set the dialog properties
  if(m_Model->IsLoadMode())
    {
    m_BrowseDialog->setAcceptMode(QFileDialog::AcceptOpen);
    m_BrowseDialog->setFileMode(QFileDialog::ExistingFile);
    }
  else
    {
    m_BrowseDialog->setAcceptMode(QFileDialog::AcceptSave);
    m_BrowseDialog->setFileMode(QFileDialog::AnyFile);
    m_BrowseDialog->setDefaultSuffix(
          m_Model->GetDefaultExtensionForSave().c_str());
    }

  // Initialize the dialog with what's in the filebox
  std::string file = m_InFilename->text().toStdString();
  std::string dir = m_Model->GetBrowseDirectory(file);
  if(dir.length())
    m_BrowseDialog->setDirectory(dir.c_str());

  // Get the file name
  if(m_BrowseDialog->exec())
    m_InFilename->setText(m_BrowseDialog->selectedFiles().first());
}

void SelectFilePage::on_inFilename_textChanged(const QString &text)
{
  bool file_exists = false;

  // The file format for the checkbox
  GuidedNativeImageIO::FileFormat fmt =
      m_Model->GuessFileFormat(text.toStdString(), file_exists);

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
}


void SummaryPage::AddItem(
    QTreeWidgetItem *parent,
    const char *key,
    ImageIOWizardModel::SummaryItem si)
{
  QTreeWidgetItem *item = new QTreeWidgetItem(parent);
  item->setText(0, key);
  item->setText(1, m_Model->GetSummaryItem(si).c_str());
}

void SummaryPage::AddItem(
    QTreeWidget *parent,
    const char *key,
    ImageIOWizardModel::SummaryItem si)
{
  QTreeWidgetItem *item = new QTreeWidgetItem(parent);
  item->setText(0, key);
  item->setText(1, m_Model->GetSummaryItem(si).c_str());
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
  AddItem(m_Tree, "Data type", ImageIOWizardModel::SI_DATATYPE);
  AddItem(m_Tree, "File size", ImageIOWizardModel::SI_FILESIZE);

  // Create metadata item
  QTreeWidgetItem *meta = new QTreeWidgetItem(m_Tree);
  meta->setText(0, "Metadata");

  // Add all metadata items
  MetaDataAccess mda(m_Model->GetGuidedIO()->GetNativeImage());
  for(size_t i = 0; i < mda.GetNumberOfKeys(); i++)
    {
    QTreeWidgetItem *item = new QTreeWidgetItem(meta);
    item->setText(0, mda.MapKeyToDICOM(mda.GetKey(i)).c_str());
    item->setText(1, mda.GetValueAsString(i).c_str());
    }
}








} // namespace

using namespace imageiowiz;

ImageIOWizard::ImageIOWizard(QWidget *parent) :
    QWizard(parent)
{
  // Add pages to the wizard
  setPage(Page_File, new SelectFilePage(this));
  setPage(Page_Summary, new SummaryPage(this));


}

void ImageIOWizard::SetModel(ImageIOWizardModel *model)
{
  // Store the model
  this->m_Model = model;

  // Assign the model to all pages
  QList<int> ids = pageIds();
  for(QList<int>::iterator it=ids.begin();it!=ids.end();it++)
    static_cast<AbstractPage *>(this->page(*it))->SetModel(model);
}


