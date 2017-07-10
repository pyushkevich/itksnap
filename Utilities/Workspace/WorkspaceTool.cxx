/*=========================================================================

  Program:   ITK-SNAP
  Language:  C++
  Copyright (c) 2017 Paul A. Yushkevich
  
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

  -----

  Copyright (c) 2003 Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notices for more information. 

=========================================================================*/
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstdarg>
#include "CommandLineHelper.h"
#include "Registry.h"
#include "GuidedNativeImageIO.h"
#include "itksys/MD5.h"
#include "itksys/Directory.hxx"
#include "IRISException.h"

using namespace std;

int usage(int rc) 
{
  cout << "itksnap_ws : ITK-SNAP Workspace Tool" << endl;
  cout << "Usage: " << endl;
  cout << "  itksnap_ws [commands]" << endl;
  cout << "I/O commands: " << endl;
  cout << "  -i <workspace>                    : Read workspace file" << endl;
  cout << "  -a <outfile.zip>                  : Package workspace into an archive" << endl;
  cout << "Informational commands: " << endl;
  cout << "  -dump                             : Dump workspace in human-readable format" << endl;
  cout << "  -get <key>                        : Get the value of a specified key" << endl;
  cout << "  -list-layers                      : List all the layers in the workspace" << endl;
  cout << "Tag assignment commands: " << endl;
  cout << "  -add-tag <key> <tag>              : Add a tag to the folder 'key'" << endl;
  cout << "  -add-tag-excl <key> <tag>         : Add a tag that is exclusive to the folder 'key'" << endl;
  cout << "Distributed segmentation server commands: " << endl;
  cout << "  -dss-auth <url> [user] [passwd]   : Sign in to the server. This will create a token" << endl;
  cout << "                                      that may be used in future -dss calls" << endl;
  cout << "  -dss-services-list                : List all available segmentation services" << endl;
  cout << "  -dss-tickets-create <service>     : Create a new ticket using current workspace" << endl;
  cout << "  -dss-tickets-list                 : List all of your tickets" << endl;
  return rc;
}

/**
 * A helper class for formatting tables. You just specify the number of columns
 * and then add elements with the << operator (header and data elements are 
 * added the same way). Then pipe it to cout
 */
class FormattedTable
{
public:
  FormattedTable(int n_col, bool header)
    : m_Width(n_col, 0), m_Columns(n_col), m_Header(header), m_CurrentColumn(0) {}

  template <class TAtomic> FormattedTable& operator << (const TAtomic &datum)
    {
    ostringstream oss;
    oss << datum;
    int w = oss.str().length();
    if(m_Width[m_CurrentColumn] < w)
      m_Width[m_CurrentColumn] = w;
    m_Data.push_back(oss.str());
    m_CurrentColumn = (m_CurrentColumn + 1) % m_Columns;
    return *this;
    }

  void Print(ostream &os)
    {
    int iCol = 0;
    for(list<string>::const_iterator it = m_Data.begin(); it != m_Data.end(); ++it)
      {
      if(iCol == 0)
        os << left;

      os << setw(m_Width[iCol] + 2) << *it;

      iCol = (iCol + 1) % m_Columns;

      if(iCol == 0 || it == m_Data.end())
        os << endl;
      }
    }

protected:
  list<string> m_Data;
  vector<int> m_Width;
  int m_Columns, m_CurrentColumn;
  bool m_Header;
};

/**
 * This class encapsulates an ITK-SNAP workspace. It is just a wrapper around 
 * a registry object, but with extra functions that support workspaces
 */
class Workspace
{
public:

  /**
   * Read the workspace from a file, determine if it has been moved or copied
   * since it was saved originally.
   */
  void ReadFromXMLFile(const char *proj_file)
    {
    // Read the contents of the project from the file
    m_Registry.ReadFromXMLFile(proj_file);

    // Get the full name of the project file
    m_WorkspaceFilePath = itksys::SystemTools::CollapseFullPath(proj_file);

    // Get the directory in which the project will be saved
    m_WorkspaceFileDir = itksys::SystemTools::GetParentDirectory(m_WorkspaceFilePath);

    // Read the location where the file was saved initially
    m_WorkspaceSavedDir = m_Registry["SaveLocation"][""];

    // If the locations are different, we will attempt to find relative paths first
    m_Moved = (m_WorkspaceFileDir != m_WorkspaceSavedDir);
    }

