/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkznzlib.h
 
*****            This code is released to the public domain.            *****

*****  Author: Mark Jenkinson, FMRIB Centre, University of Oxford       *****
*****  Date:   September 2004                                           *****

*****  Neither the FMRIB Centre, the University of Oxford, nor any of   *****
*****  its employees imply any warranty of usefulness of this software  *****
*****  for any purpose, and do not assume any liability for damages,    *****
*****  incidental or otherwise, caused by any use of this document.     *****

This library provides an interface to both compressed (gzip/zlib) and
uncompressed (normal) file IO.  The functions are written to have the
same interface as the standard file IO functions.  

To use this library instead of normal file IO, the following changes
are required:
 - replace all instances of FILE* with znzFile
 - change the name of all function calls, replacing the initial character
   f with the znz  (e.g. fseek becomes znzseek)
 - add a third parameter to all calls to znzopen (previously fopen)
   that specifies whether to use compression (1) or not (0)
 - use znz_isnull rather than any (pointer == NULL) comparisons in the code
 
NB: seeks for writable files with compression are quite restricted

*****  Converted to C++ by: Joseph Hennessey,                           *****
*****              Center for Imaging Science, Johns Hopkins University *****
*****  Date:    March 2010                                              *****

=========================================================================*/
// .NAME vtkznzlib - vtkznzlib
// .SECTION Description
// vtkznzlib 
//
// .SECTION See Also
// 

#ifndef vtkznzlib_h
#define vtkznzlib_h

#include "vtkObject.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* include optional check for HAVE_FDOPEN here, from deleted config.h:

   uncomment the following line if fdopen() exists for your compiler and
   compiler options
*/
/* #define HAVE_FDOPEN */

#ifdef HAVE_ZLIB
#if defined(ITKZLIB)
#include "itk_zlib.h"
#elif defined(VTKZLIB)
#include "vtk_zlib.h"
#else
#include "zlib.h"
#endif
#endif

//#include "vtk_znzlib_mangle.h"

struct znzptr {
  int withz;
  FILE* nzfptr;
#ifdef HAVE_ZLIB
  gzFile zfptr;
#endif
} ;

/* the type for all file pointers */
typedef struct znzptr * znzFile;


/* int znz_isnull(znzFile f); */
/* int znzclose(znzFile f); */
#define znz_isnull(f) ((f) == NULL)
#define znzclose(f)   Xznzclose(&(f))

/* Note extra argument (use_compression) where 
   use_compression==0 is no compression
   use_compression!=0 uses zlib (gzip) compression
*/

class vtkznzlib : public vtkObject
{
public:
  static vtkznzlib *New();
  vtkTypeMacro(vtkznzlib,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

static znzFile znzopen(const char *path, const char *mode, int use_compression);

static znzFile znzdopen(int fd, const char *mode, int use_compression);

static int Xznzclose(znzFile * file);

static size_t znzread(void* buf, size_t size, size_t nmemb, znzFile file);

static size_t znzwrite(const void* buf, size_t size, size_t nmemb, znzFile file);

static long znzseek(znzFile file, long offset, int whence);

static int znzrewind(znzFile stream);

static long znztell(znzFile file);

static int znzputs(const char *str, znzFile file);

static char * znzgets(char* str, int size, znzFile file);

static int znzputc(int c, znzFile file);

static int znzgetc(znzFile file);

protected:
  vtkznzlib();
  ~vtkznzlib() override;

static int znzflush(znzFile file);
static int znzeof(znzFile file);


private:
  vtkznzlib(const vtkznzlib&) = delete;
  void operator=(const vtkznzlib&) = delete;

};

#endif
