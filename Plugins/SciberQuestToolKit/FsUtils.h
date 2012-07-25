/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __fsutil_h
#define __fsutil_h

#include <vector>
using std::vector;
#include <string>
using std::string;
#include <iostream>
using std::ostream;
using std::cerr;
using std::endl;
#include <fstream>
using std::ifstream;
using std::ios;
#include <sstream>
using std::istringstream;
#include <cstdio>

#ifndef WIN32
  #define PATH_SEP "/"
#else
  #define PATH_SEP "\\"
#endif

void ToLower(string &in);
int FileExists(const char *path);
int Present(const char *path, const char *file);
int Represented(const char *path, const char *prefix);
int ScalarRepresented(const char *path, const char *prefix);
int VectorRepresented(const char *path, const char *prefix);
int SymetricTensorRepresented(const char *path, const char *prefix);
int TensorRepresented(const char *path, const char *prefix);
int GetSeriesIds(const char *path, const char *prefix, vector<int> &ids);
string StripFileNameFromPath(const string fileName);
string StripExtensionFromFileName(const string fileName);
string StripPathFromFileName(const string fileName);
size_t LoadLines(const char *fileName, vector<string> &lines);
size_t LoadText(const string &fileName, string &text);
int WriteText(string &fileName, string &text);
int SearchAndReplace(const string &searchFor,const string &replaceWith,string &inText);
ostream &operator<<(ostream &os, vector<string> v);
bool operator&(vector<string> &v, const string &s);


//*****************************************************************************
template<typename T>
size_t LoadBin(const char *fileName, size_t dlen, T *buffer)
{
  ifstream file(fileName,ios::binary);
  if (!file.is_open())
    {
    cerr << "ERROR: File " << fileName << " could not be opened." << endl;
    return 0;
    }

  // determine file size
  file.seekg(0,ios::end);
  size_t flen=file.tellg();
  file.seekg(0,ios::beg);

  // check if file size matches expected read size.
  if (dlen*sizeof(T)!=flen)
    {
    cerr
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
int NameValue(vector<string> &lines, string name, T &value)
{
  size_t nLines=lines.size();
  for (size_t i=0; i<nLines; ++i)
    {
    string tok;
    istringstream is(lines[i]);
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
size_t ParseValue(string &in,size_t at, string key, T &value)
{
  size_t p=in.find(key,at);
  if (p!=string::npos)
    {
    size_t n=key.size();

    // check to make sure match is the whole word
    if ((p!=0) && isalpha(in[p-1]) && isalpha(in[p+n]))
      {
      return string::npos;
      }
    // convert value
    const int maxValueLen=64;
    p+=n;
    istringstream valss(in.substr(p,maxValueLen));

    valss >> value;
    }
  return p;
}

#endif
