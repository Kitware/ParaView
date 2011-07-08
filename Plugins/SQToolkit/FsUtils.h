/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
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


#ifndef WIN32
  #define PATH_SEP "/"
#else
  #define PATH_SEP "\\"
#endif

int Present(const char *path, const char *file);
int Represented(const char *path, const char *prefix);
int GetSeriesIds(const char *path, const char *prefix, vector<int> &ids);
string StripFileNameFromPath(const string fileName);
string StripExtensionFromFileName(const string fileName);
string StripPathFromFileName(const string fileName);
int LoadLines(const char *fileName, vector<string> &lines);
int LoadText(const string &fileName, string &text);
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


//*****************************************************************************
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


#endif
