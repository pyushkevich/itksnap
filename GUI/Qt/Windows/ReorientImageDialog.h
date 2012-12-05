#ifndef REORIENTIMAGEDIALOG_H
#define REORIENTIMAGEDIALOG_H

#include <QDialog>
#include <SNAPCommon.h>

class ReorientImageModel;
class OrientationGraphicRenderer;

namespace Ui {
class ReorientImageDialog;
}

class ReorientImageDialog : public QDialog
{
  Q_OBJECT
  
public:
  explicit ReorientImageDialog(QWidget *parent = 0);
  ~ReorientImageDialog();

  void SetModel(ReorientImageModel *model);

protected:

  ReorientImageModel *m_Model;
  
private slots:
  void on_btnApply_clicked();

  void on_btnReverseX_clicked();

  void on_btnReverseY_clicked();

  void on_btnReverseZ_clicked();

private:
  Ui::ReorientImageDialog *ui;

  // TODO: it seems wrong to have the Qt classes own renderer objects,
  // perhaps this should be owned by the model instead?
  SmartPtr<OrientationGraphicRenderer> m_CurrentRAIRenderer, m_NewRAIRenderer;

};

#endif // REORIENTIMAGEDIALOG_H
