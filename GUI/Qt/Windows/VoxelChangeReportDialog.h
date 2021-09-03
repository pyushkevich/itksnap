#ifndef VOXELCHANGEREPORTDIALOG_H
#define VOXELCHANGEREPORTDIALOG_H

#include <QDialog>
#include <SNAPCommon.h>

class VoxelChangeReportModel;

namespace Ui {
  class VoxelChangeReportDialog;
}

class VoxelChangeReportDialog : public QDialog
{
  Q_OBJECT

public:
  explicit VoxelChangeReportDialog(QWidget *parent = nullptr);
  ~VoxelChangeReportDialog();

  /** Save voxel states before the change */
  void setStartPoint();

  /** Show change report after the change */
  void showReport();

  /** Set report description */
  void setDescription(QString des);

  /** Set Model */
  void SetModel(VoxelChangeReportModel *model);

private:
  Ui::VoxelChangeReportDialog *ui;

  SmartPtr<VoxelChangeReportModel> m_Model;
};

#endif // VOXELCHANGEREPORTDIALOG_H