  /**
   * Write the workspace to an XML file. The workspace data structure
   * is updated to reflect the new file (i.e., this is a SaveAs pattern).
   *
   * This method assumes that all the referenced image paths are correct. It does
   * not automatically try to account for a moved workspace
   *
   * You probably want to call FixMovedWorkspace() before doing SaveAs, since when the
   * Workspace is loaded, the paths may no longer be valid
   */
  void SaveAsXMLFile(const char *proj_file)
    {
    // Get the full name of the project file
    string proj_file_full = itksys::SystemTools::CollapseFullPath(proj_file);

    // Get the directory in which the project will be saved
    string project_dir = itksys::SystemTools::GetParentDirectory(proj_file_full.c_str());

    // Update the save location
    m_Registry["SaveLocation"] << project_dir;

    // Write the registry
    m_Registry.WriteToXMLFile(proj_file_full.c_str());

    // Update the internal values
    m_Moved = false;
    m_WorkspaceSavedDir = project_dir;
    m_WorkspaceFileDir = project_dir;
    m_WorkspaceFilePath = proj_file_full;
    }

  /**
   * Get number of image layers in the workspace
   */
  int GetNumberOfLayers()
    {
    // Unfortunately we have to count the folders each time we want to return the number
    // of layers. This is the unfortunate consequence of the lame way in which Registry
    // supports arrays of folders.
    int n_layers = 0;
    while(m_Registry.HasFolder(Registry::Key("Layers.Layer[%03d]", n_layers)))
      n_layers++;

    return n_layers;
    }

  /**
   * Get the folder for the n-th layer
   */
  Registry &GetLayerFolder(int layer_index)
    {
    string key = Registry::Key("Layers.Layer[%03d]", layer_index);
    return m_Registry.Folder(key);
    }

  /**
   * Find a physical file corresponding to a file referenced by the project, accouting
   * for the possibility that the project may have been moved or copied
   */
  string GetLayerActualPath(Registry &folder)
    {
    // Get the filenames for the layer
    string layer_file_full = folder["AbsolutePath"][""];

    // If the project has moved, try finding a relative location
    if(m_Moved)
      {
      string relative_path = itksys::SystemTools::RelativePath(
            m_WorkspaceSavedDir.c_str(), layer_file_full.c_str());

      string moved_file_full = itksys::SystemTools::CollapseFullPath(
            relative_path.c_str(), m_WorkspaceFileDir.c_str());

      if(itksys::SystemTools::FileExists(moved_file_full.c_str(), true))
        layer_file_full = moved_file_full;
      }

    // Return the file - no guarantee that it exists...
    return layer_file_full;
    }

  /**
   * Get the IO hints for the layer, or NULL pointer if there are no IO hints stored
   */
  Registry * GetLayerIOHints(Registry &folder)
    {
    // Load the IO hints for the image from the project - but only if this
    // folder is actually present (otherwise some projects from before 2016
    // will not load hints)
    Registry *io_hints = NULL;
    if(folder.HasFolder("IOHints"))
      io_hints = &folder.Folder("IOHints");
    return io_hints;
    }

  void PrintLayerList(std::ostream &os)
    {
    // Iterate over all the layers stored in the workspace
    int n_layers = this->GetNumberOfLayers();

    // Use a formatted table
    FormattedTable table(5, true);

    // Print the header information
    table << "Layer" << "Role" << "Nickname" << "Filename" << "Tags";

    // Load all of the layers in the current project
    for(int i = 0; i < n_layers; i++)
      {
      // Get the folder corresponding to the layer
      Registry &f_layer = this->GetLayerFolder(i);

      // Get the role 
      string role = f_layer["Role"][""];
      role = role.substr(0,role.find("Role"));

      // Use the %03d formatting for layer numbers, to match that in the registry
      char ifmt[16];
      sprintf(ifmt, "%03d", i);

      // Print the layer information
      table 
        << ifmt 
        << role 
        << f_layer["LayerMetaData.CustomNickName"][""] 
        << this->GetLayerActualPath(f_layer)
        << f_layer["Tags"][""];
      }

    table.Print(os);
    }

  /** Get a list of tags from a particular folder */
  set<string> GetTags(Registry &folder)
    {
    istringstream iss(folder["Tags"][""]);
    string tag;
    set<string> tagset;
    while(getline(iss,tag,','))
      tagset.insert(tag);

    return tagset;
    }

