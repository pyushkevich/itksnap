#ifndef REORIENTIMAGEDIALOG_H
#define REORIENTIMAGEDIALOG_H

#include <QDialog>

class ReorientImageModel;

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
  
private:
  Ui::ReorientImageDialog *ui;

  static const char m_RAICodes[3][2];
  static const char *m_AxisLabels[3][2];
};

#endif // REORIENTIMAGEDIALOG_H
