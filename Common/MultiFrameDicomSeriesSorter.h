#ifndef MULTIFRAMEDICOMSERIESSORTER_H
#define MULTIFRAMEDICOMSERIESSORTER_H

#include <map>
#include <vector>
#include <string>
#include "SNAPCommon.h"
#include "itkObject.h"
#include "itkProcessObject.h"
#include "itkObjectFactory.h"

namespace MFDS //Multi-Frame Dicom Series
{

struct DicomFile
{
  DicomFile()=delete; // default constructor should never be needed
  ~DicomFile() {}

	DicomFile(std::string &fn); // only use this constructor
  DicomFile(const DicomFile &other);
  DicomFile &operator=(const DicomFile &other);

	// sorting functions
	static bool CompareByInstanceNumber(const DicomFile &left, const DicomFile &right)
	{ return left.m_InstanceNumber < right.m_InstanceNumber; }

  // members
  std::string m_Filename;
	int m_InstanceNumber;
  double m_IPP[3];
  double m_SliceLocation; // needed for 4DCTA sorting
};

typedef std::vector<std::string> FilenamesList;
typedef std::vector<DicomFile> DicomFilesList;
typedef std::map<unsigned int, DicomFilesList> DicomFilesToFrameMap;
typedef std::map<unsigned int, FilenamesList> DicomFilenamesToFrameMap;
typedef double SliceIDType;
typedef std::map<SliceIDType, DicomFilesList> SliceGroupingResult;

/**
 * @brief The MFGroupByStrategyBase class
 * Group slices by specified tag(s) that should be defined in subclasses
 */
class MFGroupByStrategyBase : public itk::Object
{
public:
	irisITKAbstractObjectMacro(MFGroupByStrategyBase, itk::Object)

	/** Take a list of dicom files as input */
	virtual void SetInput(DicomFilesList &input)
	{ m_DicomFilesContainer = input; }

	/** The grouping logic should be implemented here */
	virtual int Apply() = 0;

	/** Return a dicom file to frame map as output */
	virtual SliceGroupingResult &GetOutput()
	{ return this->m_SliceGroupingResult; }

protected:
	MFGroupByStrategyBase() {}
	virtual ~MFGroupByStrategyBase() {}

	DicomFilesList m_DicomFilesContainer;
	SliceGroupingResult m_SliceGroupingResult;
};

/**
 * @brief The MFGroupByIPP2Strategy class
 * Group slices by Image Position Patient [2], i.e. Z-axis position
 * For time-series multi-frame image, this will put all time point slices
 * belong to the same z-axis slice into one group.
 */
class MFGroupByIPP2Strategy : public MFGroupByStrategyBase
{
public:
	irisITKObjectMacro(MFGroupByIPP2Strategy, MFGroupByStrategyBase)

	virtual int Apply() override;
protected:
	MFGroupByIPP2Strategy() {}
	virtual ~MFGroupByIPP2Strategy() {}

};

/**
 * @brief The MFOrderByStrategyBase class
 * Order slices based on specified tag(s) that should be defined in subclasses
 */
class MFOrderByStrategyBase : public itk::Object
{
public:
	irisITKAbstractObjectMacro(MFOrderByStrategyBase, itk::Object)

	/** The in-place sorting logic should be implemented here */
	virtual int Apply(DicomFilesList & dfmap) = 0;

protected:
	MFOrderByStrategyBase() {}
	virtual ~MFOrderByStrategyBase() {}
};

/**
 * @brief The MFOrderByInstanceNumberStrategy class
 * Order slices by Instance Number
 */
class MFOrderByInstanceNumberStrategy : public MFOrderByStrategyBase
{
public:
	irisITKObjectMacro(MFOrderByInstanceNumberStrategy, MFOrderByStrategyBase)

	/** The in-place sorting logic should be implemented here */
	virtual int Apply(DicomFilesList & dfmap) override;

protected:
	MFOrderByInstanceNumberStrategy() {}
	virtual ~MFOrderByInstanceNumberStrategy() {}
};


/**
 * @brief The MFOrderByIPPStrategy class
 * Order slices by Image Position Patient
 */
class MFOrderByIPPStrategy : public MFOrderByStrategyBase
{
public:
	irisITKObjectMacro(MFOrderByIPPStrategy, MFOrderByStrategyBase)

	/** The in-place sorting logic should be implemented here */
	virtual int Apply(DicomFilesList & dfmap) override;

protected:
	MFOrderByIPPStrategy() {}
	virtual ~MFOrderByIPPStrategy() {}
};

/**
 * @brief The MultiVolumeDicomSeriesSorter class
 * Take a list of filenames and returns the sorted result grouped by
 * volume_id
 */
class MultiFrameDicomSeriesSorter : public itk::ProcessObject
{
public:
	irisITKObjectMacro(MultiFrameDicomSeriesSorter, itk::ProcessObject)

	void SetFileNameList(FilenamesList &input) { m_FilenamesList = input; }

	void SetGroupingStrategy(MFGroupByStrategyBase *gs)
	{ this->m_GroupingStrat = gs; }

	void SetSliceOrderingStrategy(MFOrderByStrategyBase *sos)
	{ this->m_SliceOrderingStrat = sos; }

	void SetFrameOrderingStrategy(MFOrderByStrategyBase *fos)
	{ this->m_FrameOrderingStrat = fos; }

	int Sort();

	DicomFilesToFrameMap &GetOutput()
	{ return this->m_FilesToFrameMap; }

	DicomFilenamesToFrameMap GetFileNamesMap();


protected:
    MultiFrameDicomSeriesSorter() {}
  ~MultiFrameDicomSeriesSorter() {}

	FilenamesList m_FilenamesList;
	DicomFilesList m_DicomFilesList;
	DicomFilesToFrameMap m_FilesToFrameMap;
	SliceGroupingResult m_SliceGroupingResult;

	// strategy grouping dicom files into frames
	SmartPtr<MFGroupByStrategyBase> m_GroupingStrat;
	// strategy ordering frames for each slice
	SmartPtr<MFOrderByStrategyBase> m_FrameOrderingStrat;
	// strategy ordering slice for each frame
	SmartPtr<MFOrderByStrategyBase> m_SliceOrderingStrat;
};


} // namespace MFDS

#endif // MULTIFRAMEDICOMSERIESSORTER_H