  /** Put tags into a folder. Assumes that there are no commas in the tags */
  void PutTags(Registry &folder, const set<string> &tags)
    {
    ostringstream oss;
    for(set<string>::const_iterator it = tags.begin(); it != tags.end(); ++it)
      {
      if(it->find(',') != string::npos)
        throw IRISException("Commas are not allowed in tags. Offending tag: %s", it->c_str());

      if(it != tags.begin())
        oss << ",";
      oss << *it;
      }
    folder["Tags"] << oss.str();
    }

  /** Add a tag to a particular folder */
  void AddTag(Registry &folder, const string &newtag)
    {
    set<string> tags = this->GetTags(folder);
    tags.insert(newtag);
    this->PutTags(folder, tags);
    }

  /** Remove a tag from a folder */
  void RemoveTag(Registry &folder, const string &tag)
    {
    set<string> tags = this->GetTags(folder);
    tags.erase(tag);
    this->PutTags(folder, tags);
    }

  /** Remove given tag from all subfolders in the given folder */ 
  void ScrubTag(Registry &folder, const string &tag)
    {
    // Go over all subfolders of the given folder, recursively
    Registry::StringListType keys;
    folder.GetFolderKeys(keys);
    for(Registry::StringListType::const_iterator it = keys.begin(); it != keys.end(); ++it)
      this->ScrubTag(folder.Folder(*it), tag);

    // Do we have a tag entry
    if(folder.HasEntry("Tags"))
      this->RemoveTag(folder, tag);
    }

  /** Get the project registry by reference */
  const Registry &GetRegistry() const { return m_Registry; }

  /** Get the project registry by reference */
  Registry &GetRegistry() { return m_Registry; }

  /** Get the absolute path to the directory where the project was loaded from */
  string GetWorkspaceActualDirectory()  const { return m_WorkspaceFileDir; }

protected:

  // The Registry object containing workspace data
  Registry m_Registry;

  // Has the workspace moved from its original location
  bool m_Moved;

  // The full path and containing directory of the workspace file
  string m_WorkspaceFilePath, m_WorkspaceFileDir;

  // The directory where workspace was last saved
  string m_WorkspaceSavedDir;



};

#include <curl/curl.h>


/**
 * This class encapsulates the client side of the ALFABIS RESTful API.
 * It uses HTTP and CURL to communicate with the server
 */
class RESTClient
{
public:

  RESTClient()
    {
    m_Curl = curl_easy_init();
    m_UploadMessageBuffer[0] = 0;
    m_MessageBuffer[0] = 0; 
    }

  ~RESTClient()
    {
    curl_easy_cleanup(m_Curl);
    }

  void SetVerbose(bool verbose)
    {
    curl_easy_setopt(m_Curl, CURLOPT_VERBOSE, (long) verbose);
    }

  /**
   * Send the authentication data to the server and capture the cookie with the
   * session ID. The cookie and the server URL will be stored in a .alfabis directory
   * in user's home, so that subsequent calls do not require authentication
   */
  void Authenticate(const char *baseurl, const char *user, const char *passwd)
    {
    // Create and perform the request
    ostringstream o_url; o_url << baseurl << "/api/login";
    curl_easy_setopt(m_Curl, CURLOPT_URL, o_url.str().c_str());

    // Data to post
    char post_buffer[1024];
    sprintf(post_buffer, "email=%s&passwd=%s", user, passwd);
    curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, post_buffer);

    // Cookie file
    string cookie_jar = this->GetCookieFile();
    curl_easy_setopt(m_Curl, CURLOPT_COOKIEJAR, cookie_jar.c_str());

    // Make request
    CURLcode res = curl_easy_perform(m_Curl);

    if(res != CURLE_OK)
      throw IRISException("CURL library error: %s", curl_easy_strerror(res));

