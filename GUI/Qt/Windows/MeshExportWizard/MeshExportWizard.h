#ifndef MESHEXPORTWIZARD_H
#define MESHEXPORTWIZARD_H

#include <QWizard>

namespace Ui {
class MeshExportWizard;
}

class MeshExportModel;

class MeshExportWizard : public QWizard
{
  Q_OBJECT
  
public:
  explicit MeshExportWizard(QWidget *parent = 0);
  ~MeshExportWizard();

  void SetModel(MeshExportModel *model);
  
private:

  MeshExportModel *m_Model;

  Ui::MeshExportWizard *ui;
};

#endif // MESHEXPORTWIZARD_H
