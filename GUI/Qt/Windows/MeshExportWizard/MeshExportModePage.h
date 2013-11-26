#ifndef MESHEXPORTMODEPAGE_H
#define MESHEXPORTMODEPAGE_H

#include <QWizardPage>

namespace Ui {
class MeshExportModePage;
}

class MeshExportModel;

class MeshExportModePage : public QWizardPage
{
  Q_OBJECT
  
public:
  explicit MeshExportModePage(QWidget *parent = 0);
  ~MeshExportModePage();

  void SetModel(MeshExportModel *model);

  virtual void initializePage();
  
private:
  Ui::MeshExportModePage *ui;

  MeshExportModel *m_Model;
};

#endif // MESHEXPORTMODEPAGE_H
