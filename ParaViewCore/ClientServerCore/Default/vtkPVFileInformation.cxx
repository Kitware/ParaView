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
#include "vtkFileSequenceParser.h"
#include "vtkObjectFactory.h"
#include "vtkPVFileInformationHelper.h"
#include "vtkSmartPointer.h"

#if defined(_WIN32)
# define _WIN32_IE 0x0400  // special folder support
# define _WIN32_WINNT 0x0400  // shared folder support
# include <windows.h>   // FindFirstFile, FindNextFile, FindClose, ...
# include <direct.h>    // _getcwd
# include <shlobj.h>    // SHGetFolderPath
# include <sys/stat.h>  // stat
# include <string.h>   // for strcasecmp
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
#if defined (__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#include <vector>
#endif

#include <vtksys/SystemTools.hxx>
#include <vtksys/RegularExpression.hxx>
#include <set>
#include <string>

vtkStandardNewMacro(vtkPVFileInformation);

inline void vtkPVFileInformationAddTerminatingSlash(std::string& name)
{
  if (name.size()>0)
    {
    char last = *(name.end()-1);
    if (last != '/' && last != '\\')
      {
#if defined(_WIN32)
      name += "\\";
#else
      name += "/";
#endif
      }
    }
}

#if defined(_WIN32)

static std::string WindowsNetworkRoot = "Windows Network";

// assumes native back-slashes
static bool IsUncPath(const std::string& path)
{
  if(path.size() >= 2 && path[0] == '\\' && path[1] == '\\')
    {
    return true;
    }
  return false;
}

static bool IsNetworkPath(const std::string& path)
{
  if(path.compare(0, WindowsNetworkRoot.size(), WindowsNetworkRoot) == 0)
    {
    return true;
    }
  return false;
}

// returns true if path is valid, false otherwise
// returns subdirs, if any

static bool getNetworkSubdirs(const std::string& name,
                              DWORD DisplayType,
                              std::vector<std::string>& subdirs)
{
  HANDLE han=0;
  NETRESOURCEA rc;
  rc.dwScope = RESOURCE_GLOBALNET;
  rc.dwType = RESOURCETYPE_ANY;  // TODO, restrict this to disk at server level?
  rc.dwDisplayType = DisplayType;
  rc.dwUsage = RESOURCEUSAGE_CONTAINER;
  if(DisplayType == RESOURCEDISPLAYTYPE_NETWORK)
    {
    rc.dwUsage |= RESOURCEUSAGE_RESERVED;  // wonder why this is needed?
    }
  rc.lpLocalName = NULL;
  rc.lpProvider = NULL;
  rc.lpComment = NULL;
  rc.lpRemoteName = const_cast<char*>(name.c_str());

  DWORD ret = WNetOpenEnumA(RESOURCE_GLOBALNET,
    RESOURCETYPE_ANY, 0, &rc, &han);

  if(NO_ERROR == ret)
    {
    DWORD count = 10;
    NETRESOURCE res[10];
    DWORD bufsize = sizeof(NETRESOURCE) * count;
    do
      {
      ret = WNetEnumResourceA(han, &count, res, &bufsize);
      if(ret == NO_ERROR || ret == ERROR_MORE_DATA)
        {
        for(DWORD i=0; i<count; i++)
          {
          std::string subdir = res[i].lpRemoteName;
          if(subdir.compare(0, name.size(), name) == 0)
            {
            subdir = subdir.c_str() + name.size() + 1;
            }
          subdirs.push_back(subdir);
          }
        }
      } while(ret == ERROR_MORE_DATA);
    }
  return NO_ERROR == ret;
}


