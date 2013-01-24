#ifndef MULTICOMPONENTINSPECTOR_H
#define MULTICOMPONENTINSPECTOR_H

#include <QWidget>

namespace Ui {
class GeneralLayerInspector;
}

class ComponentSelectionModel;

class GeneralLayerInspector : public QWidget
{
  Q_OBJECT
  
public:
  explicit GeneralLayerInspector(QWidget *parent = 0);
  ~GeneralLayerInspector();

  void SetModel(ComponentSelectionModel *model);
  
private:
  Ui::GeneralLayerInspector *ui;

  ComponentSelectionModel *m_Model;
};

#endif // MULTICHANNELINSPECTOR_H
