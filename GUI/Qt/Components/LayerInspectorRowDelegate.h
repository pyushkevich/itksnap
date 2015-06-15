#ifndef LAYERINSPECTORROWDELEGATE_H
#define LAYERINSPECTORROWDELEGATE_H

#include <QWidget>
#include <SNAPComponent.h>
#include "SNAPCommon.h"
#include <QWidgetAction>

class LayerTableRowModel;
class ImageWrapperBase;
class QMenu;
class QActionGroup;
class QContextMenuEvent;
class QSlider;
class QLabel;

class OpacitySliderAction : public QWidgetAction
{
  Q_OBJECT
public:
  OpacitySliderAction(QWidget *parent = 0);
  QSlider *GetSlider() { return m_Slider; }
  QWidget *GetContainer() { return m_Container; }

protected:
  virtual QWidget *createWidget(QWidget *parent);
  QSlider *m_Slider;
  QWidget *m_Container;
};


class WidgetWithLabelAction : public QWidgetAction
{
  Q_OBJECT
public:
  WidgetWithLabelAction(QWidget *parent = 0);
  void setWidget(QWidget *widget);
  virtual void setLabelText(const QString &text);

protected slots:

  void onChanged();

protected:

  QWidget *m_Container;
  QLabel *m_Label;
};

namespace Ui {
class LayerInspectorRowDelegate;
}

/**
 * @brief The LayerInspectorRowDelegate class
 * This compound widget is used to display a row in the table of layers in
 * the layer inspector. The name LayerInspectorRowDelegate is not quite right
 * as we are not actually using a Qt list widget to display layers. Instead
 * these widgets are organized in a QScrollArea.
 */
class LayerInspectorRowDelegate : public SNAPComponent
{
  Q_OBJECT
  Q_PROPERTY(bool selected READ selected WRITE setSelected USER true NOTIFY selectionChanged)

public:
  explicit LayerInspectorRowDelegate(QWidget *parent = 0);
  ~LayerInspectorRowDelegate();

  void SetModel(LayerTableRowModel *model);

  ImageWrapperBase *GetLayer() const;

  bool selected() const { return m_Selected; }

  // Access the actions for this item
  QAction * saveAction() const;
  QAction * closeAction() const;

  // Get the context menu for this item
  QMenu *contextMenu() const;

  void enterEvent(QEvent *);
  void leaveEvent(QEvent *);

  void mousePressEvent(QMouseEvent *);
  void mouseMoveEvent(QMouseEvent *);
  void mouseReleaseEvent(QMouseEvent *);
  void contextMenuEvent(QContextMenuEvent *evt);

  bool eventFilter(QObject *, QEvent *);

public slots:

  void setSelected(bool value);

  virtual void onModelUpdate(const EventBucket &bucket);

signals:
  void selectionChanged(bool);
  void contrastInspectorRequested();
  void colorMapInspectorRequested();
  
private slots:
  void on_btnMenu_pressed();

  void on_btnMoveUp_clicked();

  void on_btnMoveDown_pressed();

  void on_actionSave_triggered();

  void on_actionClose_triggered();

  void onColorMapPresetSelected();

  void on_actionAutoContrast_triggered();

  void on_actionTextureFeatures_triggered();

  void on_actionPin_layer_triggered();

  void on_actionUnpin_layer_triggered();

  void on_actionContrast_Inspector_triggered();

  void on_actionColor_Map_Editor_triggered();

private:
  Ui::LayerInspectorRowDelegate *ui;

  // It is very important here that we keep a smart pointer to the model,
  // rather than a regular pointer. That's because the layer itself may
  // be deleted, in which case, there will be noone kept holding the model.
  SmartPtr<LayerTableRowModel> m_Model;

  bool m_Selected;
  bool m_Hover;

  static QString m_SliderStyleSheetTemplate;

  // A popup menu
  QMenu *m_PopupMenu;

  // A submenu for the color maps
  QMenu *m_ColorMapMenu, *m_DisplayModeMenu, *m_OverlaysMenu;

  // Slider for opacity that lives in the menu
  QSlider *m_OverlayOpacitySlider;
  WidgetWithLabelAction *m_OverlayOpacitySliderAction;

  // An action group for the system presets
  QActionGroup* m_SystemPresetActionGroup, *m_DisplayModeActionGroup;

  void ApplyColorMap();
  void UpdateBackgroundPalette();
  void UpdateColorMapMenu();
  void UpdateComponentMenu();
  void UpdateOverlaysMenu();
  void OnNicknameUpdate();
};

#endif // LAYERINSPECTORROWDELEGATE_H
