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
class QToolButton;
class QMenu;

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
  void SetPageToImageInfo();

  // Get the context menu corresponding to a specific layer.
  QMenu *GetLayerContextMenu(ImageWrapperBase *layer);

  bool eventFilter(QObject *source, QEvent *event);

public slots:

  virtual void onModelUpdate(const EventBucket &bucket);

  void layerSelected(bool);

signals:

  void layerListHover(bool);

private slots:
  void on_actionSaveSelectedLayerAs_triggered();

  void on_actionLayoutToggle_triggered(bool);

  void on_buttonBox_accepted();

  void on_buttonBox_rejected();

private:
  Ui::LayerInspectorDialog *ui;
  GlobalUIModel *m_Model;

  void GenerateModelsForLayers();
  void BuildLayerWidgetHierarchy();
  void SetActiveLayer(ImageWrapperBase *layer);
  void UpdateLayerLayoutAction();

  // Tool bar buttons that use actions from the selected layer widgets
  QToolButton *m_SaveSelectedButton;

  // List of layer delegate widgets
  QList<LayerInspectorRowDelegate *> m_Delegates;
};

#endif // LAYERINSPECTORDIALOG_H
