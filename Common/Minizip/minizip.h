#ifndef _minizip_H
#define _minizip_H

#include "zip.h"

#define WRITEBUFFERSIZE (16384)
#define MAXFILENAME (256)


#ifdef __cplusplus
#include <string>
#include <vector>

int CreateZIPFile(std::string ZIPfilename, std::vector<char*> files_to_zip);
void ZipAFolder(const char* zipname, const char* folder);


extern "C" {
#endif

#ifdef __APPLE__
#define unix
#endif

#ifdef WIN32
uLong filetime(char *f, tm_zip *tmzip, uLong *dt);
#else
#ifdef unix
uLong filetime(char *f, tm_zip *tmzip, uLong *dt);
#endif

int check_exist_file(const char* filename);
//void do_banner();
//void do_help();

#endif

#ifdef __cplusplus
}
#endif

#endif
