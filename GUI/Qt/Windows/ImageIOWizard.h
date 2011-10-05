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
class QTableWidget;
class QSpinBox;

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

  bool ErrorMessage(const char *subject, const char *detail = NULL);
  bool ConditionalError(bool rc, const char *subject, const char *detail);
  bool ErrorMessage(const IRISException &exc);
  void WarningMessage(const IRISWarningList &wl);
  ImageIOWizardModel *m_Model;

  // A qlabel for displaying error/warning messages. The children are
  // responsible for placing this control on their layouts
  QLabel *m_OutMessage;

  static const QString m_HtmlTemplate;
};

class SelectFilePage : public AbstractPage
{
  Q_OBJECT

public:
  explicit SelectFilePage(QWidget *parent = 0);

  int nextId() const;
  void initializePage();
  bool validatePage();
  // bool isComplete() const;

public slots:

  void on_btnBrowse_pressed();
  void on_inFilename_textChanged(const QString &text);
  void onHistorySelection();

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

  bool validatePage();


private:
  // Helper for building the tree
  void AddItem(QTreeWidgetItem *parent, const char *key, ImageIOWizardModel::SummaryItem si);
  void AddItem(QTreeWidget *parent, const char *key, ImageIOWizardModel::SummaryItem si);

  QTreeWidget *m_Tree;
  QLabel *m_Warnings;
};


class DICOMPage : public AbstractPage
{
  Q_OBJECT

public:

  explicit DICOMPage(QWidget *parent = 0);
  int nextId() const;
  void initializePage();
  bool validatePage();

private:

  QTableWidget *m_Table;
};

class RawPage : public AbstractPage
{
  Q_OBJECT

public:

  explicit RawPage(QWidget *parent = 0);
  int nextId() const;
  void initializePage();
  bool validatePage();
  virtual bool isComplete() const;

public slots:
  void onHeaderSizeChange();

private:
  QSpinBox *m_Dims[3], *m_HeaderSize;
  QComboBox *m_InFormat, *m_InEndian;
  QSpinBox *m_OutImpliedSize, *m_OutActualSize;
  unsigned long m_FileSize;
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
