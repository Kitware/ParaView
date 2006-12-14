/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFileInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVFileInformation.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkPVFileInformationHelper.h"
#include "vtkSmartPointer.h"

#if defined(_WIN32)
# include <windows.h>   // FindFirstFile, FindNextFile, FindClose, ...
# include <direct.h>    // _getcwd
# include <shlobj.h>    // SHGetFolderPath
# include <sys/stat.h>  // stat
# define vtkPVServerFileListingGetCWD _getcwd
#else
# include <sys/types.h> // DIR, struct dirent, struct stat
# include <sys/stat.h>  // stat
# include <dirent.h>    // opendir, readdir, closedir
# include <unistd.h>    // access, getcwd
# include <errno.h>     // errno
# include <string.h>    // strerror
# include <stdlib.h>    // getenv
# define vtkPVServerFileListingGetCWD getcwd
#endif

#include <vtksys/SystemTools.hxx>
#include <vtksys/RegularExpression.hxx>
#include <vtkstd/set>

vtkStandardNewMacro(vtkPVFileInformation);
vtkCxxRevisionMacro(vtkPVFileInformation, "1.1");

inline void vtkPVFileInformationAddTerminatingSlash(vtkstd::string& name)
{
  if (name.size()>0)
    {
    char last = *(name.end()-1);
    if (last != '/' && last != '\\')
      {
      name += "/";
      }
    }
}

//-----------------------------------------------------------------------------
vtkPVFileInformation::vtkPVFileInformation()
{
  this->RootOnly = 1;
  this->Contents = vtkCollection::New();
  this->Type = INVALID;
  this->Name = 0;
  this->FullPath = 0;
  this->FastFileTypeDetection = 0;
}

