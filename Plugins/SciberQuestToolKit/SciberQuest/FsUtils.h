/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __fsutil_h
#define __fsutil_h

#include <vector> // for vector
#include <string> // for string
#include <iostream> // for ostream
#include <fstream> // for fstream
#include <sstream> // for istringstream

#ifndef WIN32
  #define PATH_SEP "/"
#else
  #define PATH_SEP "\\"
#endif

void ToLower(std::string &in);
int FileExists(const char *path);
int Present(const char *path, const char *file, const char *ext);
int Represented(const char *path, const char *prefix);
int ScalarRepresented(const char *path, const char *prefix);
int VectorRepresented(const char *path, const char *prefix);
int SymetricTensorRepresented(const char *path, const char *prefix);
int TensorRepresented(const char *path, const char *prefix);
int GetSeriesIds(const char *path, const char *prefix, std::vector<int> &ids);
std::string StripFileNameFromPath(const std::string fileName);
std::string StripExtensionFromFileName(const std::string fileName);
std::string StripPathFromFileName(const std::string fileName);
size_t LoadLines(const char *fileName, std::vector<std::string> &lines);
size_t LoadText(const std::string &fileName, std::string &text);
int WriteText(std::string &fileName, std::string &text);
int SearchAndReplace(const std::string &searchFor,const std::string &replaceWith,std::string &inText);
std::ostream &operator<<(std::ostream &os, std::vector<std::string> v);
bool operator&(std::vector<std::string> &v, const std::string &s);


//*****************************************************************************
template<typename T>
size_t LoadBin(const char *fileName, size_t dlen, T *buffer)
{
  std::ifstream file(fileName,std::ios::binary);
  if (!file.is_open())
    {
    std::cerr << "ERROR: File " << fileName << " could not be opened." << std::endl;
    return 0;
    }

  // determine file size
  file.seekg(0,std::ios::end);
  size_t flen=file.tellg();
  file.seekg(0,std::ios::beg);

  // check if file size matches expected read size.
  if (dlen*sizeof(T)!=flen)
    {
    std::cerr
      << "ERROR: Expected " << dlen << " bytes but found "
      << flen << " bytes in \"" << fileName << "\".";
    return 0;
    }

  // read
  file.read((char*)buffer,flen);
  file.close();

  // return the data, it's up to the caller to free.
  return dlen;
}

/**
*/
// ****************************************************************************
template<typename T>
int NameValue(std::vector<std::string> &lines, std::string name, T &value)
{
  size_t nLines=lines.size();
  for (size_t i=0; i<nLines; ++i)
    {
    std::string tok;
    std::istringstream is(lines[i]);
    is >> tok;
    if (tok==name)
      {
      is >> value;
      return 1;
      }
    }
  return 0;
}

/**
Parse a string for a "key", starting at offset "at" then
advance past the key and attempt to convert what follows
in to a value of type "T". If the key isn't found, then
npos is returned otherwise the position imediately following
the key is returned.
*/
// ****************************************************************************
template <typename T>
size_t ParseValue(std::string &in,size_t at, std::string key, T &value)
{
  size_t p=in.find(key,at);
  if (p!=std::string::npos)
    {
    size_t n=key.size();

    // check to make sure match is the whole word
    if ((p!=0) && isalpha(in[p-1]) && isalpha(in[p+n]))
      {
      return std::string::npos;
      }
    // convert value
    const int maxValueLen=64;
    p+=n;
    std::istringstream valss(in.substr(p,maxValueLen));

    valss >> value;
    }
  return p;
}

#endif

// VTK-HeaderTest-Exclude: FsUtils.h