static bool getNetworkSubdirs(const std::string& path,
                              std::vector<std::string>& subdirs,
                              int& type)
{
  if(!IsNetworkPath(path))
    {
    return false;
    }

  // path follows this convention
  // Windows Network\Domain\Server\Share
  // this doesn't return subdirectories of "Share", use normal
  // win32 API for that

  static const int MaxTokens = 4;

  std::vector<vtksys::String> pathtokens;
  pathtokens = vtksys::SystemTools::SplitString(path.c_str()+1, '\\');

  static DWORD DisplayType[MaxTokens] =
    {
    RESOURCEDISPLAYTYPE_NETWORK,
    RESOURCEDISPLAYTYPE_DOMAIN,
    RESOURCEDISPLAYTYPE_SERVER,
    RESOURCEDISPLAYTYPE_SHARE
    };

  int tokenIndex = static_cast<int>(pathtokens.size())-1;

  if(tokenIndex >= MaxTokens)
    return false;

  std::string name = pathtokens[tokenIndex];

  if(tokenIndex == 0)
    {
    type = vtkPVFileInformation::NETWORK_DOMAIN;
    }
  else if(tokenIndex == 1)
    {
    type = vtkPVFileInformation::NETWORK_SERVER;
    }
  else if(tokenIndex == 2)
    {
    type = vtkPVFileInformation::NETWORK_SHARE;
    name = "\\\\" + name;
    }

  return getNetworkSubdirs(name, DisplayType[tokenIndex], subdirs);
}

static bool getUncSharesOnServer(const std::string& server,
                          std::vector<std::string>& ret)
{
  return getNetworkSubdirs(std::string("\\\\") + server,
                         RESOURCEDISPLAYTYPE_SERVER, ret);
}

#endif

static int vtkPVFileInformationGetType(const char* path)
{
  int type = vtkPVFileInformation::INVALID;

  std::string realpath = path;

  if(vtksys::SystemTools::FileExists(realpath.c_str()))
    {
    type = vtkPVFileInformation::SINGLE_FILE;
    }
  if(vtksys::SystemTools::FileIsDirectory(realpath.c_str()))
    {
    type = vtkPVFileInformation::DIRECTORY;
    }
#if defined(_WIN32)
  // doing stat on root of devices doesn't work

  // is it the root of a drive?
  if (realpath.size() > 0 && realpath[1] == ':' && realpath.size() == 2 ||
      (realpath.size() == 3 && realpath[2] == '\\'))
    {
    // Path may be drive letter.
    DWORD n = GetLogicalDrives();
    int which = tolower(realpath[0]) - 'a';
    if(n & (1 << which))
      {
      type = vtkPVFileInformation::DRIVE;
      }
    }

  if(IsNetworkPath(realpath))
    {
    // this code doesn't give out anything with
    // "Windows Network\..." unless its a directory.
    // that may change
    std::vector<vtksys::String> pathtokens;
    pathtokens = vtksys::SystemTools::SplitString(realpath.c_str(), '\\');
    if(pathtokens.size() == 1)
      {
      type = vtkPVFileInformation::NETWORK_ROOT;
      }
    else if(pathtokens.size() == 2)
      {
      type = vtkPVFileInformation::NETWORK_DOMAIN;
      }
    else if(pathtokens.size() == 3)
      {
      type = vtkPVFileInformation::NETWORK_SERVER;
      }
    else
      {
      type = vtkPVFileInformation::NETWORK_SHARE;
      }
    }

  // is it the root of a shared folder?
  if(IsUncPath(realpath))
    {
    std::vector<vtksys::String> parts =
      vtksys::SystemTools::SplitString(realpath.c_str()+2, '\\', true);
    if(parts.empty())
      {
      // global network
      type = vtkPVFileInformation::DIRECTORY;
      }
    else
      {
      std::vector<std::string> shares;
      bool ret = getUncSharesOnServer(parts[0], shares);
      if(parts.size() == 1 && ret)
        {
        // server exists
        type = vtkPVFileInformation::NETWORK_SERVER;
        }
      else if(parts.size() == 2 && ret)
        {
        for(unsigned int i = 0; i<shares.size(); i++)
          {
          if(parts[1] == shares[i])
            {
            // share on server exists
            type = vtkPVFileInformation::DIRECTORY;
            break;
            }
          }
        }
      }
    }
#endif
  return type;
}