//-----------------------------------------------------------------------------
vtkPVFileInformation::~vtkPVFileInformation()
{
  this->Contents->Delete();
  this->SetName(0);
  this->SetFullPath(0);
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::CopyFromObject(vtkObject* object)
{
  this->Initialize();

  vtkPVFileInformationHelper* helper = 
    vtkPVFileInformationHelper::SafeDownCast(object);
  if (!helper)
    {
    vtkErrorMacro(
      "Can collect information only from a vtkPVFileInformationHelper.");
    return;
    }

  if (helper->GetSpecialDirectories())
    {
    this->GetSpecialDirectories();
    return;
    }

  this->FastFileTypeDetection = helper->GetFastFileTypeDetection();

  vtkstd::string path = vtksys::SystemTools::CollapseFullPath(helper->GetPath(),
    vtksys::SystemTools::GetCurrentWorkingDirectory().c_str());

  this->SetName(helper->GetPath());
  this->SetFullPath(path.c_str());

  if (!vtksys::SystemTools::FileExists(this->FullPath))
    {
    return;
    }

  bool is_directory = vtksys::SystemTools::FileIsDirectory(this->FullPath);
  this->Type = (is_directory)? DIRECTORY : SINGLE_FILE;

  if (!helper->GetDirectoryListing() || !is_directory)
    {
    return;
    }

  // Since we want a directory listing, we now to platform specific listing 
  // with intelligent pattern matching hee-haa.

#if defined(_WIN32)
  this->GetWindowsDirectoryListing();
#else
  this->GetDirectoryListing();
#endif
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::GetSpecialDirectories()
{
#if defined (_WIN32)

#if (_WIN32_IE >= 0x0400) // For SHGetSpecialFolderPath()

  // Return favorite directories ...

  TCHAR szPath[MAX_PATH];

  if(SUCCEEDED(SHGetSpecialFolderPath(NULL, szPath, CSIDL_PERSONAL, false)))
    {
    vtkSmartPointer<vtkPVFileInformation> info =
      vtkSmartPointer<vtkPVFileInformation>::New();
    info->SetFullPath(szPath);
    info->SetName("My Documents");
    info->Type = DIRECTORY;
    this->Contents->AddItem(info);
    }

  if(SUCCEEDED(SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, false)))
    {
    vtkSmartPointer<vtkPVFileInformation> info =
      vtkSmartPointer<vtkPVFileInformation>::New();
    info->SetFullPath(szPath);
    info->SetName("Desktop");
    info->Type = DIRECTORY;
    this->Contents->AddItem(info);
    }

  if(SUCCEEDED(SHGetSpecialFolderPath(NULL, szPath, CSIDL_FAVORITES, false)))
    {
    vtkSmartPointer<vtkPVFileInformation> info =
      vtkSmartPointer<vtkPVFileInformation>::New();
    info->SetFullPath(szPath);
    info->SetName("Favorites");
    info->Type = DIRECTORY;
    this->Contents->AddItem(info);
    }

#endif // _WIN32_ID >= 0x0400

  // Return drive letters ...
  char strings[1024];
  DWORD n = GetLogicalDriveStrings(1024, strings);
  char* start = strings;
  char* end = start;
  for(;end != strings+n; ++end)
    {
    if(!*end)
      {
      vtkSmartPointer<vtkPVFileInformation> info =
        vtkSmartPointer<vtkPVFileInformation>::New();
      info->SetFullPath(start);
      info->SetName(start);
      info->Type = DRIVE;
      this->Contents->AddItem(info);
      start = end+1;
      }
    }

#else // _WIN32

  if(const char* home = getenv("HOME"))
    {
    vtkSmartPointer<vtkPVFileInformation> info =
      vtkSmartPointer<vtkPVFileInformation>::New();
    info->SetFullPath(home);
    info->SetName("Home");
    info->Type = DIRECTORY;
    this->Contents->AddItem(info);
    }

#endif // !_WIN32  
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::GetWindowsDirectoryListing()
{
#if defined(_WIN32)
  typedef vtkstd::set< vtkstd::string > DirectoriesType;
  typedef vtkstd::set< vtkstd::string > FilesType;
  DirectoriesType directories;
  FilesType files;

  // Search for all files in the given directory.
  vtkstd::string prefix = this->FullPath;
  vtkPVFileInformationAddTerminatingSlash(prefix);
  vtkstd::string pattern = prefix
  pattern += "*";
  WIN32_FIND_DATA data;
  HANDLE handle = FindFirstFile(pattern.c_str(), &data);
  if(handle == INVALID_HANDLE_VALUE)
    {
    LPVOID lpMsgBuf;
    FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
      GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&lpMsgBuf, 0, NULL);
    vtkErrorMacro("Error calling FindFirstFile : "
      << (char*)lpMsgBuf << "\nDirectory: " << prefix.c_str());
    LocalFree(lpMsgBuf);
    return;
    }
  int done = 0;
  while(!done)
    {
    // Look at this file.
    if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
      if(strcmp(data.cFileName, "..") != 0 &&
        strcmp(data.cFileName, ".") != 0)
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
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
      GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&lpMsgBuf, 0, NULL);
    vtkErrorMacro("Error calling FindNextFile : "
      << (char*)lpMsgBuf << "\nDirectory: " << prefix.c_str());
    LocalFree(lpMsgBuf);
    }

  if(!FindClose(handle))
    {
    vtkErrorMacro("Error calling FindClose.");
    }

  for (DirectoriesType::iterator iterD = directories.begin();
    iterD != directories.end(); ++iterD)
    {
    vtkPVFileInformation* infoD = vtkPVFileInformation::New();
    infoD->SetName(iterD->c_str());
    infoD->SetFullPath(prefix + iterD->c_str());
    infoD->Type = DIRECTORY;
    this->Contents->AddItem(infoD);
    infoD->Delete();
    }

  for (FilesType::iterator iterF = files.begin();
    iterF != files.end(); ++iterF)
    {
    vtkPVFileInformation* infoF = vtkPVFileInformation::New();
    infoF->SetName(iterF->c_str());
    infoF->SetFullPath(prefix + iterF->c_str());
    infoF->Type = SINGLE_FILE;
    this->Contents->AddItem(infoF);
    infoF->Delete();   
    }

  this->OrganizeCollection(this->Contents);

#else
  vtkErrorMacro("GetWindowsDirectoryListing cannot be called on non-Windows systems.");
#endif
}


