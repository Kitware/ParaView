/*=========================================================================

  Program:   ParaView
  Module:    vtkPVServerFileListing.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerFileListing.h"

#include "vtkClientServerInterpreter.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkSource.h"
#include "vtkClientServerStream.h"

#include <vtkstd/set>
#include <vtkstd/string>

#if defined(_WIN32)
# include <windows.h>   // FindFirstFile, FindNextFile, FindClose, ...
# include <direct.h>    // _getcwd
# include <sys/stat.h>  // stat
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
vtkCxxRevisionMacro(vtkPVServerFileListing, "1.2");

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
  if(handle == INVALID_HANDLE_VALUE)
    { 
    LPVOID lpMsgBuf;
    FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
      0,
      NULL 
      );
    // Could add check of GetLastError here.
    vtkErrorMacro("Error calling FindFirstFile : " << (char*)lpMsgBuf << "\nDirectory: " << pattern.c_str());
    LocalFree( lpMsgBuf );
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
    LPVOID lpMsgBuf;
    
    FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
      0,
      NULL 
      );
    vtkErrorMacro("Error calling FindNextFile : " << (char*)lpMsgBuf << "\nDirectory: " << dirname);
    LocalFree( lpMsgBuf );
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
    }
  closedir(dir);
  
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

//----------------------------------------------------------------------------
int vtkPVServerFileListing::FileIsReadable(const char* name)
{
#if defined(_WIN32)
  DWORD atts = GetFileAttributes(name);
  if((atts & FILE_ATTRIBUTE_NORMAL) ||
     (!(atts & FILE_ATTRIBUTE_HIDDEN) &&
      !(atts & FILE_ATTRIBUTE_SYSTEM) &&
      !(atts & FILE_ATTRIBUTE_DIRECTORY)))
    {
    return 1;
    }
#else
  // Check if we are able to access the file.
  if(access(name, R_OK) == 0)
    {
    return 1;
    }
#endif
  return 0;
}
