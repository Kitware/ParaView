/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#ifndef WIN32
  #include <dirent.h>
  #define PATH_SEP "/"
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
#else
  #include "win_windirent.h"
  #define opendir win_opendir
  #define readdir win_readdir
  #define closedir win_closedir
  #define DIR win_DIR
  #define dirent win_dirent
  #define PATH_SEP "\\"
#endif

//*****************************************************************************
void ToLower(std::string &in)
{
  size_t n=in.size();
  for (size_t i=0; i<n; ++i)
    {
    in[i]=(char)tolower(in[i]);
    }
}

//******************************************************************************
int FileExists(const char *path)
{
  #ifndef WIN32
  struct stat s;
  int iErr=stat(path,&s);
  if (iErr==0)
    {
    return 1;
    }
  #else
  (void)path;
  #endif
  return 0;
}

//******************************************************************************
int Present(const char *path, const char *fileName, const char *ext)
{
  std::ostringstream fn;
  fn << path << PATH_SEP << fileName << "." << ext;
  FILE *fp=fopen(fn.str().c_str(),"r");
  if (fp==0)
    {
    // file is not present.
    return 0;
    }
  // file is present.
  fclose(fp);
  return 1;
}

// Return 1 if a file is found with the prefix in the directory given by path.
//******************************************************************************
int Represented(const char *path, const char *prefix)
{
  size_t prefixLen=strlen(prefix);
  #ifndef NDEBUG
  if (prefix[prefixLen-1]!='_')
    {
    std::cerr << __LINE__ << " Error: prefix is expected to end with '_' but it does not." << std::endl;
    return 0;
    }
  #endif
  DIR *ds=opendir(path);
  if (ds)
    {
    struct dirent *de;
    while ((de=readdir(ds))!=0)
      {
      char *fname=de->d_name;
      if (strncmp(fname,prefix,prefixLen)==0)
        {
        // Found at least one file beginning with the given prefix.
        closedir(ds);
        return 1;
        }
      }
    closedir(ds);
    }
  else
    {
    std::cerr << __LINE__ << " Error: Failed to open the given directory. " << std::endl
              << path << std::endl;
    }
  //We failed to find any files starting with the given prefix
  return 0;
}

//******************************************************************************
int ScalarRepresented(const char *path, const char *scalar)
{
  std::string prefix(scalar);
  prefix+="_";

  return Represented(path,prefix.c_str());
}

//******************************************************************************
int VectorRepresented(const char *path, const char *vector)
{
  std::string xprefix(vector);
  xprefix+="x_";

  std::string yprefix(vector);
  yprefix+="y_";

  std::string zprefix(vector);
  zprefix+="z_";

  return
    Represented(path,xprefix.c_str())
    && Represented(path,yprefix.c_str())
    && Represented(path,zprefix.c_str());
}

//******************************************************************************
int SymetricTensorRepresented(const char *path, const char *tensor)
{
  std::string xxprefix(tensor);
  xxprefix+="-xx_";

  std::string xyprefix(tensor);
  xyprefix+="-xy_";

  std::string xzprefix(tensor);
  xzprefix+="-xz_";

  std::string yyprefix(tensor);
  yyprefix+="-yy_";

  std::string yzprefix(tensor);
  yzprefix+="-yz_";

  std::string zzprefix(tensor);
  zzprefix+="-zz_";

  return
    Represented(path,xxprefix.c_str())
    && Represented(path,xyprefix.c_str())
    && Represented(path,xzprefix.c_str())
    && Represented(path,yyprefix.c_str())
    && Represented(path,yzprefix.c_str())
    && Represented(path,zzprefix.c_str());
}

//******************************************************************************
int TensorRepresented(const char *path, const char *tensor)
{
  std::string xxprefix(tensor);
  xxprefix+="-xx_";

  std::string xyprefix(tensor);
  xyprefix+="-xy_";

  std::string xzprefix(tensor);
  xzprefix+="-xz_";

  std::string yxprefix(tensor);
  yxprefix+="-yx_";

  std::string yyprefix(tensor);
  yyprefix+="-yy_";

  std::string yzprefix(tensor);
  yzprefix+="-yz_";

  std::string zxprefix(tensor);
  zxprefix+="-zx_";

  std::string zyprefix(tensor);
  zyprefix+="-zy_";

  std::string zzprefix(tensor);
  zzprefix+="-zz_";

  return
    Represented(path,xxprefix.c_str())
    && Represented(path,xyprefix.c_str())
    && Represented(path,xzprefix.c_str())
    && Represented(path,yxprefix.c_str())
    && Represented(path,yyprefix.c_str())
    && Represented(path,yzprefix.c_str())
    && Represented(path,zxprefix.c_str())
    && Represented(path,zyprefix.c_str())
    && Represented(path,zzprefix.c_str());
}

