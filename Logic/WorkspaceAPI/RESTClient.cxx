#include "RESTClient.h"
#include <sstream>
#include <fstream>
#include <cstdarg>
#include "IRISException.h"
#include "itksys/SystemTools.hxx"
#include "FormattedTable.h"

using itksys::SystemTools;

#ifdef WIN32
#define CURL_STATICLIB
#endif
#include <curl/curl.h>



using namespace std;

RESTClient::RESTClient()
{
  m_Curl = curl_easy_init();
  m_UploadMessageBuffer[0] = 0;
  m_MessageBuffer[0] = 0;
  m_OutputFile = NULL;
}

RESTClient::~RESTClient()
{
  curl_easy_cleanup(m_Curl);
}

void RESTClient::SetVerbose(bool verbose)
{
  curl_easy_setopt(m_Curl, CURLOPT_VERBOSE, (long) verbose);
}

void RESTClient::SetOutputFile(FILE *outfile)
{
  m_OutputFile = outfile;
}

void RESTClient::Authenticate(const char *baseurl, const char *token)
{
  // Create and perform the request
  ostringstream o_url; o_url << baseurl << "/api/login";
  curl_easy_setopt(m_Curl, CURLOPT_URL, o_url.str().c_str());

  // Data to post
  char post_buffer[1024];
  sprintf(post_buffer, "token=%s", token);
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

bool RESTClient::Get(const char *rel_url, ...)
{
  // Handle the ...
  std::va_list args;
  va_start(args, rel_url);

  // Expand the URL
  char url_buffer[4096];
  vsprintf(url_buffer, rel_url, args);

  // Done with the ...
  va_end(args);

  // Use the same code as POST, but with null string
  return this->Post(url_buffer, NULL);
}

bool RESTClient::Post(const char *rel_url, const char *post_string, ...)
{
  // Handle the ...
  std::va_list args;
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

  // If there is no output file, use the default callbacl
  if(!m_OutputFile)
    {
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, RESTClient::WriteCallback);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, &m_Output);
    }
  else
    {
    curl_easy_setopt(m_Curl, CURLOPT_WRITEFUNCTION, RESTClient::WriteToFileCallback);
    curl_easy_setopt(m_Curl, CURLOPT_WRITEDATA, m_OutputFile);
    }

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

bool RESTClient::UploadFile(
    const char *rel_url, const char *filename,
    std::map<string, string> extra_fields, ...)
{
  // Expand the URL
  std::va_list args;
  va_start(args, extra_fields);
  char url_buffer[4096];
  vsprintf(url_buffer, rel_url, args);

  // The URL to post to
  string url = this->GetServerURL() + "/" + url_buffer;
  curl_easy_setopt(m_Curl, CURLOPT_URL, url.c_str());

  // The cookie JAR
  string cookie_jar = this->GetCookieFile();
  curl_easy_setopt(m_Curl, CURLOPT_COOKIEFILE, cookie_jar.c_str());

  // Get the full path and just the name from the filename
  string fn_full_path = SystemTools::CollapseFullPath(filename);
  string fn_name = SystemTools::GetFilenameName(filename);

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

  /* Add the extra form fields */
  for(std::map<string,string>::const_iterator it = extra_fields.begin();
      it != extra_fields.end(); ++it)
    {
    char post_buffer[4096];
    vsprintf(post_buffer, it->second.c_str(), args);

    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, it->first.c_str(),
                 CURLFORM_COPYCONTENTS, post_buffer,
                 CURLFORM_END);
    }

  // Done expanding args
  va_end(args);

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

const char *RESTClient::GetOutput()
{
  return m_Output.c_str();
}

std::string RESTClient::GetFormattedCSVOutput(bool header)
{
  FormattedTable ft;
  ft.ParseCSV(m_Output);
  ostringstream oss;
  ft.Print(oss);
  return oss.str();
}

const char *RESTClient::GetResponseText()
{
  sprintf(m_MessageBuffer, "Response %ld, Text: %s", m_HTTPCode, m_Output.c_str());
  return m_MessageBuffer;
}

const char *RESTClient::GetUploadStatistics()
{
  return m_UploadMessageBuffer;
}

string RESTClient::GetDataDirectory()
{
  // Compute the platform-independent home directory
  vector<string> split_path;
  SystemTools::SplitPath("~/.alfabis",split_path,true);
  string ddir = SystemTools::JoinPath(split_path);
  SystemTools::MakeDirectory(ddir.c_str());
  return ddir;
}

string RESTClient::GetCookieFile()
{
  string cfile = this->GetDataDirectory() + "/cookie.jar";
  return SystemTools::ConvertToOutputPath(cfile);
}

string RESTClient::GetServerURLFile()
{
  string sfile = this->GetDataDirectory() + "/server";
  return SystemTools::ConvertToOutputPath(sfile);
}

string RESTClient::GetServerURL()
{
  string sfile = this->GetServerURLFile();
  if(!SystemTools::FileExists(sfile))
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

size_t RESTClient::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  string *buffer = static_cast<string *>(userp);
  buffer->append((char *) contents, size * nmemb);
  return size * nmemb;
}

size_t RESTClient::WriteToFileCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  FILE *file = static_cast<FILE *>(userp);
  return fwrite(contents, size, nmemb, file);
}



static void REST_DebugDump(const char *text,
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
int REST_DebugCallback(void *handle, curl_infotype type, char *data, size_t size, void *userp)
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

  REST_DebugDump(text, trace_file, (unsigned char *)data, size, true);
  return 0;
}