//-----------------------------------------------------------------------------
void vtkPVFileInformation::GetDirectoryListing()
{
#if defined(_WIN32)

  vtkErrorMacro("GetDirectoryListing() cannot be called on Windows systems.");
  return;

#else

  typedef vtkstd::set<vtkstd::string> SetOfStrings;
  SetOfStrings contents;
  SetOfStrings directories;
  SetOfStrings files;

  vtkstd::string prefix = this->FullPath;
  vtkPVFileInformationAddTerminatingSlash(prefix);

  // Open the directory and make sure it exists.
  DIR* dir = opendir(this->FullPath);
  if(!dir)
    {
    // Could add check of errno here.
    return;
    }

  // Loop through the directory listing.
  while(const dirent* d = readdir(dir))
    {
    // Skip the special directory entries.
    if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
      {
      continue;
      }
    //contents.insert(d->d_name);
    vtkPVFileInformation* info = vtkPVFileInformation::New();
    info->SetName(d->d_name);
    info->SetFullPath((prefix + d->d_name).c_str());
    info->Type = INVALID;
    this->Contents->AddItem(info);
    info->Delete();
    }
  closedir(dir);

  this->OrganizeCollection(this->Contents);

  // Now we detect the file types for items.
  // We dissolve any groups that contain non-file items.
  
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(this->Contents->NewIterator());
 

  vtkstd::vector<vtkSmartPointer<vtkPVFileInformation> > result;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVFileInformation* obj = vtkPVFileInformation::SafeDownCast(
      iter->GetCurrentObject());
    if (obj->DetectType())
      {
      result.push_back(obj);
      }
    else
      {
      // Add children to result.
      for (int cc=0; cc < obj->Contents->GetNumberOfItems(); cc++)
        {
        vtkPVFileInformation* child = vtkPVFileInformation::SafeDownCast(
          obj->Contents->GetItemAsObject(cc));
        if (child->DetectType())
          {
          result.push_back(child);
          }
        }
      }
    }

  this->Contents->RemoveAllItems();
  vtkstd::vector<vtkSmartPointer<vtkPVFileInformation> >::iterator iter2;
  for (iter2 = result.begin(); iter2 != result.end(); ++iter2)
    {
    this->Contents->AddItem(*iter2);
    }
#endif
}


