#ifndef ORIENTATION_WIDGET_GUI_H
#define ORIENTATION_WIDGET_GUI_H

#include <QMainWindow>


#include "ui_OrientationWidgetGUI.h"

class vtkRenderer;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkCommand;
class vtkMatrix4x4;
#include <vtkSmartPointer.h>

class OrientationGraphicRenderer;
#include "ReorientProps.h"

class OrientationWidgetGUI : public QMainWindow, public Ui::MainWindow
{

  SmartPtr< OrientationGraphicRenderer > m_pRAIRenderer;

  ReorientProps m_ReorientProps;

  Q_OBJECT
public:
  OrientationWidgetGUI();
  ~OrientationWidgetGUI();

  vtkSmartPointer < vtkMatrix4x4 > getMtrx4x4GUI();

public slots:
  void slotReorient(int anDummyRow, int anDummyCol);
  void slotPhiThetaPsi(double adbValue);
  void slotSelectNegativeOrientation(bool abInterpretNegativeOrientation3x3);
};

#endif //ORIENTATION_WIDGET_GUI_H
