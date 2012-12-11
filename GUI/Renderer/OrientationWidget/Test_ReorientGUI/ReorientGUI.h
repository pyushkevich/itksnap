#ifndef REORIENT_GUI_H
#define REORIENT_GUI_H

#include <QMainWindow>


#include "ui_ReorientGUI.h"

#include "Reorient.h"

class vtkRenderer;
class vtkEventQtSlotConnect;
class vtkObject;
class vtkCommand;

class ReorientGUI : public QMainWindow, public Ui::MainWindow
{

  Reorient m_Reorient;

  Q_OBJECT
public:
  ReorientGUI();
  ~ReorientGUI();

  vtkSmartPointer < vtkMatrix4x4 > getMtrx4x4GUI();

public slots:
  void slotReorient(int anDummyRow, int anDummyCol);
  void slotPhiThetaPsi(double adbValue);
  void slotSelectNegativeOrientation(bool abInterpretNegativeOrientation3x3);
};

#endif //REORIENT_GUI_H