//-----------------------------------------------------------------------------
bool vtkPVFileInformation::DetectType()
{
  if (this->Type == FILE_GROUP)
    {
    vtkSmartPointer<vtkCollectionIterator> iter;
    iter.TakeReference(this->Contents->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkPVFileInformation* child = vtkPVFileInformation::SafeDownCast(
        iter->GetCurrentObject());
      if (!child->DetectType() || child->Type != SINGLE_FILE)
        {
        return false;
        }
      if (this->FastFileTypeDetection)
        {
        // Assume all children are same as this child.
        for (; !iter->IsDoneWithTraversal(); iter->GoToNextItem())
          {
          vtkPVFileInformation* child2 = vtkPVFileInformation::SafeDownCast(
            iter->GetCurrentObject());
          child2->Type = child->Type;
          }
        break;
        }
      }
    return true;
    }
  else if (this->Type == INVALID)
    {
    if (vtksys::SystemTools::FileExists(this->FullPath))
      {
      this-> Type = 
        (vtksys::SystemTools::FileIsDirectory(this->FullPath))?
        DIRECTORY : SINGLE_FILE;
      return true;
      }
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::OrganizeCollection(vtkCollection* collection)
{
  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPVFileInformation> > 
    MapOfStringToInfo;
  MapOfStringToInfo fileGroups;

  vtkstd::string prefix = this->FullPath;
  vtkPVFileInformationAddTerminatingSlash(prefix);

  vtkSmartPointer<vtkCollection> result = vtkSmartPointer<vtkCollection>::New();
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(collection->NewIterator());

  vtksys::RegularExpression reg_ex("^(.*)\\.([0-9.]+)$");

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVFileInformation* obj = vtkPVFileInformation::SafeDownCast(
      iter->GetCurrentObject());
    if (!obj || !obj->GetName())
      {
      continue;
      }

    if (obj->Type != FILE_GROUP && obj->Type != DIRECTORY 
      && reg_ex.find(obj->GetName()))
      {
      vtkstd::string groupName = reg_ex.match(1);
      vtkPVFileInformation* group = 0;
      if (fileGroups.find(groupName) == fileGroups.end())
        {
        group = vtkPVFileInformation::New();
        group->SetName(groupName.c_str());
        group->SetFullPath((prefix + groupName).c_str());
        group->Type = FILE_GROUP;
        fileGroups[groupName] = group;
        group->Delete();
        }
      else
        {
        group = fileGroups[groupName];
        }
      group->Contents->AddItem(obj);
      }
    else
      {
      result->AddItem(obj);
      }
    }

  // Now scan through all created groups and dissolve trivial groups
  // i.e. groups with single entries. Add all other groups to the 
  // results.
 for (MapOfStringToInfo::iterator iter2 = fileGroups.begin();
   iter2 != fileGroups.end(); ++iter2)
   {
   vtkPVFileInformation* group = iter2->second;
   if (group->Contents->GetNumberOfItems() > 1)
     {
     result->AddItem(group);
     }
   else
     {
     for (int cc=0; cc < group->Contents->GetNumberOfItems(); cc++)
       {
       result->AddItem(group->Contents->GetItemAsObject(cc));
       }
     }
   }
 collection->RemoveAllItems();

 iter.TakeReference(result->NewIterator());
 for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
   {
   collection->AddItem(iter->GetCurrentObject());
   }
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::CopyToStream(vtkClientServerStream* stream)
{
  *stream << vtkClientServerStream::Reply
    << this->Name
    << this->FullPath
    << this->Type
    << this->Contents->GetNumberOfItems();
  
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(this->Contents->NewIterator());
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkClientServerStream childStream;
    vtkPVFileInformation* child = vtkPVFileInformation::SafeDownCast(
      iter->GetCurrentObject());
    child->CopyToStream(&childStream);
    *stream << childStream;
    }
  *stream << vtkClientServerStream::End;
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::CopyFromStream(const vtkClientServerStream* css)
{
  this->Initialize();
  const char* temp = 0;
  if (!css->GetArgument(0, 0, &temp))
    {
    vtkErrorMacro("Error parsing Name.");
    return;
    }
  this->SetName(temp);

  if (!css->GetArgument(0, 1, &temp))
    {
    vtkErrorMacro("Error parsing FullPath.");
    return;
    }
  this->SetFullPath(temp);

  if (!css->GetArgument(0, 2, &this->Type))
    {
    vtkErrorMacro("Error parsing Type.");
    return;
    }

  int num_of_children =0;
  if (!css->GetArgument(0, 3, &num_of_children))
    {
    vtkErrorMacro("Error parsing Number of children.");
    return;
    }
  for (int cc=0; cc < num_of_children; cc++)
    {
    vtkPVFileInformation* child = vtkPVFileInformation::New();
    vtkClientServerStream childStream;
    if (!css->GetArgument(0, 4+cc, &childStream))
      {
      vtkErrorMacro("Error parsing child #" << cc);
      return;
      }
    child->CopyFromStream(&childStream);
    this->Contents->AddItem(child);
    child->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::Initialize()
{
  this->SetName(0);
  this->SetFullPath(0);
  this->Type = INVALID;
  this->Contents->RemoveAllItems();
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Name: " << (this->Name? this->Name: "(null)") << endl;
  os << indent << "FullPath: " 
    << (this->FullPath? this->FullPath : "(null)") << endl;
  os << indent << "Type: " ;
  switch (this->Type)
    {
  case INVALID:
    os << "INVALID" << endl;
    break;

  case SINGLE_FILE:
    os << "SINGLE_FILE" << endl;
    break;

  case DIRECTORY:
    os << "DIRECTORY" << endl;
    break;

  case FILE_GROUP:
    os << "FILE_GROUP" << endl;
    }
  for (int cc=0; cc < this->Contents->GetNumberOfItems(); cc++)
    {
    this->Contents->GetItemAsObject(cc)->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "FastFileTypeDetection: " << this->FastFileTypeDetection << endl;
}
