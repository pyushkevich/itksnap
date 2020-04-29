#ifndef _miniunz_H
#define _miniunz_H


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__
#define unix
#endif

#ifdef unix
# include <unistd.h>
# include <utime.h>
#else
# include <direct.h>
# include <io.h>
#endif

#include "unzip.h"

#define CASESENSITIVITY (0)
#define WRITEBUFFERSIZE (8192)
#define MAXFILENAME (256)

#ifdef WIN32
#define USEWIN32IOAPI
#include "iowin32.h"
#endif

/* change_file_date : change the date/time of a file
    filename : the filename of the file where date/time must be modified
    dosdate : the new date at the MSDos format (4 bytes)
    tmu_date : the SAME new date at the tm_unz format */
void change_file_date(const char *filename, uLong dosdate, tm_unz tmu_date);


/* mymkdir and change_file_date are not 100 % portable
   As I don't know well Unix, I wait feedback for the unix portion */

int mymkdir(const char* dirname);

int makedir (char *newdir);

void do_banner();

void do_help();

int do_list(unzFile uf);

int do_extract_currentfile(unzFile uf,const int* popt_extract_without_path,int* popt_overwrite,const char* password);

int do_extract(unzFile uf,int opt_extract_without_path,int opt_overwrite,const char* password);

int do_extract_onefile(unzFile uf,const char* filename,int opt_extract_without_path,int opt_overwrite,const char* password);

const char* extract_zip (const char* filename);

#ifdef __cplusplus
}
#endif
#endif