    // Store the URL for the future
    ofstream f_url(this->GetServerURLFile().c_str());
    f_url << baseurl;
    f_url.close();
    }

  /** 
   * Basic GET command. Returns true if HTTP code of 200 is received.
   *  - The string output can be retrieved with GetOutput()
   *  - The error message for non 200 code can be retrieved with GetResponseText()
   */
  bool Get(const char *rel_url, ...)
    {
    // Handle the ...
    va_list args;
    va_start(args, rel_url);

    // Expand the URL
    char url_buffer[4096];
    vsprintf(url_buffer, rel_url, args);

    // Done with the ...
    va_end(args);

    // Use the same code as POST, but with null string
    return this->Post(url_buffer, NULL);
    }

  /** 
   * Basic POST command - give a relative URL and fields to send. Both the
   * rel_url and the post_string can have printf-like expressions, which are
   * evaluated in sequential order
   */
  bool Post(const char *rel_url, const char *post_string, ...)
    {
    // Handle the ...
    va_list args;
    va_start(args, post_string);

    // Expand the URL
    char url_buffer[4096];
    vsprintf(url_buffer, rel_url, args);

    // The URL to post to
    string url = this->GetServerURL() + "/" + url_buffer;
    curl_easy_setopt(m_Curl, CURLOPT_URL, url.c_str());

    // The cookie JAR
    string cookie_jar = this->GetCookieFile();
    curl_easy_setopt(m_Curl, CURLOPT_COOKIEFILE, cookie_jar.c_str());

    // The POST data
    if(post_string)
      {
      char post_buffer[4096];
      vsprintf(post_buffer, post_string, args);
      curl_easy_setopt(m_Curl, CURLOPT_POSTFIELDS, post_buffer);

      cout << "POST " << url << " VALUES " << post_buffer << endl;
      }

    // Done with ...
    va_end (args);

    // Capture output
    m_Output.clear();
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, RESTClient::WriteCallback);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &m_Output);

    // Make request
    CURLcode res = curl_easy_perform(m_Curl);

    if(res != CURLE_OK)
      throw IRISException("CURL library error: %s", curl_easy_strerror(res));

    // Capture the response code
    m_HTTPCode = 0L;
    curl_easy_getinfo(m_Curl, CURLINFO_RESPONSE_CODE, &m_HTTPCode);

    // Get the code
    return m_HTTPCode == 200L;
    }

  bool UploadFile(const char *rel_url, const char *filename, ...)
    {
    // Expand the URL
    va_list args;
    va_start(args, filename);
    char url_buffer[4096];
    vsprintf(url_buffer, rel_url, args);
    va_end(args);

    // The URL to post to
    string url = this->GetServerURL() + "/" + url_buffer;
    curl_easy_setopt(m_Curl, CURLOPT_URL, url.c_str());

    // The cookie JAR
    string cookie_jar = this->GetCookieFile();
    curl_easy_setopt(m_Curl, CURLOPT_COOKIEFILE, cookie_jar.c_str());

    // Get the full path and just the name from the filename
    string fn_full_path = itksys::SystemTools::CollapseFullPath(filename);
    string fn_name = itksys::SystemTools::GetFilenameName(filename);

    // Use the multi-part (-F) commands
    struct curl_httppost *formpost=NULL;
    struct curl_httppost *lastptr=NULL;
    struct curl_slist *headerlist=NULL;
    static const char buf[] = "Expect:";

    /* Fill in the file upload field */ 
    curl_formadd(&formpost, &lastptr,
      CURLFORM_COPYNAME, "myfile",
      CURLFORM_FILE, fn_full_path.c_str(),
      CURLFORM_END);

    /* Fill in the filename field */ 
    curl_formadd(&formpost, &lastptr,
      CURLFORM_COPYNAME, "filename",
      CURLFORM_COPYCONTENTS, fn_name.c_str(),
      CURLFORM_END);

    /* Fill in the submit field too, even if this is rarely needed */ 
    curl_formadd(&formpost, &lastptr,
      CURLFORM_COPYNAME, "submit",
      CURLFORM_COPYCONTENTS, "send",
      CURLFORM_END);

    /* initialize custom header list (stating that Expect: 100-continue is not wanted */ 
    headerlist = curl_slist_append(headerlist, buf);

    curl_easy_setopt(m_Curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(m_Curl, CURLOPT_HTTPPOST, formpost);
    
    // Capture output
    m_Output.clear();
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, RESTClient::WriteCallback);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &m_Output);
    
    // Make request
    CURLcode res = curl_easy_perform(m_Curl);

    if(res != CURLE_OK)
      throw IRISException("CURL library error: %s", curl_easy_strerror(res));

    // Get the upload statistics
    double upload_size, upload_time;
    curl_easy_getinfo(m_Curl, CURLINFO_SIZE_UPLOAD, &upload_size);
    curl_easy_getinfo(m_Curl, CURLINFO_TOTAL_TIME, &upload_time);
    sprintf( m_UploadMessageBuffer, "%.1f Mb in %.1f s", upload_size / 1.0e6, upload_time);

    /* then cleanup the formpost chain */ 
    curl_formfree(formpost);

    /* free slist */ 
    curl_slist_free_all(headerlist);

    // Capture the response code
    m_HTTPCode = 0L;
    curl_easy_getinfo(m_Curl, CURLINFO_RESPONSE_CODE, &m_HTTPCode);

    // Get the code
    return m_HTTPCode == 200L;
    }

  const char *GetOutput()
    {
    return m_Output.c_str();
    }

  const char *GetResponseText()
    {
    sprintf(m_MessageBuffer, "Response %ld, Text: %s", m_HTTPCode, m_Output.c_str());
    return m_MessageBuffer;
    }

  const char *GetUploadStatistics()
    {
    return m_UploadMessageBuffer;
    }

