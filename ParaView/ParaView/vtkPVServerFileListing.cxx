/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerFileListing.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVServerFileListing.h"

#include "vtkClientServerInterpreter.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkSource.h"

#include <vtkstd/set>
#include <vtkstd/string>

#if defined(_WIN32)
# include <windows.h>   // FindFirstFile, FindNextFile, FindClose
# include <direct.h>    // _getcwd
# define vtkPVServerFileListingGetCWD _getcwd
#else
# include <sys/types.h> // DIR, struct dirent, struct stat
# include <sys/stat.h>  // stat
# include <dirent.h>    // opendir, readdir, closedir
# include <unistd.h>    // access, getcwd
# include <errno.h>     // errno
# include <string.h>    // strerror
# define vtkPVServerFileListingGetCWD getcwd
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerFileListing);
vtkCxxRevisionMacro(vtkPVServerFileListing, "1.1.2.3");

//----------------------------------------------------------------------------
class vtkPVServerFileListingInternals
{
public:
  vtkClientServerStream Result;
  vtkstd::string CurrentWorkingDirectory;
};

//----------------------------------------------------------------------------
vtkPVServerFileListing::vtkPVServerFileListing()
{
  this->Internal = new vtkPVServerFileListingInternals;
}

//----------------------------------------------------------------------------
vtkPVServerFileListing::~vtkPVServerFileListing()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVServerFileListing::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
const vtkClientServerStream&
vtkPVServerFileListing::GetFileListing(const char* dirname, int save)
{
  // Reset the result for a new listing.
  this->Internal->Result.Reset();

  // If there is a process module, make sure we are process 0.
  if(this->ProcessModule && (this->ProcessModule->GetPartitionId() > 0))
    {
    return this->Internal->Result;
    }

  if(dirname)
    {
    // If an empty name is given, use the current working directory.
    if(dirname[0])
      {
      this->List(dirname, save?1:0);
      }
    else
      {
      this->List(".", save?1:0);
      }
    }
  else
    {
    vtkErrorMacro("GetFileListing cannot work with a NULL directory.");
    }

  return this->Internal->Result;
}

//----------------------------------------------------------------------------
void vtkPVServerFileListing::List(const char* dirname, int save)
{
  typedef vtkstd::set< vtkstd::string > DirectoriesType;
  typedef vtkstd::set< vtkstd::string > FilesType;
  DirectoriesType directories;
  FilesType files;

  // Prepare to construct a whole filename.
  vtkstd::string prefix = dirname;
  char last = *(prefix.end()-1);
  if(last != '/' && last != '\\')
    {
    prefix += "/";
    }

#if defined(_WIN32)
  // Search for all files in the given directory.
  vtkstd::string pattern = prefix;
  pattern += "*";
  WIN32_FIND_DATA data;
  HANDLE handle = FindFirstFile(pattern.c_str(), &data);
  if(!handle)
    {
    // Could add check of GetLastError here.
    return;
    }

  int done = 0;
  while(!done)
    {
    // Look at this file.
    if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
      if(strcmp(data.cFileName, "..") != 0 && strcmp(data.cFileName, ".") != 0)
        {
        directories.insert(data.cFileName);
        }
      }
    else if((data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) ||
            (!((data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) && save) &&
             !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) &&
             !(data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) &&
             !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))
      {
      files.insert(data.cFileName);
      }

    // Find the next file.
    done = (FindNextFile(handle, &data) == 0)?1:0;
    }

  if(GetLastError() != ERROR_NO_MORE_FILES)
    {
    // Could add check of GetLastError here.
    vtkErrorMacro("Error calling FindNextFile.");
    }

  if(!FindClose(handle))
    {
    // Could add check of GetLastError here.
    vtkErrorMacro("Error calling FindClose.");
    }
#else
  // Open the directory and make sure it exists.
  DIR* dir = opendir(dirname);
  if(!dir)
    {
    // Could add check of errno here.
    return;
    }

  // Loop through the directory listing.
  while(const dirent* d = readdir(dir))
    {
    // Skip the special directory entries.
    if(strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
      {
      continue;
      }

    // Construct the full filename.
    vtkstd::string filename = prefix;
    filename += d->d_name;

    // Check the file type.
    struct stat info;
    if(stat(filename.c_str(), &info) < 0)
      {
      int e = errno;
      vtkErrorMacro("Cannot stat file \"" << filename.c_str() << "\": "
                    << strerror(e));
      continue;
      }

    if(info.st_mode & S_IFDIR)
      {
      // Check if we are able to access the directory.
      if(access(filename.c_str(), R_OK) == 0)
        {
        directories.insert(d->d_name);
        }
      }
    else if(info.st_mode & S_IFREG)
      {
      // Check if we are able to access the file.
      if(access(filename.c_str(), save?W_OK:R_OK) == 0)
        {
        files.insert(d->d_name);
        }
      }
    int fixme; // Handle S_IFLNK (symbolic links).
    }
#endif

  // List the directories in the first message.
  this->Internal->Result << vtkClientServerStream::Reply;
  for(DirectoriesType::const_iterator d = directories.begin();
      d != directories.end(); ++d)
    {
    this->Internal->Result << d->c_str();
    }
  this->Internal->Result << vtkClientServerStream::End;

  // List the files in the second message.
  this->Internal->Result << vtkClientServerStream::Reply;
  for(FilesType::const_iterator f = files.begin();
      f != files.end(); ++f)
    {
    this->Internal->Result << f->c_str();
    }
  this->Internal->Result << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
const char* vtkPVServerFileListing::GetCurrentWorkingDirectory()
{
  char buf[2048];
  vtkPVServerFileListingGetCWD(buf, sizeof(buf));
  this->Internal->CurrentWorkingDirectory = buf;
  return this->Internal->CurrentWorkingDirectory.c_str();
}

//----------------------------------------------------------------------------
int vtkPVServerFileListing::FileIsDirectory(const char* dirname)
{
  struct stat fs;
  if(stat(dirname, &fs) == 0)
    {
#ifdef _WIN32
    return ((fs.st_mode & _S_IFDIR) != 0)? 1:0;
#else
    return S_ISDIR(fs.st_mode)? 1:0;
#endif
    }
  return 0;
}
