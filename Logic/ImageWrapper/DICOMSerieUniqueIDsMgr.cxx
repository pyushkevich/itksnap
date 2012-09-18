#include "gdcmStringFilter.h"
#include "gdcmDirectory.h"
#include "gdcmIPPSorter.h"
#include "gdcmImageReader.h"
#include "gdcmImageHelper.h"
#include "gdcmTrace.h"

#include "DICOMSerieUniqueIDsMgr.h"

using namespace std;

namespace SNAP
{

DICOMSerieUniqueIDsMgr::DICOMSerieUniqueIDsMgr()
{
  //gdcm::Trace::WarningOff();
  m_UseSeriesDetails = false;
  Clear();
  UserLessThanFunction = 0;
  DirectOrder = true;
  //LoadMode = 0;
}

DICOMSerieUniqueIDsMgr::~DICOMSerieUniqueIDsMgr()
{
  Clear();
}

void DICOMSerieUniqueIDsMgr::AddRestriction(uint16_t group, uint16_t elem, string const &value, int op)
{
  Rule r;
  r.group = group;
  r.elem  = elem;
  r.value = value;
  r.op    = op;
  Restrictions.push_back( r );
}

void DICOMSerieUniqueIDsMgr::AddRestriction(const string & tag)
{
  gdcm::Tag t;
  t.ReadFromPipeSeparatedString(tag.c_str());
  AddRestriction( gdcm::Tag(t.GetGroup(), t.GetElement()) );
}

void DICOMSerieUniqueIDsMgr::AddRestriction(const gdcm::Tag& tag)
{
  Rule r;
  r.group = tag.GetGroup();
  r.elem  = tag.GetElement();
  Refine.push_back( r );
}

void DICOMSerieUniqueIDsMgr::SetUseSeriesDetails( bool useSeriesDetails )
{
  UseSeriesDetails = useSeriesDetails;
}

void DICOMSerieUniqueIDsMgr::CreateDefaultUniqueSeriesIdentifier()
{
  // If the user requests, additional information can be appended
  // to the SeriesUID to further differentiate volumes in the DICOM
  // objects being processed.

  // 0020 0011 Series Number
  // A scout scan prior to a CT volume scan can share the same
  //   SeriesUID, but they will sometimes have a different Series Number
  AddRestriction( gdcm::Tag(0x0020, 0x0011) );
  // 0018 0024 Sequence Name
  // For T1-map and phase-contrast MRA, the different flip angles and
  //   directions are only distinguished by the Sequence Name
  AddRestriction( gdcm::Tag(0x0018, 0x0024) );
  // 0018 0050 Slice Thickness
  // On some CT systems, scout scans and subsequence volume scans will
  //   have the same SeriesUID and Series Number - YET the slice
  //   thickness will differ from the scout slice and the volume slices.
  AddRestriction( gdcm::Tag(0x0018, 0x0050) );
  // 0028 0010 Rows
  // If the 2D images in a sequence don't have the same number of rows,
  // then it is difficult to reconstruct them into a 3D volume.
  AddRestriction( gdcm::Tag(0x0028, 0x0010));
  // 0028 0011 Columns
  // If the 2D images in a sequence don't have the same number of columns,
  // then it is difficult to reconstruct them into a 3D volume.
  AddRestriction( gdcm::Tag(0x0028, 0x0011));
}

void DICOMSerieUniqueIDsMgr::Clear()
{
  // For all the 'Single SerieUID' Filesets that may already exist
  FileList *l = GetFirstSingleSerieUIDFileSet();
  while (l)
    {
    // For all the gdcm::File of a File set
    for (FileList::iterator it  = l->begin();
      it != l->end();
      ++it)
      {
      //delete *it; // remove each entry
      }
    l->clear();
    delete l;     // remove the container
    l = GetNextSingleSerieUIDFileSet();
    }
  // Need to clear that too:
  SingleSerieUIDFileSetHT.clear();
}

void DICOMSerieUniqueIDsMgr::SetDirectory(string const &dir, bool recursive)
{
  gdcm::Directory dirList;
  unsigned int nfiles = dirList.Load(dir, recursive); (void)nfiles;

  gdcm::Directory::FilenamesType const &filenames = dirList.GetFilenames();
  for( gdcm::Directory::FilenamesType::const_iterator it = filenames.begin();
    it != filenames.end(); ++it)
    {
    AddFileName( *it );
    }
}

void DICOMSerieUniqueIDsMgr::AddFileName(string const &filename)
{
  // Only accept DICOM file containing Image (Pixel Data element):
  gdcm::ImageReader reader;
  reader.SetFileName( filename.c_str() );
  if( !reader.Read() )
    {
    gdcmWarningMacro("Could not read file: " << filename );
    }
  else
    {
    gdcm::SmartPointer<FileWithName> f = new FileWithName( reader.GetFile() );
    f->filename = filename;
    (void)AddFile( *f /*reader.GetFile()*/ ); // discard return value
    }
}

bool CompareDicomString(const string &s1, const char *s2, int op)
{
  // s2 is the string from the DICOM reference e.g. : 'MONOCHROME1'
  string s1_even = s1; //Never change input parameter
  string s2_even = /*DicomString(*/ s2 ;
  assert( s2_even.size() % 2 == 0 );
  if ( s1_even[s1_even.size()-1] == ' ' )
    {
    s1_even[s1_even.size()-1] = '\0'; //replace space character by null
    }
  switch (op)
    {
  case DICOM_EQUAL :
    return s1_even == s2_even;
  case DICOM_DIFFERENT :
    return s1_even != s2_even;
  case DICOM_GREATER :
    return s1_even >  s2_even;
  case DICOM_GREATEROREQUAL :
    return s1_even >= s2_even;
  case DICOM_LESS :
    return s1_even <  s2_even;
  case DICOM_LESSOREQUAL :
    return s1_even <= s2_even;
  default :
    gdcmDebugMacro(" Wrong operator : " << op);
    return false;
    }
}
bool DICOMSerieUniqueIDsMgr::AddFile(FileWithName &header)
{
  gdcm::StringFilter sf;
  sf.SetFile( header );
  int allrules = 1;
  // First step the user has defined a set of rules for the DICOM
  // he is looking for.
  // make sure the file correspond to his set of rules:

  string s;
  for(SerieRestrictions::iterator it2 = Restrictions.begin();
    it2 != Restrictions.end();
    ++it2)
    {
    const Rule &r = *it2;
    //s = header->GetEntryValue( r.group, r.elem );
      s = sf.ToString( gdcm::Tag(r.group,r.elem) );
    if ( !CompareDicomString(s, r.value.c_str(), r.op) )
      {
      // Argh ! This rule is unmatched; let's just quit
      allrules = 0;
      break;
      }
    }

  if ( allrules ) // all rules are respected:
    {
    // Allright! we have a found a DICOM that matches the user expectation.
    // Let's add it to the specific 'id' which by default is uid (Serie UID)
    // but can be `refined` by user with more paramater (see AddRestriction(g,e))

    string id = CreateUniqueSeriesIdentifier( &header );
    // if id == GDCM_UNFOUND then consistently we should find GDCM_UNFOUND
    // no need here to do anything special

    if ( SingleSerieUIDFileSetHT.count(id) == 0 )
      {
      gdcmDebugMacro(" New Serie UID :[" << id << "]");
      // create a list in 'id' position
      SingleSerieUIDFileSetHT[id] = new FileList;
      }
    // Current Serie UID and DICOM header seems to match add the file:
    SingleSerieUIDFileSetHT[id]->push_back( header );
    }
  else
    {
    // one rule not matched, tell user:
    return false;
    }
  return true;
}

FileList *DICOMSerieUniqueIDsMgr::GetFirstSingleSerieUIDFileSet()
{
  ItFileSetHt = SingleSerieUIDFileSetHT.begin();
  if ( ItFileSetHt != SingleSerieUIDFileSetHT.end() )
    return ItFileSetHt->second;
  return NULL;
}

FileList *DICOMSerieUniqueIDsMgr::GetNextSingleSerieUIDFileSet()
{

  ++ItFileSetHt;
  if ( ItFileSetHt != SingleSerieUIDFileSetHT.end() )
    return ItFileSetHt->second;
  return NULL;
}

bool DICOMSerieUniqueIDsMgr::UserOrdering(FileList *fileList)
{
  sort(fileList->begin(), fileList->end(), //SerieHelper::
            UserLessThanFunction);
  if (!DirectOrder)
    {
    reverse(fileList->begin(), fileList->end());
    }
  return true;
}

#if 0