#if defined(_WIN32)
static std::string vtkPVFileInformationResolveLink(const std::string& fname,
                                                      WIN32_FIND_DATA& wfd)
{
  IShellLink* shellLink;
  HRESULT hr;
  char Link[MAX_PATH];
  bool coInit = false;
  std::string result;

  hr = ::CoCreateInstance(CLSID_ShellLink, NULL,
                          CLSCTX_INPROC_SERVER, IID_IShellLink,
                          (LPVOID*)&shellLink);
  if(hr == CO_E_NOTINITIALIZED)
    {
    coInit = true;
    ::CoInitialize(NULL);
    hr = ::CoCreateInstance(CLSID_ShellLink, NULL,
                            CLSCTX_INPROC_SERVER, IID_IShellLink,
                            (LPVOID*)&shellLink);
    }
  if(SUCCEEDED(hr))
    {
    IPersistFile* ppf;
    hr = shellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
    if(SUCCEEDED(hr))
      {
      std::wstring wfname(fname.begin(), fname.end());
      hr = ppf->Load((LPOLESTR)wfname.c_str(), STGM_READ);
      if(SUCCEEDED(hr))
        {
        if(shellLink->GetPath(Link, MAX_PATH, &wfd, SLGP_UNCPRIORITY) == NOERROR)
          {
          result = Link;
          }
        ppf->Release();
        }
      }
    shellLink->Release();
    }
  if(coInit)
    {
    CoUninitialize();
    }

  return result;

}
#endif

std::string MakeAbsolutePath(const std::string& path,
                            const std::string& working_dir)
{
  std::string ret = path;
#if defined(WIN32)
  if(!IsUncPath(path) && !IsNetworkPath(path))
#endif
    {
    ret = vtksys::SystemTools::CollapseFullPath(path.c_str(),
      working_dir.c_str());
    }
  return ret;
}


//-----------------------------------------------------------------------------
class vtkPVFileInformationSet :
  public std::set<vtkSmartPointer<vtkPVFileInformation> >
{
};

//-----------------------------------------------------------------------------
vtkPVFileInformation::vtkPVFileInformation()
{
  this->RootOnly = 1;
  this->Contents = vtkCollection::New();
  this->SequenceParser = vtkFileSequenceParser::New();
  this->Type = INVALID;
  this->Name = NULL;
  this->FullPath = NULL;
  this->FastFileTypeDetection = 0;
  this->Hidden = false;
}

//-----------------------------------------------------------------------------
vtkPVFileInformation::~vtkPVFileInformation()
{
  this->Contents->Delete();
  this->SequenceParser->Delete();
  this->SetName(NULL);
  this->SetFullPath(NULL);
}

