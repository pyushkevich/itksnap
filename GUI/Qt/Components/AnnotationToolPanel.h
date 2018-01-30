#ifndef ANNOTATIONTOOLPANEL_H
#define ANNOTATIONTOOLPANEL_H

#include <QWidget>
class GlobalUIModel;

namespace Ui {
class AnnotationToolPanel;
}

class AnnotationToolPanel : public QWidget
{
  Q_OBJECT

public:
  explicit AnnotationToolPanel(QWidget *parent = 0);
  ~AnnotationToolPanel();

  void SetModel(GlobalUIModel *model);

private slots:
  void on_btnOpen_clicked();

  void on_btnSave_clicked();

private:
  Ui::AnnotationToolPanel *ui;
  GlobalUIModel *m_Model;
};

#endif // ANNOTATIONTOOLPANEL_H
