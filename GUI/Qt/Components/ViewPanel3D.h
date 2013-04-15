#ifndef VIEWPANEL3D_H
#define VIEWPANEL3D_H

#include <SNAPComponent.h>

namespace Ui {
  class ViewPanel3D;
}

class Generic3DModel;
class GenericView3D;

class ViewPanel3D : public SNAPComponent
{
  Q_OBJECT

public:
  explicit ViewPanel3D(QWidget *parent = 0);
  ~ViewPanel3D();

  // Register with the global model
  void Initialize(GlobalUIModel *model);

  void OnRenderProgress();

  GenericView3D *Get3DView();

private slots:
  void on_btnUpdateMesh_clicked();

  void on_btnScreenshot_clicked();

  void on_btnResetView_clicked();

private:
  Ui::ViewPanel3D *ui;

  GlobalUIModel *m_GlobalUI;

  Generic3DModel *m_Model;

};

#endif // VIEWPANEL3D_H
