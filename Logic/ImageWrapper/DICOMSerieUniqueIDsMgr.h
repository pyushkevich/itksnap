
#ifndef __DICOMSerieUniqueIDsMgr_h_
#define __DICOMSerieUniqueIDsMgr_h_

#include "gdcmTag.h"
#include "gdcmSmartPointer.h"
#include "gdcmFile.h"
#include <vector>
#include <string>
#include <map>

namespace SNAP
{

enum CompOperators {
   DICOM_EQUAL = 0,
   DICOM_DIFFERENT,
   DICOM_GREATER,
   DICOM_GREATEROREQUAL,
   DICOM_LESS,
   DICOM_LESSOREQUAL
};

/**
 * \brief FileWithName
 *
 * \details
 * Backward only class do not use in newer code
 */
class FileWithName : public gdcm::File
{
public:
  FileWithName(File &f):File(f),filename(){}
  std::string filename;
};

typedef std::vector< gdcm::SmartPointer<FileWithName> > FileList;
typedef bool (*BOOL_FUNCTION_PFILE_PFILE_POINTER)(gdcm::File *, gdcm::File *);
class Scanner;

/**
 * \class DICOMSerieUniqueIDsMgr
 * \brief This class is a helper providing unique IDs for series
 * and is based on gdcm::SerieHelper which is deprecated.
 */
class DICOMSerieUniqueIDsMgr
{
public:
  DICOMSerieUniqueIDsMgr();
  ~DICOMSerieUniqueIDsMgr();

  void Clear();
  //void SetLoadMode (int ) {}
  void SetDirectory(std::string const &dir, bool recursive=false);

  void AddRestriction(const std::string & tag);
  void SetUseSeriesDetails( bool useSeriesDetails );
  void CreateDefaultUniqueSeriesIdentifier();
  FileList *GetFirstSingleSerieUIDFileSet();
  FileList *GetNextSingleSerieUIDFileSet();
  std::string CreateUniqueSeriesIdentifier( gdcm::File * inFile );
  //void OrderFileList(FileList *fileSet);
  void AddRestriction(uint16_t group, uint16_t elem, std::string const &value, int op);

protected:
  bool UserOrdering(FileList *fileSet);
  void AddFileName(std::string const &filename);
  bool AddFile(FileWithName &header);
  void AddRestriction(const gdcm::Tag& tag);
  //bool ImagePositionPatientOrdering(FileList *fileSet);
  //bool FileNameOrdering( FileList *fileList );

  typedef struct {
    uint16_t group;
    uint16_t elem;
    std::string value;
    int op;
  } Rule;
  typedef std::vector<Rule> SerieRestrictions;

  typedef std::map<std::string, FileList *> SingleSerieUIDFileSetmap;
  SingleSerieUIDFileSetmap SingleSerieUIDFileSetHT;
  SingleSerieUIDFileSetmap::iterator ItFileSetHt;

private:
  SerieRestrictions Restrictions;
  SerieRestrictions Refine;

  bool UseSeriesDetails;
  bool DirectOrder;

  BOOL_FUNCTION_PFILE_PFILE_POINTER UserLessThanFunction;

  bool m_UseSeriesDetails;
};

} // end namespace SNAP


#endif //__DICOMSerieUniqueIDsMgr_h_
