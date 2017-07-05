/**
 * This is a command-line tool for distributed processing and segmentation
 * of ITK-SNAP workspaces using the ALFABIS server.
 */
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <string>
#include "CommandLineHelper.h"

using namespace std;

int main(int argc, char *argv[])
{
  // Initialize the CURL library
  curl_global_init(CURL_GLOBAL_ALL);
  CURL *curl = curl_easy_init();
  if(!curl)
    {
    cerr << "libcurl failed to initialize" << endl;
    return -1;
    }

  try 
    {
    // Authentication parameters
    string userid, passwd, baseurl;

    // Location of the session file
    string url_file = ".alfabis_session_url.dat";
    string cookie_file = ".alfabis_session_cookies.dat";

    CommandLineHelper cl(argc, argv);
    while(!cl.is_at_end())
      {
      // Read the next command
      std::string arg = cl.read_command();

      // Specify user on the command-line
      if(arg == "-user" || arg == "-u")
        {
        userid = cl.read_string();
        }
      else if(arg == "-pass" || arg == "-p")
        {
        passwd = cl.read_string();
        }

      // Initialize session - takes the ALFABIS base URL
      else if(arg == "-init-session" || arg == "-is")
        {
        // Get the URL
        string url = cl.read_string();

        // TODO: prompt for password if not supplied
        
        // Create and perform the request
        ostringstream o_url; o_url << url << "/api/login";
        curl_easy_setopt(curl, CURLOPT_URL, o_url.str().c_str());

        // Data to post
        char post_buffer[1024];
        sprintf(post_buffer, "email=%s&passwd=%s", userid.c_str(), passwd.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_buffer);

        // Cookie file
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_file.c_str());

        // Make request
        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK)
          {
          cerr << "CURL error: " << curl_easy_strerror(res) << endl;
          return -1;
          }
        curl_easy_cleanup(curl);

        // Store the URL for future
        ofstream f_url(url_file.c_str());
        f_url << url;
        }

      else if(arg == "-list-services" || arg == "-ls")
        {
        // Read the URL from file
        ifstream f_url(url_file.c_str());
        string url;
        f_url >> url;

        // Create and perform the request
        ostringstream o_url; o_url << url << "/api/services";
        curl_easy_setopt(curl, CURLOPT_URL, o_url.str().c_str());

        // Cookie file
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookie_file.c_str());

        // Make request
        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK)
          {
          cerr << "CURL error: " << curl_easy_strerror(res) << endl;
          return -1;
          }
        curl_easy_cleanup(curl);

        }

      
      }

  
    }
  catch(...)
    {

    }

  // Handle the command-line requests


  

  
}