bool vtkPVFileInformation::IsDirectory(int t)
{
  return t == DIRECTORY || t == DIRECTORY_LINK ||
         t == DRIVE || t == NETWORK_ROOT ||
         t == NETWORK_DOMAIN || t == NETWORK_SERVER ||
         t == NETWORK_SHARE;
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

  std::string working_directory =
    vtksys::SystemTools::GetCurrentWorkingDirectory().c_str();
  if (helper->GetWorkingDirectory() && helper->GetWorkingDirectory()[0])
    {
    working_directory = helper->GetWorkingDirectory();
    }

  std::string path = MakeAbsolutePath(helper->GetPath(), working_directory);

  this->SetName(helper->GetPath());
  bool isLink = false;

#if defined(_WIN32)
  std::string::size_type idx;
  for(idx = path.find('/', 0);
        idx != std::string::npos;
        idx = path.find('/', idx))
    {
    path.replace(idx, 1, 1, '\\');
    }

  int len = static_cast<int>(path.size());
  if(len > 4 && path.compare(len-4, 4, ".lnk") == 0)
    {
    WIN32_FIND_DATA data;
    path = vtkPVFileInformationResolveLink(path, data);
    isLink = true;
    }
#endif

  this->SetFullPath(path.c_str());

  this->Type = vtkPVFileInformationGetType(this->FullPath);
  if(isLink && this->Type == SINGLE_FILE)
    {
    this->Type = SINGLE_FILE_LINK;
    }
  else if(isLink && this->Type == DIRECTORY)
    {
    this->Type = DIRECTORY_LINK;
    }

  //determine if this is a hidden directory/file
  this->SetHiddenFlag();

  if (this->IsDirectory(this->Type) && helper->GetDirectoryListing())
    {
    // Since we want a directory listing, we now to platform specific listing
    // with intelligent pattern matching hee-haa.
#if defined(_WIN32)
    this->GetWindowsDirectoryListing();
#else
    this->GetDirectoryListing();
#endif
    }

}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::GetSpecialDirectories()
{
#if defined (_WIN32)

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

  // Return drive letters ...
  DWORD n = GetLogicalDrives();
  for(int i=0; i<sizeof(DWORD)*8; i++)
    {
    if(n & (1<<i))
      {
      std::string driveLetter;
      driveLetter += 'A' + i;
      driveLetter += ":\\";
      vtkSmartPointer<vtkPVFileInformation> info =
        vtkSmartPointer<vtkPVFileInformation>::New();
      info->SetFullPath(driveLetter.c_str());
      info->SetName(driveLetter.c_str());
      info->Type = DRIVE;
      this->Contents->AddItem(info);
      }
    }

  vtkSmartPointer<vtkPVFileInformation> info =
      vtkSmartPointer<vtkPVFileInformation>::New();
  info->SetFullPath(WindowsNetworkRoot.c_str());
  info->SetName("Windows Network");
  info->Type = NETWORK_ROOT;
  this->Contents->AddItem(info);

#else // _WIN32
#if defined (__APPLE__ )
  //-------- Get the List of Mounted Volumes from the System

  int idx = 1;
  HFSUniStr255 hfsname;
  FSRef ref;
  while (noErr == FSGetVolumeInfo(kFSInvalidVolumeRefNum, idx++, NULL,
      kFSVolInfoNone, NULL, &hfsname, &ref))
    {
    CFURLRef resolvedUrl = CFURLCreateFromFSRef(NULL, &ref);
    if (resolvedUrl)
      {
      CFStringRef url;
      url = CFURLCopyFileSystemPath(resolvedUrl, kCFURLPOSIXPathStyle);
      if(url)
        {
        CFStringRef cfname = CFStringCreateWithCharacters(kCFAllocatorDefault,
            hfsname.unicode, hfsname.length);

        CFIndex pathSize = CFStringGetLength(url)+1;
        std::vector<char> pathChars(pathSize, 0);
        OSStatus pathStatus = CFStringGetCString(url, &pathChars[0], pathSize,
            kCFStringEncodingASCII);

        pathSize = CFStringGetLength(cfname)+1;
        std::vector<char> nameChars(pathSize, 0);
        OSStatus nameStatus = CFStringGetCString(cfname, &nameChars[0], pathSize,
            kCFStringEncodingASCII);

        if (pathStatus && nameStatus)
          {
          vtkSmartPointer<vtkPVFileInformation> info = vtkSmartPointer<
              vtkPVFileInformation>::New();
          info->SetFullPath( &(pathChars.front() ));
          info->SetName( &(nameChars.front() ));
          info->Type = DRIVE;
          this->Contents->AddItem(info);
          }
        CFRelease(cfname);
        }
      CFRelease(resolvedUrl);
      }
    }
  //-- Read the com.apple.sidebar.plist file to get the user's list of directories
  CFPropertyListRef p = CFPreferencesCopyAppValue(CFSTR("useritems"),
      CFSTR("com.apple.sidebarlists"));
  if (p && CFDictionaryGetTypeID() == CFGetTypeID(p))
    {
    CFArrayRef r = (CFArrayRef)(CFDictionaryGetValue((CFDictionaryRef)p,
        CFSTR("CustomListItems")));
    if (r && CFArrayGetTypeID() == CFGetTypeID(r))
      {
      int count = CFArrayGetCount(r);
      for (int i=0; i<count; i++)
        {
        CFDictionaryRef dr = (CFDictionaryRef)CFArrayGetValueAtIndex(r, i);
        if (dr && CFDictionaryGetTypeID() == CFGetTypeID(dr))
          {
          CFStringRef name = 0;
          CFStringRef url = 0;
          CFDataRef alias;
          if (CFDictionaryGetValueIfPresent(dr, CFSTR("Name"),
              (const void**)&name) && CFDictionaryGetValueIfPresent(dr,
              CFSTR("Alias"), (const void**)&alias) && name && alias
              && CFStringGetTypeID() == CFGetTypeID(name) && CFDataGetTypeID()
              == CFGetTypeID(alias) )
            {
            CFIndex dataSize = CFDataGetLength(alias);
            AliasHandle tAliasHdl = (AliasHandle) NewHandle(dataSize);
            if (tAliasHdl)
              {
              CFDataGetBytes(alias, CFRangeMake( 0, dataSize),
                  ( UInt8*) *tAliasHdl );
              FSRef tFSRef;
              Boolean changed;
              if (noErr == FSResolveAlias(NULL, tAliasHdl, &tFSRef, &changed))
                {
                CFURLRef resolvedUrl = CFURLCreateFromFSRef(NULL, &tFSRef);
                if (resolvedUrl)
                  {
                  url = CFURLCopyFileSystemPath(resolvedUrl,
                      kCFURLPOSIXPathStyle);
                  CFRelease(resolvedUrl);
                  }
                }
              DisposeHandle((Handle)tAliasHdl);
              }

            if(!url || !name)
              {
              continue;
              }

            // now put the name and path into a FileInfo Object
            CFIndex pathSize = CFStringGetLength(url)+1;
            std::vector<char> pathChars(pathSize, 0);
            OSStatus pathStatus = CFStringGetCString(url, &pathChars[0],
                pathSize, kCFStringEncodingASCII);

            pathSize = CFStringGetLength(name)+1;
            std::vector<char> nameChars(pathSize, 0);
            OSStatus nameStatus = CFStringGetCString(name, &nameChars[0],
                pathSize, kCFStringEncodingASCII);

            if (pathStatus && nameStatus)
              {
              vtkSmartPointer<vtkPVFileInformation> info = vtkSmartPointer<
                  vtkPVFileInformation>::New();
              info->SetFullPath( &(pathChars.front() ));
              info->SetName( &(nameChars.front() ));
              info->Type = DIRECTORY;
              this->Contents->AddItem(info);
              }
            }
          }
        }
      }
    }
#else
  if(const char* home = getenv("HOME"))
    {
    vtkSmartPointer<vtkPVFileInformation> info =
      vtkSmartPointer<vtkPVFileInformation>::New();
    info->SetFullPath(home);
    info->SetName("Home");
    info->Type = DIRECTORY;
    this->Contents->AddItem(info);
    }
#endif
#endif // !_WIN32
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::GetWindowsDirectoryListing()
{
#if defined(_WIN32)
  vtkPVFileInformationSet info_set;

  if(IsNetworkPath(this->FullPath))
    {
    std::vector<std::string> shares;
    int type;
    if(getNetworkSubdirs(this->FullPath, shares, type))
      {
      for(unsigned int i=0; i<shares.size(); i++)
        {
        vtkPVFileInformation* info = vtkPVFileInformation::New();
        info->SetName(shares[i].c_str());
        std::string fullpath =
          vtksys::SystemTools::CollapseFullPath(shares[i].c_str(), this->FullPath);
        info->SetFullPath(fullpath.c_str());
        info->Type = type;
        info->FastFileTypeDetection = this->FastFileTypeDetection;
        info_set.insert(info);
        info->Delete();
        }
      }
    this->OrganizeCollection(info_set);

    for (vtkPVFileInformationSet::iterator iter = info_set.begin();
      iter != info_set.end(); ++iter)
      {
      this->Contents->AddItem(*iter);
      }
    return;
    }

  if(IsUncPath(this->FullPath))
    {
    bool didListing = false;
    std::vector<vtksys::String> parts =
      vtksys::SystemTools::SplitString(this->FullPath+2, '\\', true);

    if(parts.size() == 1)
      {
      // get list of all shares on server
      std::vector<std::string> shares;
      if(getUncSharesOnServer(parts[0], shares))
        {
        for(unsigned int i=0; i<shares.size(); i++)
          {
          vtkPVFileInformation* info = vtkPVFileInformation::New();
          info->SetName(shares[i].c_str());
          std::string fullpath = "\\\\" + parts[0] + "\\" + shares[i];
          info->SetFullPath(fullpath.c_str());
          info->Type = NETWORK_SHARE;
          info->FastFileTypeDetection = this->FastFileTypeDetection;
          info_set.insert(info);
          info->Delete();
          }
        }
      didListing = true;
      }

    if(didListing)
      {
      this->OrganizeCollection(info_set);

      for (vtkPVFileInformationSet::iterator iter = info_set.begin();
        iter != info_set.end(); ++iter)
        {
        this->Contents->AddItem(*iter);
        }
      return;
      }
    // fall through for normal file listing that works after shares are
    // known
    }

  // Search for all files in the given directory.
  std::string prefix = this->FullPath;
  vtkPVFileInformationAddTerminatingSlash(prefix);
  std::string pattern = prefix;
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

  do
    {
    std::string filename = data.cFileName;
    if(filename == "." || filename == "..")
      continue;
    std::string fullpath = prefix + filename;
    size_t len = filename.size();
    bool isLink = false;

    if(len > 4 && strncmp(filename.c_str()+len-4, ".lnk", 4) == 0)
      {
      fullpath = vtkPVFileInformationResolveLink(fullpath, data);
      filename.resize(filename.size() - 4);
      isLink = true;
      }

    DWORD isdir = data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    DWORD isfile = (data.dwFileAttributes & FILE_ATTRIBUTE_NORMAL) ||
                   (!(data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) &&
                   !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));

    FileTypes type = isdir ? DIRECTORY : SINGLE_FILE;
    if(isLink)
      {
      type = type == DIRECTORY ? DIRECTORY_LINK : SINGLE_FILE_LINK;
      }

    if(isdir || isfile)
      {
      vtkPVFileInformation* infoD = vtkPVFileInformation::New();
      infoD->SetName(filename.c_str());
      infoD->SetFullPath(fullpath.c_str());
      infoD->Type = type;
      infoD->FastFileTypeDetection = this->FastFileTypeDetection;
      infoD->SetHiddenFlag(); //needs full path set first
      info_set.insert(infoD);
      infoD->Delete();
      }

    // Find the next file.
    } while(FindNextFile(handle, &data) != 0);

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

  this->OrganizeCollection(info_set);

  for (vtkPVFileInformationSet::iterator iter = info_set.begin();
    iter != info_set.end(); ++iter)
    {
    this->Contents->AddItem(*iter);
    }

#else
  vtkErrorMacro("GetWindowsDirectoryListing cannot be called on non-Windows systems.");
#endif
}

