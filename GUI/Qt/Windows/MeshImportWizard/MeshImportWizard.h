#ifndef MESHIMPORTWIZARD_H
#define MESHIMPORTWIZARD_H

#include <QWizard>

class MeshImportModel;

namespace Ui {
  class MeshImportWizard;
}

class MeshImportWizard : public QWizard
{
  Q_OBJECT

public:
  explicit MeshImportWizard(QWidget *parent = nullptr);
  ~MeshImportWizard();

  void SetModel(MeshImportModel *model);

protected:
  MeshImportModel *m_Model;

private:
  Ui::MeshImportWizard *ui;
};

#endif // MESHIMPORTWIZARD_H
