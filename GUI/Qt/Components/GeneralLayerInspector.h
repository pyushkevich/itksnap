#ifndef MULTICOMPONENTINSPECTOR_H
#define MULTICOMPONENTINSPECTOR_H

#include <QWidget>
#include <EventBucket.h>

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

  static std::map<int, const char*> MeshDataTypeToIcon;

public slots:
  virtual void onModelUpdate(const EventBucket &bucket);
  
private slots:
  void on_btnUp_clicked();
  void on_btnDown_clicked();
  void on_spinBoxTP_valueChanged(int value);

private:
  Ui::GeneralLayerInspector *ui;

  LayerGeneralPropertiesModel *m_Model;

  // storing the id to detect mesh layer change
  unsigned long m_CurrentlyActiveMeshLayerId;

};

#endif // MULTICHANNELINSPECTOR_H