protected:

  /** The CURL handle */
  CURL *m_Curl;

  /** Output stream */
  string m_Output;

  /** HTTP Code received */
  long m_HTTPCode;

  /** Message buffer */
  char m_MessageBuffer[1024];

  /** Upload message buffer */
  char m_UploadMessageBuffer[1024];

  string GetDataDirectory()
    {
    // Compute the platform-independent home directory
    vector<string> split_path;
    itksys::SystemTools::SplitPath("~/.alfabis",split_path,true);
    string ddir = itksys::SystemTools::JoinPath(split_path);
    itksys::SystemTools::MakeDirectory(ddir.c_str());
    return ddir;
    }

  string GetCookieFile() 
    {
    string cfile = this->GetDataDirectory() + "/cookie.jar";
    return itksys::SystemTools::ConvertToOutputPath(cfile);
    }

  string GetServerURLFile()
    {
    string sfile = this->GetDataDirectory() + "/server";
    return itksys::SystemTools::ConvertToOutputPath(sfile);
    }

  string GetServerURL()
    {
    string sfile = this->GetServerURLFile();
    if(!itksys::SystemTools::FileExists(sfile))
      throw IRISException("A server has not been configured yet - please sign in");
    try
      {
      string url;
      ifstream ifs(sfile.c_str());
      ifs >> url;
      ifs.close();
      return url;
      }
    catch(std::exception &exc)
      {
      throw IRISException("Failed to read server URL from %s, system exception: %s",
        sfile.c_str(), exc.what());
      }
    }

  static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
    string *buffer = static_cast<string *>(userp);
    buffer->append((char *) contents, size * nmemb);
    return size * nmemb;
    }

  static void dump(const char *text,
      FILE *stream, unsigned char *ptr, size_t size,
      char nohex)
      {
      size_t i;
      size_t c;

      unsigned int width=0x10;

      if(nohex)
        /* without the hex output, we can fit more on screen */ 
        width = 0x40;

      fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
        text, (long)size, (long)size);

      for(i=0; i<size; i+= width) {

        fprintf(stream, "%4.4lx: ", (long)i);

        if(!nohex) {
          /* hex not disabled, show it */ 
          for(c = 0; c < width; c++)
            if(i+c < size)
              fprintf(stream, "%02x ", ptr[i+c]);
            else
              fputs("   ", stream);
        }

        for(c = 0; (c < width) && (i+c < size); c++) {
          /* check for 0D0A; if found, skip past and start a new line of output */ 
          if(nohex && (i+c+1 < size) && ptr[i+c]==0x0D && ptr[i+c+1]==0x0A) {
            i+=(c+2-width);
            break;
          }
          fprintf(stream, "%c",
            (ptr[i+c]>=0x20) && (ptr[i+c]<0x80)?ptr[i+c]:'.');
          /* check again for 0D0A, to avoid an extra \n if it's at width */ 
          if(nohex && (i+c+2 < size) && ptr[i+c+1]==0x0D && ptr[i+c+2]==0x0A) {
            i+=(c+3-width);
            break;
          }
        }
        fputc('\n', stream); /* newline */ 
      }
      fflush(stream);
      }

  /**
   * Use this fuction to trace CURL communications:
   *
   *   // Trace on
   *   FILE *trace_file = fopen("/tmp/curl_trace.txt","wt");
   *   curl_easy_setopt(m_Curl, CURLOPT_DEBUGFUNCTION, &RESTClient::DebugCallback);
   *   curl_easy_setopt(m_Curl, CURLOPT_DEBUGDATA, trace_file);
   *   curl_easy_setopt(m_Curl, CURLOPT_VERBOSE, 1L);
   *   ...
   *   fclose(trace_file)
   */
  static int DebugCallback(CURL *handle, curl_infotype type,
    char *data, size_t size, void *userp)
    {
    FILE *trace_file = (FILE *) userp;
    const char *text;
    (void)handle; /* prevent compiler warning */ 

    switch(type) {
    case CURLINFO_TEXT:
      fprintf(trace_file, "== Info: %s", data);
      /* FALLTHROUGH */ 
    default: /* in case a new one is introduced to shock us */ 
      return 0;

    case CURLINFO_HEADER_OUT:
      text = "=> Send header";
      break;
    case CURLINFO_DATA_OUT:
      text = "=> Send data";
      break;
    case CURLINFO_SSL_DATA_OUT:
      text = "=> Send SSL data";
      break;
    case CURLINFO_HEADER_IN:
      text = "<= Recv header";
      break;
    case CURLINFO_DATA_IN:
      text = "<= Recv data";
      break;
    case CURLINFO_SSL_DATA_IN:
      text = "<= Recv SSL data";
      break;
    }

    dump(text, trace_file, (unsigned char *)data, size, true);
    return 0;
    }

};

