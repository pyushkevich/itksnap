#include "VoxelChangeReportModel.h"
#include "GlobalUIModel.h"
#include "IRISApplication.h"
#include "GenericImageData.h"

VoxelChangeReportModel::~VoxelChangeReportModel()
{
  this->ResetReportCache();
}

void VoxelChangeReportModel::SetParentModel(GlobalUIModel *parent)
{
  this->m_Parent = parent;
}

void VoxelChangeReportModel::setStartingPoint()
{
  this->PopulateReportCache(true);
}

VoxelChangeReportModel::VoxelChangeReportType&
VoxelChangeReportModel::getReport()
{
  this->PopulateReportCache(false);
  // this->PrintInfo();
  return this->m_ReportCache;
}

void VoxelChangeReportModel::PopulateReportCache(bool isStartingRun)
{
  // Clear old report data
  if (isStartingRun)
    this->ResetReportCache();

  // Iterate through frames to count label voxels
  LabelImageWrapper *liw = m_Parent->GetDriver()->GetSelectedSegmentationLayer();
  unsigned int dimT = liw->GetNumberOfTimePoints();
  SegmentationStatistics voxelCounter;

  typedef SegmentationStatistics::LabelVoxelCount CountResultType;
  CountResultType res;

  for (unsigned int i = 0; i < dimT; ++i)
    {
      res.clear();

      // Change current frame to target frame
      liw->SetTimePointIndex(i);

      // Count voxels
      voxelCounter.GetVoxelCount(res, m_Parent->GetDriver());

      // Get single voxel volume
      const double *spacing = liw->GetImageBase()->GetSpacing().GetDataPointer();
      const double volVoxel = spacing[0] * spacing[1] * spacing[2];

      // Initialize label voxel count entries
      LabelVoxelChangeType lvc;
      for (auto cit = res.cbegin(); cit != res.cend(); ++cit)
        {

          if (isStartingRun)
            {
              // Allocate a new VoexlChange
              VoxelChange *ch = new VoxelChange();
              ch->cnt_before = cit->second;
              ch->vol_before_mm3 = ch->cnt_before * volVoxel;
              lvc[cit->first] = ch;

              // Add current frame's counting result to report
              m_ReportCache[i] = lvc;
            }
          else
            {
              // Get existing VoxelChange
              VoxelChange *ech = m_ReportCache[i][cit->first];
              ech->cnt_after = cit->second;
              ech->vol_after_mm3 = ech->cnt_after * volVoxel;
              ech->vol_change_mm3 = ech->vol_after_mm3 - ech->vol_before_mm3;
              ech->vol_change_pct = ech->vol_change_mm3 / ech->vol_before_mm3;
              ech->cnt_change = ech->cnt_after - ech->cnt_before;
            }

        }
    }

  // Restore the time point to where the cursor is pointing
  liw->SetTimePointIndex(m_Parent->GetDriver()->GetCursorTimePoint());
  liw->Modified();
}



void VoxelChangeReportModel::ResetReportCache()
{
  for (auto fit = m_ReportCache.begin(); fit != m_ReportCache.end(); ++fit)
    {
      // For each frame, delete VoxelChange struct for each label
      for (auto lit = fit->second.begin(); lit != fit->second.end(); ++lit)
        {
          delete lit->second;
        }
      // Then clear the map
      fit->second.clear();
    }
  // Clear the report cache
  m_ReportCache.clear();
}

SmartPtr<ColorLabelTable>
VoxelChangeReportModel::GetColorLabelTable()
{
  return m_Parent->GetDriver()->GetColorLabelTable();
}

/*
VoxelChangeReportModel::VoxelChangeReportType&
VoxelChangeReportModel::getReport()
{
  // Get current voxel counts

  // Calculate delta
}
*/

void VoxelChangeReportModel::PrintInfo() const
{
  std::cout << "Voxel Change Report ======================" << std::endl;
  for (auto fit = m_ReportCache.cbegin(); fit != m_ReportCache.cend(); ++fit)
    {
      std::cout << "Frame: " << fit->first << "--------" << std::endl;
      for (auto lit = fit->second.cbegin(); lit != fit->second.cend(); ++lit)
        {
          std::cout << "<Label "<< lit->first << "> ";
          lit->second->PrintInfo();
        }
    }
}