/* There is a problem with the Portland compiler, large file
support and glibc/Linux system headers:
             http://www.pgroup.com/userforum/viewtopic.php?
             p=1992&sid=f16167f51964f1a68fe5041b8eb213b6
*/
#if defined(__PGI) && defined(__USE_FILE_OFFSET64)
# define dirent dirent64
#endif

//-----------------------------------------------------------------------------
void vtkPVFileInformation::GetDirectoryListing()
{
#if defined(_WIN32)

  vtkErrorMacro("GetDirectoryListing() cannot be called on Windows systems.");
  return;

#else

  vtkPVFileInformationSet info_set;
  std::string prefix = this->FullPath;
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
    vtkPVFileInformation* info = vtkPVFileInformation::New();
    info->SetName(d->d_name);
    info->SetFullPath((prefix + d->d_name).c_str());
    info->Type = INVALID;
    info->SetHiddenFlag();

    // fix to bug #09452 such that directories with trailing names can be
    // shown in the file dialog
#if defined (__SVR4) && defined (__sun)
  struct stat status;
  int res = stat( info->FullPath, &status );
  if ( res != -1 && status.st_mode & S_IFDIR )
    {
    info->Type = DIRECTORY;
    }
#else
  if ( d->d_type & DT_DIR )
    {
    info->Type = DIRECTORY;
    }
#endif

    info->FastFileTypeDetection = this->FastFileTypeDetection;
    info_set.insert(info);
    info->Delete();
    }
  closedir(dir);

  this->OrganizeCollection(info_set);

  // Now we detect the file types for items.
  // We dissolve any groups that contain non-file items.

  for (vtkPVFileInformationSet::iterator iter = info_set.begin();
    iter != info_set.end(); ++iter)
    {
    vtkPVFileInformation* obj = (*iter);
    if (obj->DetectType())
      {
      this->Contents->AddItem(obj);
      }
    else
      {
      // Add children to contents.
      for (int cc=0; cc < obj->Contents->GetNumberOfItems(); cc++)
        {
        vtkPVFileInformation* child = vtkPVFileInformation::SafeDownCast(
          obj->Contents->GetItemAsObject(cc));
        if (child->DetectType())
          {
          this->Contents->AddItem(child);
          }
        }
      }
    }