  namespace details {
bool MyFileNameSortPredicate(const gdcm::SmartPointer<FileWithName>& d1, const gdcm::SmartPointer<FileWithName>& d2)
{
  return d1->filename < d2->filename;
}
}

bool DICOMSerieUniqueIDsMgr::FileNameOrdering( FileList *fileList )
{
  sort(fileList->begin(), fileList->end(), details::MyFileNameSortPredicate);

  return true;
}
  
bool DICOMSerieUniqueIDsMgr::ImagePositionPatientOrdering( FileList *fileList )
{
  //iop is calculated based on the file file
  vector<double> cosines;
  double normal[3] = {};
  vector<double> ipp;
  double dist;
  double min = 0, max = 0;
  bool first = true;

  multimap<double,gdcm::SmartPointer<FileWithName> > distmultimap;
  // Use a multimap to sort the distances from 0,0,0
  for ( FileList::const_iterator
    it = fileList->begin();
    it != fileList->end(); ++it )
    {
    if ( first )
      {
      //(*it)->GetImageOrientationPatient( cosines );
      cosines = gdcm::ImageHelper::GetDirectionCosinesValue( **it );

      // You only have to do this once for all slices in the volume. Next,
      // for each slice, calculate the distance along the slice normal
      // using the IPP ("Image Position Patient") tag.
      // ("dist" is initialized to zero before reading the first slice) :
      normal[0] = cosines[1]*cosines[5] - cosines[2]*cosines[4];
      normal[1] = cosines[2]*cosines[3] - cosines[0]*cosines[5];
      normal[2] = cosines[0]*cosines[4] - cosines[1]*cosines[3];

      ipp = gdcm::ImageHelper::GetOriginValue( **it );
      //ipp[0] = (*it)->GetXOrigin();
      //ipp[1] = (*it)->GetYOrigin();
      //ipp[2] = (*it)->GetZOrigin();

      dist = 0;
      for ( int i = 0; i < 3; ++i )
        {
        dist += normal[i]*ipp[i];
        }

        distmultimap.insert(pair<const double,gdcm::SmartPointer<FileWithName> >(dist, *it));

      max = min = dist;
      first = false;
      }
    else
      {
      ipp = gdcm::ImageHelper::GetOriginValue( **it );
      //ipp[0] = (*it)->GetXOrigin();
      //ipp[1] = (*it)->GetYOrigin();
      //ipp[2] = (*it)->GetZOrigin();

      dist = 0;
      for ( int i = 0; i < 3; ++i )
        {
        dist += normal[i]*ipp[i];
        }

      distmultimap.insert(pair<const double,gdcm::SmartPointer<FileWithName> >(dist, *it));

      min = (min < dist) ? min : dist;
      max = (max > dist) ? max : dist;
      }
    }

  // Find out if min/max are coherent
  if ( min == max )
    {
    gdcmWarningMacro("Looks like all images have the exact same image position"
      << ". No PositionPatientOrdering sort performed" );
    return false;
    }

  // Check to see if image shares a common position
  bool ok = true;
  for (multimap<double, gdcm::SmartPointer<FileWithName> >::iterator it2 = distmultimap.begin();
    it2 != distmultimap.end();
    ++it2)
    {
    if (distmultimap.count((*it2).first) != 1)
      {
      gdcmErrorMacro("File: "
        //<< ((*it2).second->GetFileName())
        << " Distance: "
        << (*it2).first
        << " position is not unique");

      ok = false;
      }
    }
  if (!ok)
    {
    return false;
    }

  fileList->clear();  // doesn't delete list elements, only nodes

  if (DirectOrder)
    {
    for (multimap<double, gdcm::SmartPointer<FileWithName> >::iterator it3 = distmultimap.begin();
      it3 != distmultimap.end();
      ++it3)
      {
      fileList->push_back( (*it3).second );
      }
    }
  else // user asked for reverse order
    {
    multimap<double, gdcm::SmartPointer<FileWithName> >::const_iterator it4;
    it4 = distmultimap.end();
    do
      {
      it4--;
      fileList->push_back( (*it4).second );
      } while (it4 != distmultimap.begin() );
    }

  distmultimap.clear();

  return true;
}


void DICOMSerieUniqueIDsMgr::OrderFileList(FileList *fileSet)
{
  gdcm::IPPSorter ipps;
  if ( DICOMSerieUniqueIDsMgr::UserLessThanFunction )
    {
    UserOrdering( fileSet );
    return;
    }
  else if ( ImagePositionPatientOrdering( fileSet ) )
    {
    return ;
    }
  /*
  else if ( ImageNumberOrdering(fileSet ) )
  {
  return ;
  }*/
  else
  {
  FileNameOrdering(fileSet );
  }
}

#endif //0

string DICOMSerieUniqueIDsMgr::CreateUniqueSeriesIdentifier( gdcm::File * inFile )
{
  gdcm::StringFilter sf;
  sf.SetFile( *inFile );
  if( true /*inFile->IsReadable()*/ )
    {
    // 0020 000e UI REL Series Instance UID
    string uid = sf.ToString( gdcm::Tag(0x0020, 0x000e) );
    string id = uid.c_str();
    if(m_UseSeriesDetails)
      {
      for(SerieRestrictions::iterator it2 = Refine.begin();
        it2 != Refine.end();
        ++it2)
        {
        const Rule &r = *it2;
        string s = sf.ToString( gdcm::Tag(r.group, r.elem) );
        //if( s == gdcm::GDCM_UNFOUND )
        //  {
        //  s = "";
        //  }
        if( id == uid && !s.empty() )
          {
          id += "."; // add separator
          }
        id += s;
        }
      }
    // Eliminate non-alnum characters, including whitespace...
    //   that may have been introduced by concats.
    for(size_t i=0; i<id.size(); i++)
      {
      while(i<id.size()
        && !( id[i] == '.'
          || (id[i] >= 'a' && id[i] <= 'z')
          || (id[i] >= '0' && id[i] <= '9')
          || (id[i] >= 'A' && id[i] <= 'Z')))
        {
        id.erase(i, 1);
        }
      }
    return id;
    }
  else // Could not open inFile
    {
    cerr << "Could not parse series info." << endl;
    string id = "GDCM_UNFOUND"; //gdcm::GDCM_UNFOUND;
    return id;
    }
}

} // end namespace gdcm
