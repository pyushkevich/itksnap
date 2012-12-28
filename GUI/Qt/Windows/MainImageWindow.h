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
class EventBucket;
class QModelIndex;
class QProgressDialog;

class LabelEditorDialog;
class LayerInspectorDialog;
class QtProgressReporterDelegate;
class ReorientImageDialog;

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


  void LoadRecent(QString file);
public slots:

  void AdjustMarginsForDocks();
  void onModelUpdate(const EventBucket &b);

private slots:
  void on_actionOpenGrey_triggered();

  void on_actionQuit_triggered();

  void on_actionLoad_from_Image_triggered();

  void on_actionAdd_Greyscale_Overlay_triggered();

  void on_actionOpen_RGB_Image_triggered();

  void on_actionAdd_RGB_Overlay_triggered();

  void on_actionImage_Contrast_triggered();

  void on_actionLabel_Editor_triggered();

  void on_actionRecent_1_triggered();
  void on_actionRecent_2_triggered();
  void on_actionRecent_3_triggered();
  void on_actionRecent_4_triggered();
  void on_actionRecent_5_triggered();

  void onSnakeWizardFinished();

  void on_listRecent_clicked(const QModelIndex &index);

  void on_actionUnload_All_triggered();


  void on_actionReorient_Image_triggered();

private:

  void UpdateRecentMenu();

  // For convenience, an array of the four panels (3 slice/1 3D)
  QWidget *m_ViewPanels[4];

  // Left and right docks
  QDockWidget *m_DockLeft, *m_DockRight;

  // Progress dialog
  QProgressDialog *m_Progress;

  // IRIS main toolbox (in left dock)
  IRISMainToolbox *m_Toolbox;

  // SNAP wizard panel (in right dock)
  SnakeWizardPanel *m_SnakeWizard;

  Ui::MainImageWindow *ui;

  GlobalUIModel *m_Model;

  LabelEditorDialog *m_LabelEditor;

  LayerInspectorDialog *m_LayerInspector;

  ReorientImageDialog *m_ReorientImageDialog;

  QtProgressReporterDelegate *m_ProgressReporterDelegate;
};

#endif // MAINIMAGEWINDOW_H
