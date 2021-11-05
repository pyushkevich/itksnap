#ifndef MESHIMPORTWIZARD_H
#define MESHIMPORTWIZARD_H

#include <QWizard>

namespace Ui {
  class MeshImportWizard;
}

class MeshImportWizard : public QWizard
{
  Q_OBJECT

public:
  explicit MeshImportWizard(QWidget *parent = nullptr);
  ~MeshImportWizard();

private:
  Ui::MeshImportWizard *ui;
};

#endif // MESHIMPORTWIZARD_H
