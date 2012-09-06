/*=========================================================================

  Program:   ITK-SNAP
  Module:    $RCSfile: Filename.cxx,v $
  Language:  C++
  Date:      $Date: 2010/10/18 11:25:44 $
  Version:   $Revision: 1.12 $
  Copyright (c) 2011 Paul A. Yushkevich

  This file is part of ITK-SNAP

  ITK-SNAP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/

#ifndef MAINIMAGEWINDOW_H
#define MAINIMAGEWINDOW_H

#include <QMainWindow>
#include "SNAPCommon.h"

class GenericSliceView;
class SliceViewPanel;
class GlobalUIModel;
class QDockWidget;
class SnakeWizardPanel;
class IRISMainToolbox;

class LabelEditorDialog;
class LayerInspectorDialog;

namespace Ui {
class MainImageWindow;
}

class MainImageWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainImageWindow(QWidget *parent = 0);
  ~MainImageWindow();

  SliceViewPanel *GetSlicePanel(unsigned int i);

  // Initialize with a model
  void Initialize(GlobalUIModel *model);

  // Get model
  irisGetMacro(Model, GlobalUIModel *)

  // Initiate active contour segmentation
  void OpenSnakeWizard();

public slots:

  void AdjustMarginsForDocks();

private slots:
  void on_actionOpen_Greyscale_Image_triggered();

  void on_actionQuit_triggered();

  void on_actionLoad_from_Image_triggered();

  void on_actionAdd_Greyscale_Overlay_triggered();

  void on_actionOpen_RGB_Image_triggered();

  void on_actionAdd_RGB_Overlay_triggered();

  void on_actionImage_Contrast_triggered();

  void on_actionLabel_Editor_triggered();

private:

  // Left and right docks
  QDockWidget *m_DockLeft, *m_DockRight;

  // IRIS main toolbox (in left dock)
  IRISMainToolbox *m_Toolbox;

  // SNAP wizard panel (in right dock)
  SnakeWizardPanel *m_SnakeWizard;

  Ui::MainImageWindow *ui;

  GlobalUIModel *m_Model;

  LabelEditorDialog *m_LabelEditor;

  LayerInspectorDialog *m_LayerInspector;
};

#endif // MAINIMAGEWINDOW_H
