/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
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
using namespace std;

#ifndef WIN32
  #include <dirent.h>
  #define PATH_SEP "/"
#else
  #include "win_windirent.h"
  #define opendir win_opendir
  #define readdir win_readdir
  #define closedir win_closedir
  #define DIR win_DIR
  #define dirent win_dirent
  #define PATH_SEP "\\"
#endif



//******************************************************************************
int Present(const char *path, const char *fileName)
{
  ostringstream fn;
  fn << path << PATH_SEP << fileName;
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
    cerr << __LINE__ << " Error: prefix is expected to end with '_' but it does not." << endl;
    return 0;
    }
  #endif
  DIR *ds=opendir(path);
  if (ds)
    {
    struct dirent *de;
    while ((de=readdir(ds)))
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
    cerr << __LINE__ << " Error: Failed to open the given directory. " << endl
         << path << endl;
    }
  //We failed to find any files starting with the given prefix
  return 0;
}

// Collect the ids from a collection of files that start with
// the same prefix (eg. prefix_ID.ext). The prefix should include
// the underscore.
//*****************************************************************************
int GetSeriesIds(const char *path, const char *prefix, vector<int> &ids)
{
  size_t prefixLen=strlen(prefix);
  #ifndef NDEBUG
  if (prefix[prefixLen-1]!='_')
    {
    cerr << __LINE__ << " Error: prefix is expected to end with '_' but it does not." << endl;
    return 0;
    }
  #endif

  DIR *ds=opendir(path);
  if (ds)
    {
    struct dirent *de;
    while ((de=readdir(ds)))
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
    sort(ids.begin(),ids.end());
    return 1;
    }
  else
    {
    cerr << __LINE__ << " Error: Failed to open the given directory. " << endl
         << path << endl;
    }
  //We failed to find any files starting with the given prefix
  return 0;
}


// Returns the path not including the file name and not
// including the final PATH_SEP. If PATH_SEP isn't found 
// then ".PATH_SEP" is returned.
//*****************************************************************************
string StripFileNameFromPath(const string fileName)
{
  size_t p;
  p=fileName.find_last_of(PATH_SEP);
  if (p==string::npos)
    {
    // current directory
    return "." PATH_SEP;
    }
  return fileName.substr(0,p); // TODO Why does this leak?
}

// Returns the file name not including the extension (ie what ever is after
// the last ".". If there is no "." then the fileName is retnurned unmodified.
//*****************************************************************************
string StripExtensionFromFileName(const string fileName)
{
  size_t p;
  p=fileName.rfind(".");
  if (p==string::npos)
    {
    // current directory
    return fileName;
    }
  return fileName.substr(0,p); // TODO Why does this leak?
}

// Returns the file name from the given path. If PATH_SEP isn't found
// then the filename is returned unmodified.
//*****************************************************************************
string StripPathFromFileName(const string fileName)
{
  size_t p;
  p=fileName.find_last_of(PATH_SEP);
  if (p==string::npos)
    {
    // current directory
    return fileName;
    }
  return fileName.substr(p+1,string::npos); // TODO Why does this leak?
}


//*****************************************************************************
int LoadLines(const char *fileName, vector<string> &lines)
{
  // Load each line in the given file into a the vector.
  int nRead=0;
  const int bufSize=1024;
  char buf[bufSize]={'\0'};
  ifstream file(fileName);
  if (!file.is_open())
    {
    cerr << "ERROR: File " << fileName << " could not be opened." << endl;
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
int LoadText(const string &fileName, string &text)
{
  ifstream file(fileName.c_str());
  if (!file.is_open())
    {
    cerr << "ERROR: File " << fileName << " could not be opened." << endl;
    return 0;
    }
  // Determine the length of the file ...
  file.seekg (0,ios::end);
  size_t nBytes=file.tellg();
  file.seekg (0, ios::beg);
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
int WriteText(string &fileName, string &text)
{
  ofstream file(fileName.c_str());
  if (!file.is_open())
    {
    cerr << "ERROR: File " << fileName << " could not be opened." << endl;
    return 0;
    }
  file << text << endl;
  file.close();
  return 1;
}

//*****************************************************************************
int SearchAndReplace(
        const string &searchFor,
        const string &replaceWith,
        string &inText)
{
  int nFound=0;
  const size_t n=searchFor.size();
  size_t at=string::npos;
  while ((at=inText.find(searchFor))!=string::npos)
    {
    inText.replace(at,n,replaceWith);
    ++nFound;
    }
  return nFound;
}

//*****************************************************************************
ostream &operator<<(ostream &os, vector<string> v)
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
bool operator&(vector<string> &v, const string &s)
{
  // test for set inclusion.
  const size_t n=v.size();
  for (size_t i=0; i<n; ++i)
    {
    if (v[i]==s) return true;
    }
  return false;
}