#endif
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::SetHiddenFlag( )
{
#if defined(_WIN32)
  if ( !this->FullPath )
    {
    this->Hidden = false;
    return;
    }
  LPCSTR fp = this->FullPath;
  DWORD flags= GetFileAttributes(fp);
  this->Hidden =( flags & FILE_ATTRIBUTE_HIDDEN) ? true: false;
#else
  if ( !this->Name )
    {
    this->Hidden = false;
    return;
    }
  this->Hidden =( this->Name[0] == '.' )? true:false;
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

struct vtkPVFileInformation::vtkInfo
{
  typedef std::map<int, vtkSmartPointer<vtkPVFileInformation> > ChildrenType;
  vtkSmartPointer<vtkPVFileInformation> Group;
  ChildrenType Children;

};

//-----------------------------------------------------------------------------
void vtkPVFileInformation::OrganizeCollection(vtkPVFileInformationSet& info_set)
{
  typedef std::map<std::string, vtkInfo> MapOfStringToInfo;
  MapOfStringToInfo fileGroups;

  std::string prefix = this->FullPath;
  vtkPVFileInformationAddTerminatingSlash(prefix);

  for (vtkPVFileInformationSet::iterator iter = info_set.begin();
    iter != info_set.end(); )
    {
    vtkPVFileInformation* obj = *iter;

    if (obj->Type != FILE_GROUP && !IsDirectory(obj->Type))
      {
      bool match = false;

      match = this->SequenceParser->ParseFileSequence(obj->GetName());

      if (match)
        {
        std::string groupName = this->SequenceParser->GetSequenceName();
        int groupIndex = this->SequenceParser->GetSequenceIndex();

        MapOfStringToInfo::iterator iter2 = fileGroups.find(groupName);
        vtkPVFileInformation* group = 0;
        if (iter2 == fileGroups.end())
          {
          group = vtkPVFileInformation::New();
          group->SetName(groupName.c_str());
          group->SetFullPath((prefix + groupName).c_str());
          group->Type = FILE_GROUP;
          //the group inherits the hidden flag of the first item in the group
          group->Hidden = obj->Hidden;
          group->FastFileTypeDetection = this->FastFileTypeDetection;
          //fileGroups[groupName] = group;
          vtkInfo info;
          info.Group = group;
          fileGroups[groupName] = info;
          group->Delete();

          iter2 = fileGroups.find(groupName);
          }

        iter2->second.Children[groupIndex] = obj;
        vtkPVFileInformationSet::iterator prev_iter = iter++;
        info_set.erase(prev_iter);
        continue;
        }
      }
    ++iter;
    }


  // Now scan through all created groups and dissolve trivial groups
  // i.e. groups with single entries. Add all other groups to the
  // results.
 for (MapOfStringToInfo::iterator iter2 = fileGroups.begin();
   iter2 != fileGroups.end(); ++iter2)
   {
   vtkInfo& info = iter2->second;
   vtkPVFileInformation* group = info.Group;
   if (info.Children.size() > 1)
     {
     vtkInfo::ChildrenType::iterator childIter = info.Children.begin();
     for (; childIter != info.Children.end();++childIter)
       {
       group->Contents->AddItem(childIter->second.GetPointer());
       }
     // Build group children.
     info_set.insert(group);
     }
   else
     {
     vtkInfo::ChildrenType::iterator childIter = info.Children.begin();
     for (; childIter != info.Children.end();++childIter)
       {
       info_set.insert(
         childIter->second.GetPointer());
       }
     }
   }
}

//-----------------------------------------------------------------------------
void vtkPVFileInformation::CopyToStream(vtkClientServerStream* stream)
{
  *stream << vtkClientServerStream::Reply
    << this->Name
    << this->FullPath
    << this->Type
    << this->Hidden
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

  if (!css->GetArgument(0, 3, &this->Hidden))
    {
    vtkErrorMacro("Error parsing Hidden.");
    return;
    }

  int num_of_children =0;
  if (!css->GetArgument(0, 4, &num_of_children))
    {
    vtkErrorMacro("Error parsing Number of children.");
    return;
    }
  for (int cc=0; cc < num_of_children; cc++)
    {
    vtkPVFileInformation* child = vtkPVFileInformation::New();
    vtkClientServerStream childStream;
    if (!css->GetArgument(0, 5+cc, &childStream))
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
  this->Hidden = false;
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
  os << indent << "Hidden: "<< this->Hidden << endl;
  os << indent << "FastFileTypeDetection: " << this->FastFileTypeDetection << endl;

  for (int cc=0; cc < this->Contents->GetNumberOfItems(); cc++)
    {
    os << endl;
    this->Contents->GetItemAsObject(cc)->PrintSelf(os, indent.GetNextIndent());
    }
}
