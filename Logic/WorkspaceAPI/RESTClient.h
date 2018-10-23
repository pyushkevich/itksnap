#ifndef RESTCLIENT_H
#define RESTCLIENT_H

#include <string>
#include <cstdarg>
#include <map>

/**
 * This class encapsulates the client side of the ALFABIS RESTful API.
 * It uses HTTP and CURL to communicate with the server
 */
class RESTClient
{
public:

  RESTClient();

  ~RESTClient();

  void SetVerbose(bool verbose);

  /**
   * Set a FILE * to which to write the output of the Get/Post. This overrides
   * the default behaviour to capture the output in a string that can be accessed
   * with GetOutput(). This is useful for downloading binary files
   */
  void SetOutputFile(FILE *outfile);

  /**
   * Send the authentication data to the server and capture the cookie with the
   * session ID. The cookie and the server URL will be stored in a .alfabis directory
   * in user's home, so that subsequent calls do not require authentication
   */
  bool Authenticate(const char *baseurl, const char *token);

  /**
   * This call will set the server URL for subsequent calls. Subsequent calls will fail
   * unless there is a cookie (session) present for the selected URL
   */
  static void SetServerURL(const char *baseurl);

  /**
   * Call this function prior to post if you want to open up the cookie jar to receive
   * cookies from the server. This is done automatically by the Authenticate function
   */
  void SetReceiveCookieMode(bool mode);

  /**
   * Basic GET command. Returns true if HTTP code of 200 is received.
   *  - The string output can be retrieved with GetOutput()
   *  - The error message for non 200 code can be retrieved with GetResponseText()
   */
  bool Get(const char *rel_url, ...);

  /** Version of Get that takes a va_list */
  bool GetVA(const char *rel_url, std::va_list args);

  /**
   * Basic POST command - give a relative URL and fields to send. Both the
   * rel_url and the post_string can have printf-like expressions, which are
   * evaluated in sequential order
   */
  bool Post(const char *rel_url, const char *post_string, ...);

  /** Version of Post that takes a va_list */
  bool PostVA(const char *rel_url, const char *post_string, std::va_list args);

  // Progress callback signature
  typedef  void ( *ProgressCallbackFunction )(void *, double);

  /**
   * Set the callback command for uploads and downloads
   */
  void SetProgressCallback(void *cb_data, ProgressCallbackFunction fn);

  bool UploadFile(const char *rel_url, const char *filename,
    std::map<std::string,std::string> extra_fields, ...);

  const char *GetOutput();

  std::string GetFormattedCSVOutput(bool header);

  const char *GetResponseText();

  const char *GetUploadStatistics();

protected:

  /** The CURL handle */
  void *m_Curl;

  /** The sharing handle */
  void *m_Share;

  /** Optional file for output */
  FILE *m_OutputFile;

  /** Output stream */
  std::string m_Output;

  /** HTTP Code received */
  long m_HTTPCode;

  /** Message buffer */
  char m_MessageBuffer[1024];

  /** Upload message buffer */
  char m_UploadMessageBuffer[1024];

  /** Error buffer */
  char *m_ErrorBuffer;

  bool m_ReceiveCookieMode;

  /** Callback stuff */
  std::pair<void *, ProgressCallbackFunction> m_CallbackInfo;

  static std::string GetDataDirectory();

  static std::string GetCookieFile();

  static std::string GetServerURLFile();

  static std::string GetServerURL();

  static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp);

  static size_t WriteToFileCallback(void *contents, size_t size, size_t nmemb, void *userp);



};



#endif // RESTCLIENT_H