/**
 * Export the workspace in a way such that all images are in the same directory as
 * the workspace and are in NIFTI format with generic filenames and no identifiers
 */
void ExportWorkspace(const Workspace &ws, const char *new_workspace)
{
  // Duplicate the workspace data
  Workspace wsexp = ws;

  // Get the directory where the new workspace will go
  string wsdir = itksys::SystemTools::GetParentDirectory(new_workspace);

  // Iterate over all the layers stored in the workspace
  int n_layers = wsexp.GetNumberOfLayers();

  // Load all of the layers in the current project
  for(int i = 0; i < n_layers; i++)
    {
    // Get the folder corresponding to the layer
    Registry &f_layer = wsexp.GetLayerFolder(i);

    // The the (possibly moved) absolute filename 
    string fn_layer = wsexp.GetLayerActualPath(f_layer);

    // The IO hints for the file
    Registry *io_hints = wsexp.GetLayerIOHints(f_layer);

    // Create a native image IO object for this image
    SmartPtr<GuidedNativeImageIO> io = GuidedNativeImageIO::New();

    // Load the header of the image and the image data
    io->ReadNativeImage(fn_layer.c_str(), *io_hints);

    // Compute the hash of the image data to generate filename
    std::string image_md5 = io->GetNativeImageMD5Hash();

    // Create a filename that combines the layer index with the hash code
    char fn_layer_new[4096];
    sprintf(fn_layer_new, "%s/layer_%03d_%s.nii.gz", wsdir.c_str(), i, image_md5.c_str());

    // Save the layer there. Since we are saving as a NIFTI, we don't need to
    // provide any hints
    Registry dummy_hints;
    io->SaveNativeImage(fn_layer_new, dummy_hints);

    // Update the layer folder with the new path
    f_layer["AbsolutePath"] << fn_layer_new;

    // There are no hints necessary for NIFTI
    f_layer.Folder("IOHints").Clear();
    }

  // Write the updated project
  wsexp.SaveAsXMLFile(new_workspace);
}

/**
 * Cross-platform way of getting a temporary path
 */
string GetTempDirName()
{
#ifdef WIN32
  char tempDir[_MAX_PATH+1] = "";

  // We will try putting the executable in the system temp directory.
  // Note that the returned path already has a trailing slash.
  DWORD length = GetTempPath(_MAX_PATH+1, tempDir);
  if(length <= 0 || length > _MAX_PATH)
    throw IRISException("Unable to create temporary directory");

  return tempDir;

#else
  char tmp_template[4096];
  strcpy(tmp_template, "/tmp/alfabis_XXXXXX");
  string tmpdir = mkdtemp(tmp_template);
  return tmpdir;
#endif
}

/**
 * Export a workspace to a temporary directory and use it to create a new ticket
 */
