#ifndef MESHEXPORTBROWSEPAGE_H
#define MESHEXPORTBROWSEPAGE_H

#include <QWizardPage>

namespace Ui {
class MeshExportBrowsePage;
}

class MeshExportModel;

class MeshExportBrowsePage : public QWizardPage
{
  Q_OBJECT
  
public:
  explicit MeshExportBrowsePage(QWidget *parent = 0);
  ~MeshExportBrowsePage();
  
  void SetModel(MeshExportModel *model);

  virtual void initializePage();
  virtual bool validatePage();
  virtual bool isComplete();

private:
  Ui::MeshExportBrowsePage *ui;

  MeshExportModel *m_Model;
};

#endif // MESHEXPORTBROWSEPAGE_H
