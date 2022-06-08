#ifndef MESHIMPORTFILESELECTIONPAGE_H
#define MESHIMPORTFILESELECTIONPAGE_H

#include <QWizardPage>

class MeshImportModel;
class GuidedMeshIO;

namespace Ui {
  class MeshImportFileSelectionPage;
}

class MeshImportFileSelectionPage : public QWizardPage
{
  Q_OBJECT

public:
  explicit MeshImportFileSelectionPage(QWidget *parent = nullptr);
  ~MeshImportFileSelectionPage();

  void SetModel(MeshImportModel *model);

  void initializePage() override;

  bool validatePage() override;

protected:
  MeshImportModel *m_Model;

private:
  Ui::MeshImportFileSelectionPage *ui;
};

#endif // MESHIMPORTFILESELECTIONPAGE_H