void CreateWorkspaceTicket(const Workspace &ws, const char *service_name)
{
  // Create a new ticket
  RESTClient rc;
  if(!rc.Post("api/tickets","service=%s", service_name))
    throw IRISException("Failed to create new ticket (%s)", rc.GetResponseText());

  int ticket_id = atoi(rc.GetOutput());
      
  cout << "Created new ticket (" << ticket_id << ")" << endl;

  // Create temporary directory for the export
  string tempdir = GetTempDirName();
  itksys::SystemTools::MakeDirectory(tempdir);

  // Export the workspace file to the temporary directory
  char ws_fname_buffer[4096];
  sprintf(ws_fname_buffer, "%s/ticket_%08d.itksnap", tempdir.c_str(), ticket_id);
  ExportWorkspace(ws, ws_fname_buffer);

  cout << "Exported workspace to " << ws_fname_buffer << endl;

  // For each of the files in the directory upload it
  itksys::Directory dir;
  dir.Load(tempdir);
  for(int i = 0; i < dir.GetNumberOfFiles(); i++)
    {
    RESTClient rcu;
    const char *thisfile = dir.GetFile(i);
    string file_full_path = itksys::SystemTools::CollapseFullPath(thisfile, dir.GetPath());
    if(!itksys::SystemTools::FileIsDirectory(file_full_path))
      {
      if(!rcu.UploadFile("api/tickets/%d/files", file_full_path.c_str(), ticket_id))
        throw IRISException("Failed up upload file %s (%s)", file_full_path.c_str(), rcu.GetResponseText());

      cout << "Upload " << thisfile << " (" << rcu.GetUploadStatistics() << ")" << endl;
      }
    }

  // TODO: we should verify that all the files were successfully sent, via MD5

  // Mark this ticket as ready
  if(!rc.Post("api/tickets/%d/status","status=ready", ticket_id))
    throw IRISException("Failed to mark ticket as ready (%s)", rc.GetResponseText());

  cout << "Changed ticket status to (" << rc.GetOutput() << ")" << endl;
  
}


int main(int argc, char *argv[])
{
  // There must be some commands!
  if(argc < 2)
    return usage(-1);

  // Command line parsing helper
  CommandLineHelper cl(argc, argv);

  // Workspace object
  Workspace ws;

  // Parse the commands in order
  while(!cl.is_at_end())
    {
    try 
      {
      // Read the next command
      std::string arg = cl.read_command();

      // Handle the various commands
      if(arg == "-i")
        {
        ws.ReadFromXMLFile(cl.read_existing_filename().c_str());
        }
      else if(arg == "-a")
        {
        // Create an archive
        ExportWorkspace(ws, cl.read_output_filename().c_str());
        }
      else if(arg == "-dump")
        {
        ws.GetRegistry().Print(cout);
        }
      else if(arg == "-list-layers")
        {
        ws.PrintLayerList(cout);
        }
      else if(arg == "-add-tag")
        {
        string key = cl.read_string();
        string tag = cl.read_string();
        ws.AddTag(ws.GetRegistry().Folder(key), tag);
        }
      else if(arg == "-add-tag-excl")
        {
        string key = cl.read_string();
        string tag = cl.read_string();
        ws.ScrubTag(ws.GetRegistry(), tag);
        ws.AddTag(ws.GetRegistry().Folder(key), tag);
        }
      else if(arg == "-delete-tag")
        {
        string key = cl.read_string();
        string tag = cl.read_string();
        ws.RemoveTag(ws.GetRegistry().Folder(key), tag);
        }
      else if(arg == "-dss-auth")
        {
        // TODO: add ability to prompt for user and password with hidden password
        string url = cl.read_string();
        string email = cl.read_string();
        string pass = cl.read_string();
        RESTClient rc;
        rc.Authenticate(url.c_str(), email.c_str(), pass.c_str());
        }
      else if(arg == "-dss-services-list")
        {
        RESTClient rc;
        if(rc.Get("api/services"))
          cout << rc.GetOutput();
        else
          throw IRISException("Error listing services: %s", rc.GetResponseText());
        }
      else if(arg == "-dss-tickets-create" || arg == "-dss-ticket-create")
        {
        string service_name = cl.read_string();
        CreateWorkspaceTicket(ws, service_name.c_str());
        }
      else if(arg == "-dss-tickets-list")
        {
        RESTClient rc;
        if(rc.Get("api/tickets"))
          cout << rc.GetOutput();
        else
          throw IRISException("Error listing tickets: %s", rc.GetResponseText());
        }
      }
    catch(IRISException &exc)
      {
      cerr << "ITK-SNAP exception: " << exc.what() << endl;
      return -1;
      }
    catch(std::exception &sexc)
      {
      cerr << "System exception: " << sexc.what() << endl;
      }
    }


}
