/* iowin32.h -- IO base function header for compress/uncompress .zip
   files using zlib + zip or unzip API
   This IO API version uses the Win32 API (for Microsoft Windows)

   Version 1.01e, February 12th, 2005

   Copyright (C) 1998-2005 Gilles Vollant
*/


#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(WIN32)
   #include <windows.h>

void fill_win32_filefunc OF((zlib_filefunc_def* pzlib_filefunc_def));
#endif

#ifdef __cplusplus
}
#endif
