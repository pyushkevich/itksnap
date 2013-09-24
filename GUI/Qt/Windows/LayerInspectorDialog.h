#ifndef LAYERINSPECTORDIALOG_H
#define LAYERINSPECTORDIALOG_H

#include <QDialog>
#include <QAbstractListModel>

class IntensityCurveBox;
class ContrastInspector;
class GlobalUIModel;
class EventBucket;
class LayerInspectorRowDelegate;
class ImageWrapperBase;

namespace Ui {
    class LayerInspectorDialog;
}




class LayerInspectorDialog : public QDialog
{
  Q_OBJECT

public:
    explicit LayerInspectorDialog(QWidget *parent = 0);
    ~LayerInspectorDialog();

  void SetModel(GlobalUIModel *model);

  void SetPageToContrastAdjustment();
  void SetPageToColorMap();

  bool eventFilter(QObject *source, QEvent *event);

public slots:

  virtual void onModelUpdate(const EventBucket &bucket);

  void layerSelected(bool);

signals:

  void layerListHover(bool);

private slots:
  void on_actionSaveSelectedLayerAs_triggered();

private:
  Ui::LayerInspectorDialog *ui;
  GlobalUIModel *m_Model;

  void GenerateModelsForLayers();
  void BuildLayerWidgetHierarchy();
  void SetActiveLayer(ImageWrapperBase *layer);

  // List of layer delegate widgets
  QList<LayerInspectorRowDelegate *> m_Delegates;
};

#endif // LAYERINSPECTORDIALOG_H
