#ifndef MULTICOMPONENTINSPECTOR_H
#define MULTICOMPONENTINSPECTOR_H

#include <QWidget>

namespace Ui {
class GeneralLayerInspector;
}

class GlobalUIModel;
class LayerGeneralPropertiesModel;

class GeneralLayerInspector : public QWidget
{
  Q_OBJECT
  
public:
  explicit GeneralLayerInspector(QWidget *parent = 0);
  ~GeneralLayerInspector();

  void SetModel(LayerGeneralPropertiesModel *model);
  
private slots:
  void on_btnUp_clicked();
  void on_btnDown_clicked();

private:
  Ui::GeneralLayerInspector *ui;

  LayerGeneralPropertiesModel *m_Model;
};

#endif // MULTICHANNELINSPECTOR_H
