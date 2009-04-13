#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;

#include "PluginManager.h"
#include "DatabasePluginManager.h"
#include "DatabasePluginInfo.h"
#include "VisItException.h"


const char *dbTypeStr[] = {
    "STSD",
    "STMD",
    "MTSD",
    "MTMD",
    "CUSTOM"};


ostream &operator<<(ostream &os, vector<string> v)
{
  os << "[";
  size_t n=v.size();
  if (n>0)
    {
    os << v[0];
    for (size_t i=1; i<n; ++i)
      {
      os << ", " << v[i];
      }
    }
  os << "]";
  return os;
}


int main(int argc, char **argv)
{
  const char pluginDir[]="/home/burlen/ext2/v3/visit1.10.0/src/plugins";

  try
    {
    DatabasePluginManager dbm;
    dbm.Initialize(PluginManager::MDServer, false, pluginDir);

    int nPlugins=dbm.GetNAllPlugins();

    dbm.LoadPluginsNow();

    for (int i=0; i<nPlugins; ++i)
      {
      string id=dbm.GetAllID(i);
      CommonDatabasePluginInfo *info=dbm.GetCommonPluginInfo(id);
      if (info) 
        {
        cerr << info->GetName() << "{" << endl
             << "id:    " << id << endl
             << "type:  " << dbTypeStr[info->GetDatabaseType()] << endl
             << "ver:   " << info->GetVersion() << endl
             << "exts:  " << dbm.PluginFileExtensions(id) << endl
             << "files: " << dbm.PluginFilenames(id) << "}" << endl << endl;
        }
      else
        {
        cerr << id << endl;
        }
      }
    }
  catch(VisItException &e)
    {
    cerr << e.Message() << endl;
    }

  return 0;
}

