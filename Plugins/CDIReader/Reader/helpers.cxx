#include "helpers.h"
#include <sstream>

using namespace std;

string GetPathName(const string& s)
{
  char sep = '/';
#ifdef _WIN32
  sep = '\\';
#endif

  size_t i = s.rfind(sep, s.length());
  if (i != string::npos)
  {
    return (s.substr(0, i));
  }

  return ("");
}

string GetBasenameFromUri(const string& s)
{
  const char sep = '/';
  size_t i = s.rfind(sep, s.length());
  if (i != string::npos)
  {
    return (s.substr(i + 1));
  }
  string res = s;
  return (res);
}

bool CheckFileAccess(const std::string& name)
{
  if (FILE* file = fopen(name.c_str(), "r"))
  {
    fclose(file);
    return true;
  }
  else
  {
    return false;
  }
}

//-----------------------------------------------------------------------------
//  Function to convert cartesian coordinates to spherical, for use in
//  computing points in different layers of multilayer spherical view
//----------------------------------------------------------------------------

// Strip leading and trailing punctuation
// http://answers.yahoo.com/question/index?qid=20081226034047AAzf8lj
void Strip(string& s)
{
  string::iterator i = s.begin();
  while (ispunct(*i))
    s.erase(i);
  string::reverse_iterator j = s.rbegin();
  while (ispunct(*j))
  {
    s.resize(s.length() - 1);
    j = s.rbegin();
  }
}

string ConvertInt(int number)
{
  stringstream ss;
  ss << number;
  return ss.str();
}
