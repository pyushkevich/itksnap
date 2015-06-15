#ifndef ANNOTATIONTOOLPANEL_H
#define ANNOTATIONTOOLPANEL_H

#include <QWidget>

namespace Ui {
class AnnotationToolPanel;
}

class AnnotationToolPanel : public QWidget
{
  Q_OBJECT

public:
  explicit AnnotationToolPanel(QWidget *parent = 0);
  ~AnnotationToolPanel();

private:
  Ui::AnnotationToolPanel *ui;
};

#endif // ANNOTATIONTOOLPANEL_H
