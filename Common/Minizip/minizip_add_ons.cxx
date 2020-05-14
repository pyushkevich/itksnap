#include "minizip.h"

#include "cstring"
#include <iostream>

#if(WIN32)
#define USEWIN32IOAPI
#include "iowin32.h"
#include <direct.h>
#include <strsafe.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif


int CreateZIPFile(std::string ZIPfilename, std::vector<char*> files_to_zip)
{
  int opt_compress_level=Z_DEFAULT_COMPRESSION;
  char filename_try[MAXFILENAME+16];
  int err=0;
  int size_buf = WRITEBUFFERSIZE;
  void* buf = (void*)malloc(size_buf);
  if (buf==NULL)
  {
      printf("Error allocating memory\n");
      return ZIP_INTERNALERROR;
  }


  int i,len;
  int dot_found=0;

  strncpy(filename_try, ZIPfilename.c_str(),MAXFILENAME-1);
  // strncpy doesnt append the trailing NULL, of the string is too long.
  filename_try[ MAXFILENAME ] = '\0';

  len=(int)strlen(filename_try);
  for (i=0;i<len;i++)
      if (filename_try[i]=='.')
          dot_found=1;

  if (dot_found==0)
      strcat(filename_try,".zip");


  zipFile zf;
  int errclose;
#ifdef USEWIN32IOAPI
  zlib_filefunc_def ffunc;
  fill_win32_filefunc(&ffunc);
  zf = zipOpen2(filename_try,0,NULL,&ffunc);
#else
  zf = zipOpen(filename_try,0);
#endif

  if (zf == NULL)
  {
      std::cout << "error opening " << filename_try << std::endl;
      err= ZIP_ERRNO;
  }

  for (i=0; (i < (int)files_to_zip.size()) && (err==ZIP_OK); i++)
  {

      int size_read;
      char* filenameinzip = files_to_zip.at(i);
      zip_fileinfo zi;

      zi.tmz_date.tm_sec = zi.tmz_date.tm_min = zi.tmz_date.tm_hour =
              zi.tmz_date.tm_mday = zi.tmz_date.tm_mon = zi.tmz_date.tm_year = 0;
      zi.dosDate = 0;
      zi.internal_fa = 0;
      zi.external_fa = 0;
      filetime(filenameinzip,&zi.tmz_date,&zi.dosDate);

      err = zipOpenNewFileInZip(zf,filenameinzip,&zi,
                                NULL,0,NULL,0,NULL, // comment
                                (opt_compress_level != 0) ? Z_DEFLATED : 0,
                                opt_compress_level);
      //}
      if (err != ZIP_OK)
          std::cout << "error in opening" << filenameinzip << " in zipfile" << std::endl;
      else
      {
          if (std::string(filenameinzip).find(".gz") != std::string::npos)
          {
              gzFile fin = gzopen(filenameinzip,"rb");

              if (fin==NULL)
              {
                  err=ZIP_ERRNO;
                  std::cout << "error in opening " << filenameinzip << " for reading." << std::endl;
              }


              if (err == ZIP_OK)
                  do
              {
                  err = ZIP_OK;
                  size_read = (int)gzread(fin, buf, size_buf);

                  if (size_read>0)
                  {
                      err = zipWriteInFileInZip (zf,buf,size_read);
                      if (err<0)
                      {
                          std::cout << "error in writing " << filenameinzip << " in the zipfile" << std::endl;
                      }

                  }
              } while ((err == ZIP_OK) && (size_read>0));

              if (fin)
                  gzclose(fin);

          }
          else{
              FILE * fin = fopen(filenameinzip,"rb");
              if (fin==NULL)
              {
                  err=ZIP_ERRNO;
                  std::cout << "error in opening " << filenameinzip << " for reading." << std::endl;
              }

              if (err == ZIP_OK)
                  do
              {
                  err = ZIP_OK;
                  size_read = (int)fread(buf,1,size_buf,fin);
                  if (size_read < size_buf)
                      if (feof(fin)==0)
                      {
                          std::cout << "error in reading " << filenameinzip << std::endl;
                          // err = ZIP_ERRNO;
                          break;
                      }

                  if (size_read>0)
                  {
                      err = zipWriteInFileInZip (zf,buf,size_read);
                      if (err<0)
                      {
                          std::cout << "error in writing " << filenameinzip << " in the zipfile" << std::endl;
                      }

              }
              } while ((err == ZIP_OK) && (size_read>0));

              if (fin)
                  fclose(fin);
          }
      }
      if (err<0)
          err=ZIP_ERRNO;
      else
      {
          err = zipCloseFileInZip(zf);
          if (err!=ZIP_OK)
              std::cout << "error in closing " << filenameinzip << " in the zipfile" << std::endl;
      }

  }
  errclose = zipClose(zf,NULL);
  if (errclose != ZIP_OK)
      std::cout << "error in closing " << filename_try << std::endl;

  free(buf);
  return 0;  // to avoid warning
}

void ZipAFolder(const char* zipname, const char* path_to_folder)
{
  // If the file already exists, it'll be overwritten!
  std::string folder = std::string(path_to_folder).substr(std::string(path_to_folder).find_last_of("/") +1);
  std::string path = std::string(path_to_folder).substr(0, std::string(path_to_folder).find_last_of("/"));
  std::vector<char*> files_in_folder;

#if(WIN32)
  _chdir(path.c_str());
#else
  chdir(path.c_str());
#endif

#ifdef USEWIN32IOAPI
  std::string folder_to_read = std::string(path_to_folder) + "\\*";
  WIN32_FIND_DATA fd;
  HANDLE hFind = ::FindFirstFile(folder_to_read.c_str(), &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
      do {
          // read all (real) files in current folder
          // , delete '!' read other 2 default folder . and ..
          if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0) {
              std::string full_path = std::string(folder) + "/" + std::string(fd.cFileName);
              char* copy = new char;
              strcpy(copy, full_path.c_str());
              files_in_folder.push_back(copy);
          }
      } while (::FindNextFile(hFind, &fd));
      ::FindClose(hFind);
  }
#else
  DIR* dir;
  struct dirent* ent;
  if ((dir = opendir(path_to_folder)) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      char* filename = ent->d_name;
      if (strcmp(filename, ".") != 0 && strcmp(filename, "..") != 0)
      {
        std::string full_path = std::string(folder) + "/" + std::string(filename);
        char* copy = new char;
        strcpy(copy,full_path.c_str());
        files_in_folder.push_back(copy);
      }
    }
    closedir(dir);
  }
#endif

  CreateZIPFile(zipname, files_in_folder);
}
