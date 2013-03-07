#ifndef DISPLAYLAYOUTINSPECTOR_H
#define DISPLAYLAYOUTINSPECTOR_H

#include <QWidget>

class DisplayLayoutModel;

namespace Ui {
class DisplayLayoutInspector;
}

class DisplayLayoutInspector : public QWidget
{
  Q_OBJECT
  
public:
  explicit DisplayLayoutInspector(QWidget *parent = 0);
  ~DisplayLayoutInspector();

  void SetModel(DisplayLayoutModel *model);
    
private:
  Ui::DisplayLayoutInspector *ui;

  DisplayLayoutModel *m_Model;
};

#endif // DISPLAYLAYOUTINSPECTOR_H
