#ifndef MESHIMPORTWIZARD_H
#define MESHIMPORTWIZARD_H

#include <QWizard>
#include <QMessageBox>

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

  /** Create a message box reminding user about loading mesh
   *  to the current time point of a new mesh layer
   *  ***Caller should delete the returned object after use! ***
   */
  static QMessageBox *
  CreateLoadToNewLayerMessageBox(QWidget *parent, unsigned int displayTP);

  /** Create a message box reminding user about loading mesh
   *  to the current time point of a new mesh layer
   *  ***Caller should delete the returned object after use! ***
   */
  static QMessageBox *
  CreateLoadToTimePointMessageBox(QWidget *parent, unsigned int displayTP);

protected:
  MeshImportModel *m_Model;

private:
  Ui::MeshImportWizard *ui;
};

#endif // MESHIMPORTWIZARD_H
