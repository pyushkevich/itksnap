#ifndef VOXELCHANGEREPORTMODEL_H
#define VOXELCHANGEREPORTMODEL_H

#include <AbstractModel.h>
#include "SegmentationStatistics.h"

class GlobalUIModel;

class VoxelChangeReportModel : public AbstractModel
{
public:
  irisITKObjectMacro(VoxelChangeReportModel, AbstractModel);

  /** Struct storing voxel count change */
  struct VoxelChange
  {
    unsigned long cnt_before = 0;
    unsigned long cnt_after = 0;
    double vol_before_mm3 = 0.0;
    double vol_after_mm3 = 0.0;
    double vol_change_pct = 0.0;
    double vol_change_mm3 = 0.0;
    long long cnt_change = 0; // use long long for change < -2,147,483,678 (lower limit of long)
    /** Constructor */
    VoxelChange() {}

    /** Print Info */
    void PrintInfo() const
    {
      std::cout << "cnt_before: " << cnt_before << ", "
                << "cnt_after: " << cnt_after << ", "
                << "vol_before_mm3: " << vol_before_mm3 << ", "
                << "vol_after_mm3: " << vol_after_mm3 << ", "
                << "vol_change_mm3: " << vol_change_mm3 << ", "
                << "vol_change_pct: " << vol_change_pct << ", "
                << "change: " << cnt_change << std::endl;
    }
  };

  typedef unsigned int TimePointType;
  typedef std::map<LabelType, VoxelChange*> LabelVoxelChangeType;
  typedef std::map<TimePointType, LabelVoxelChangeType> VoxelChangeReportType;

  /** Save the states of voxels before the change*/
  void setStartingPoint();

  /** Calculate the change of voxels and return result to the UI */
  VoxelChangeReportType& getReport();

  /** Set Parent Model */
  void SetParentModel(GlobalUIModel *parent);

  /** Get Color Label */
  SmartPtr<ColorLabelTable> GetColorLabelTable();

  /** Debug Print Info */
  void PrintInfo() const;

protected:
  /** Constructor */
  VoxelChangeReportModel() {};
  ~VoxelChangeReportModel();

  /** Clear report cache */
  void ResetReportCache();

  /** Populate report cache */
  void PopulateReportCache(bool isStartingRun);

  /** Report cache */
  VoxelChangeReportType m_ReportCache;

  /** A pointer to parent model */
  GlobalUIModel *m_Parent;
};

#endif // VOXELCHANGEREPORTMODEL_H
