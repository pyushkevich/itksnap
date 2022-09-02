#include "MultiFrameDicomSeriesSorter.h"

#include <sstream>
#include <exception>

#include "gdcmImageReader.h"
#include "gdcmElement.h"
#include "gdcmTag.h"
#include "gdcmStringFilter.h"
#include "gdcmIPPSorter.h"
#include "itksys/SystemTools.hxx"
#include "IRISException.h"

using namespace MFDS;

const gdcm::Tag tagInstanceNumber(0x0020,0x0013);
const gdcm::Tag tagSliceLocation(0x0020, 0x1041); // needed for CTA
const gdcm::Tag tagIPP(0x0020, 0x0032);
const gdcm::Tag tagIOP(0x0020, 0x0037);

//=========================================
// DicomFile Implementation
//=========================================

DicomFile
::DicomFile(std::string &fn)
{
	if (!itksys::SystemTools::FileExists(fn))
		throw IRISException("File \"%s\" does not exist", fn.c_str());

	this->m_Filename = fn;
	std::set<gdcm::Tag> tags{tagInstanceNumber, tagSliceLocation, tagIPP};
	gdcm::ImageReader reader;
	reader.SetFileName(fn.c_str());

	if (reader.ReadSelectedTags(tags))
		{
		gdcm::StringFilter strFilter;
		strFilter.SetFile(reader.GetFile());
		try
			{
			// parse IPP
			std::string value = strFilter.ToString(tagIPP);
			gdcm::Element<gdcm::VR::DS, gdcm::VM::VM3> eIPP;
			std::stringstream ssipp (value);
			eIPP.Read(ssipp);
			for (int i = 0; i < 3; ++i) m_IPP[i] = eIPP[i];

			// parse other fields
			this->m_SliceLocation = std::stod(strFilter.ToString(tagSliceLocation));
			this->m_InstanceNumber = std::stoi(strFilter.ToString(tagInstanceNumber));
			}
		catch (std::exception &e)
			{
			throw IRISException("Error parsing dicom file: %s. \n %s", fn.c_str(), e.what());
			}
		}
	else
		{
		throw IRISException("DicomFileInfo: gdcm::ImageReader failed to read the file: %s"
													, fn.c_str());
		}
}

DicomFile
::DicomFile(const DicomFile &other)
{
	this->m_Filename = other.m_Filename;
	for (int i = 0; i < 3; ++i) this->m_IPP[i] = other.m_IPP[i];
	this->m_InstanceNumber = other.m_InstanceNumber;
	this->m_SliceLocation = other.m_SliceLocation;
}

DicomFile &
DicomFile
::operator=(const DicomFile &other)
{
	this->m_Filename = other.m_Filename;
	for (int i = 0; i < 3; ++i) this->m_IPP[i] = other.m_IPP[i];
	this->m_InstanceNumber = other.m_InstanceNumber;
	this->m_SliceLocation = other.m_SliceLocation;
	return *this;
}

//=========================================
// MFGroupByIPP2Strategy Implementation
//=========================================
int
MFGroupByIPP2Strategy
::Apply()
{
	for (auto &df : m_DicomFilesContainer)
		{
		double ipp_z = df.m_IPP[2];
		if (m_SliceGroupingResult.count(ipp_z))
			m_SliceGroupingResult[ipp_z].push_back(df);
		else
			m_SliceGroupingResult[ipp_z] = DicomFilesList{ df };
		}

	return EXIT_SUCCESS;
}



//=================================================
// MFOrderByInstanceNumberStrategy Implementation
//=================================================
int
MFOrderByInstanceNumberStrategy
::Apply(DicomFilesList &df)
{
	std::sort(df.begin(), df.end(), DicomFile::CompareByInstanceNumber);
	return EXIT_SUCCESS;
}

//=========================================
// MFOrderByIPPStrategy Implementation
//=========================================
int
MFOrderByIPPStrategy
::Apply(DicomFilesList &dflist)
{
	std::vector<std::string> filenames;
	std::map<std::string, DicomFile> DicomFileToFilenameMap;

	for (auto &df : dflist)
		{
		filenames.push_back(df.m_Filename);
		DicomFileToFilenameMap.insert(std::pair<std::string, DicomFile>(df.m_Filename, df));
		}

	gdcm::IPPSorter ippSorter;
	if (ippSorter.Sort(filenames))
		{
		DicomFilesList sorted;

		for (auto &fn : filenames)
			sorted.push_back(DicomFileToFilenameMap.at(fn));

		// for some reason itk series reader always read image to RAI regardless of the order
		// so we are reversing the slice order here and
		// will flip it back from RAI to RAS after itk loading
		std::reverse(sorted.begin(), sorted.end());

		dflist.clear();
		dflist = sorted; // copy the sorted list back

		return EXIT_SUCCESS;
		}
	else
		return EXIT_FAILURE;
}

//===================================================
// MultiFrameDicomSeriesSorter Implementation
//===================================================
int
MultiFrameDicomSeriesSorter
::Sort()
{
	// the Sorter has to be initialized correctly
	assert(m_FilenamesList.size() > 0 && m_GroupingStrat
				 && m_FrameOrderingStrat && m_SliceOrderingStrat);

	this->InvokeEvent(itk::StartEvent());
	this->UpdateProgress(0.0);

	// build dicom file list
	for (auto &fn : m_FilenamesList)
		m_DicomFilesList.push_back(DicomFile(fn));

	// apply grouping strat
	m_GroupingStrat->SetInput(m_DicomFilesList);
	m_GroupingStrat->Apply();
	m_SliceGroupingResult = m_GroupingStrat->GetOutput();

	this->UpdateProgress(0.3);

	float frmOrdProgInc = 0.3 / m_SliceGroupingResult.size(); // frame ordering progress increment

	// apply frame ordering strat for each slice and populate the frame map
	for (auto &kv : m_SliceGroupingResult)
		{
		m_FrameOrderingStrat->Apply(kv.second);
		unsigned int crntFrame = 1;
		for (auto &df : kv.second)
			{
			if (m_FilesToFrameMap.count(crntFrame))
				m_FilesToFrameMap[crntFrame].push_back(df);
			else
				m_FilesToFrameMap[crntFrame] = DicomFilesList{ df };

			++crntFrame;
			}
		this->UpdateProgress(Superclass::GetProgress() + frmOrdProgInc);
		}

	float sliceOrdProgInc = 0.4 / m_FilesToFrameMap.size();
	// apply slice ordering strat for each frame
	for (auto &kv : m_FilesToFrameMap)
		{
		m_SliceOrderingStrat->Apply(kv.second);
		this->UpdateProgress(Superclass::GetProgress() + sliceOrdProgInc);
		}

	this->UpdateProgress(1.0);

	return EXIT_SUCCESS;
}

DicomFilenamesToFrameMap
MultiFrameDicomSeriesSorter
::GetFileNamesMap()
{
	DicomFilenamesToFrameMap ret;
	for (auto &kv : m_FilesToFrameMap)
		{
		FilenamesList fnlist;
		for (auto &df : kv.second)
			fnlist.push_back(df.m_Filename);

		ret[kv.first] = fnlist;
		}
	return ret;
}

