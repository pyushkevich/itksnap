#ifndef IMAGEIOWIZARD_H
#define IMAGEIOWIZARD_H

#include <QWizard>
#include "SNAPCommon.h"
#include "ImageIOWizardModel.h"

class QLineEdit;
class QPushButton;
class QComboBox;
class QLabel;
class QFileDialog;
class QStandardItemModel;
class QMenu;
class QTreeWidget;
class QTreeWidgetItem;

// Helper classes in their own namespace, so I can use simple class names
namespace imageiowiz
{

class AbstractPage : public QWizardPage
{
  Q_OBJECT

public:
  explicit AbstractPage(QWidget *parent = 0);

  irisSetMacro(Model, ImageIOWizardModel *)
  irisGetMacro(Model, ImageIOWizardModel *)

protected:

  ImageIOWizardModel *m_Model;
};

class SelectFilePage : public AbstractPage
{
  Q_OBJECT

public:
  explicit SelectFilePage(QWidget *parent = 0);

  int nextId();
  void initializePage();
  bool validatePage();
  // bool isComplete() const;

public slots:

  void on_btnBrowse_pressed();
  void on_inFilename_textChanged(const QString &text);
  void on_HistorySelection();

private:
  QLineEdit *m_InFilename;
  QPushButton *m_BtnBrowse, *m_BtnHistory;
  QComboBox *m_InFormat;
  QLabel *m_OutFilenameError;
  QFileDialog *m_BrowseDialog;
  QStandardItemModel *m_FormatModel;
  QMenu *m_HistoryMenu;
};

class SummaryPage : public AbstractPage
{
  Q_OBJECT

public:
  explicit SummaryPage(QWidget *parent = 0);

  int nextId() const { return -1; }
  void initializePage();

private:
  // Helper for building the tree
  void AddItem(QTreeWidgetItem *parent, const char *key, ImageIOWizardModel::SummaryItem si);
  void AddItem(QTreeWidget *parent, const char *key, ImageIOWizardModel::SummaryItem si);

  QTreeWidget *m_Tree;
};

} // end namespace

class ImageIOWizard : public QWizard
{
  Q_OBJECT




public:

  enum { Page_File, Page_Raw, Page_DICOM, Page_Summary };

  explicit ImageIOWizard(QWidget *parent = 0);

  void SetModel(ImageIOWizardModel *model);

signals:

public slots:

protected:


  // The model
  ImageIOWizardModel *m_Model;
};

#endif // IMAGEIOWIZARD_H
