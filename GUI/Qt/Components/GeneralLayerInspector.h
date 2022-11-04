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

public slots:
  virtual void onModelUpdate(const EventBucket &bucket);
  
private slots:
  void on_btnUp_clicked();
  void on_btnDown_clicked();
  void on_spinBoxTP_valueChanged(int value);
	void meshData_domainChanged();
	void meshData_selectionChanged(int value);
//	void meshVector_selectionChanged(int value);


private:
  Ui::GeneralLayerInspector *ui;

  LayerGeneralPropertiesModel *m_Model;

  // storing the id to detect mesh layer change
  unsigned long m_CurrentlyActiveMeshLayerId;

	static std::map<int, const char*> m_MeshDataTypeToIcon;
};

#endif // MULTICHANNELINSPECTOR_H