// Collect the ids from a collection of files that start with
// the same prefix (eg. prefix_ID.ext). The prefix should include
// the underscore.
//*****************************************************************************
int GetSeriesIds(const char *path, const char *prefix, std::vector<int> &ids)
{
  size_t prefixLen=strlen(prefix);
  #ifndef NDEBUG
  if (prefix[prefixLen-1]!='_')
    {
    std::cerr << __LINE__ << " Error: prefix is expected to end with '_' but it does not." << std::endl;
    return 0;
    }
  #endif

  DIR *ds=opendir(path);
  if (ds)
    {
    struct dirent *de;
    while ((de=readdir(ds))!=0)
      {
      char *fname=de->d_name;
      if (strncmp(fname,prefix,prefixLen)==0)
        {
        if (isdigit(fname[prefixLen]))
          {
          int id=atoi(fname+prefixLen);
          ids.push_back(id);
          }
        }
      }
    closedir(ds);
    std::sort(ids.begin(),ids.end());
    return 1;
    }
  else
    {
    std::cerr << __LINE__ << " Error: Failed to open the given directory. " << std::endl
              << path << std::endl;
    }
  //We failed to find any files starting with the given prefix
  return 0;
}


// Returns the path not including the file name and not
// including the final PATH_SEP. If PATH_SEP isn't found
// then ".PATH_SEP" is returned.
//*****************************************************************************
std::string StripFileNameFromPath(const std::string fileName)
{
  size_t p;
  p=fileName.find_last_of(PATH_SEP);
  if (p==std::string::npos)
    {
    // current directory
    return "." PATH_SEP;
    }
  return fileName.substr(0,p); // TODO Why does this leak?
}

// Returns the file name not including the extension (ie what ever is after
// the last ".". If there is no "." then the fileName is retnurned unmodified.
//*****************************************************************************
std::string StripExtensionFromFileName(const std::string fileName)
{
  size_t p;
  p=fileName.rfind(".");
  if (p==std::string::npos)
    {
    // current directory
    return fileName;
    }
  return fileName.substr(0,p);
}

// Returns the file name from the given path. If PATH_SEP isn't found
// then the filename is returned unmodified.
//*****************************************************************************
std::string StripPathFromFileName(const std::string fileName)
{
  size_t p;
  p=fileName.find_last_of(PATH_SEP);
  if (p==std::string::npos)
    {
    // current directory
    return fileName;
    }
  return fileName.substr(p+1,std::string::npos); // TODO Why does this leak?
}


//*****************************************************************************
size_t LoadLines(const char *fileName, std::vector<std::string> &lines)
{
  // Load each line in the given file into a the vector.
  size_t nRead=0;
  const int bufSize=1024;
  char buf[bufSize]={'\0'};
  std::ifstream file(fileName);
  if (!file.is_open())
    {
    std::cerr << "ERROR: File " << fileName << " could not be opened." << std::endl;
    return 0;
    }
  while(file.good())
    {
    file.getline(buf,bufSize);
    if (file.gcount()>1)
      {
      lines.push_back(buf);
      ++nRead;
      }
    }
  file.close();
  return nRead;
}

//*****************************************************************************
size_t LoadText(const std::string &fileName, std::string &text)
{
  std::ifstream file(fileName.c_str());
  if (!file.is_open())
    {
    std::cerr << "ERROR: File " << fileName << " could not be opened." << std::endl;
    return 0;
    }
  // Determine the length of the file ...
  file.seekg (0,std::ios::end);
  size_t nBytes=(size_t)file.tellg();
  file.seekg (0, std::ios::beg);
  // and allocate a buffer to hold its contents.
  char *buf=new char [nBytes];
  memset(buf,0,nBytes);
  // Read the file and convert to a string.
  file.read (buf,nBytes);
  file.close();
  text=buf;
  return nBytes;
}

//*****************************************************************************
int WriteText(std::string &fileName, std::string &text)
{
  std::ofstream file(fileName.c_str());
  if (!file.is_open())
    {
    std::cerr << "ERROR: File " << fileName << " could not be opened." << std::endl;
    return 0;
    }
  file << text << std::endl;
  file.close();
  return 1;
}

//*****************************************************************************
int SearchAndReplace(
        const std::string &searchFor,
        const std::string &replaceWith,
        std::string &inText)
{
  int nFound=0;
  const size_t n=searchFor.size();
  size_t at=std::string::npos;
  while ((at=inText.find(searchFor))!=std::string::npos)
    {
    inText.replace(at,n,replaceWith);
    ++nFound;
    }
  return nFound;
}

//*****************************************************************************
std::ostream &operator<<(std::ostream &os, std::vector<std::string> v)
{
  size_t n=v.size();
  if (n>0)
    {
    os << v[0];
    for (size_t i=1; i<n; ++i)
      {
      os << " " << v[i];
      }
    }
  return os;
}

//*****************************************************************************
bool operator&(std::vector<std::string> &v, const std::string &s)
{
  // test for set inclusion.
  const size_t n=v.size();
  for (size_t i=0; i<n; ++i)
    {
    if (v[i]==s) return true;
    }
  return false;
}
