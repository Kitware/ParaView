#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <map>
using namespace std;

#include "PluginManager.h"
#include "DatabasePluginManager.h"
#include "DatabasePluginInfo.h"
#include "VisItException.h"

#include "BootstrapConfigure.h"

// This is the template used to generate the server manager 
// XML tags.
const char SOURCE_PROXY_XML_TEMPLATE[]="\
<!--\n\
========================================================================@NAME@ -->\n\
<SourceProxy\n\
    name=\"@NAME@VisItDatabaseBridge\"\n\
    class=\"vtkVisItDatabaseBridge\"\n\
    label=\"VisIt @NAME@ Database Bridge\"\n\
    base_proxyname=\"VisItDatabaseBridgeUI\"\n\
    base_proxygroup=\"sources\">\n\
  <Documentation\n\
      short_help=\"@NAME@ Reader.\"\n\
      long_help=\"@NAME@ Reader. @DESCRIPTION@\">\n\
  </Documentation>\n\
  <!-- Id of Visit plugin to be used. -->\n\
  <StringVectorProperty\n\
    name=\"PluginId\"\n\
    command=\"SetPluginId\"\n\
    number_of_elements=\"1\"\n\
    default_values=\"@PLUGIN_ID@\">\n\
    <Documentation>The id used by VisIt to identify this plugin.</Documentation>\n\
  </StringVectorProperty>\n\
</SourceProxy>\
";

const char PQ_READER_XML_TEMPLATE[]="\
<!--\n\
========================================================================@NAME@ -->\n\
<Reader\n\
    name=\"@NAME@VisItDatabaseBridge\"\n\
    extensions=\"@FILE_EXT@\"\n\
    file_description=\"@NAME@ Files (beta)\">\n\
</Reader>\
";

const char PANEL_ASSOCIATION_TEMPLATE[]="  @NAME@VisItDatabaseBridge";

// Human freindly strings for plugin description.
const char *dbTypeStr[] = {
    "STSD",
    "STMD",
    "MTSD",
    "MTMD",
    "CUSTOM"};

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
int main(int argc, char **argv)
{
  // Command line usage:
  if (argc<5)
    {
    cerr << "VisIt database bridge plugin configuration generator." << endl
         << "Usage:" << endl
         << "\trequired:/path/to/plugins" << endl
         << "\trequired:/runtime/path/for/plugin/manager" << endl
         << "\trequired:/path/to/servermanager.xml.in" << endl
         << "\trequired:/path/to/readers.xml.in" << endl
         << "\trequired:/path/to/CMakeLists.txt.in" << endl
         << "\toptional:/path/to/skip/file" << endl
         << "Paths should be privded using Unix seperators." << endl;
    return 1;
    }
  // Configure environment for the following run based on platform.
  InitializeEnvironment(argc,argv);
  // The plugin folder has been provieded as the first command
  // tail argument. The plugin manager will give us access to 
  // all of the plugins there.
  const char *pluginPath=argv[1];
  // At runtime the PluginManager needs to know where to look
  // for plugins.
  const char *runTimePluginPath=argv[2];
  // The server manager configuration file has been provided as
  // the second command tail argument. We will configure this file
  // and write the results.

  cerr << "Arg1 pluginPath: " << pluginPath << endl;
  cerr << "Arg2 runTimePath: " << runTimePluginPath << endl;
  cerr << "Arg3: " << argv[3] << endl;
  cerr << "Arg4: " << argv[4] << endl;
  cerr << "Arg5: " << argv[5] << endl;
  string smConfigFileIn(argv[3]);
  string smConfigText;
  if (!LoadText(smConfigFileIn,smConfigText))
    {
    cerr << "Failed to load server manager config file. Aborting." << endl;
    return 1;
    }
  // As we process each plugin the coresponding XML will be written
  // to this stream, After all plugins have been processed this
  // stream is used to configure the above file.
  ostringstream sourceProxies;
  // The client side configuration file (i.e. readers.xml) has been
  // provided as the third command tail argument. We will configure
  // this file and write the results.
  string pqReadersConfigFileIn(argv[4]);
  string pqReadersConfigText;
  if (!LoadText(pqReadersConfigFileIn,pqReadersConfigText))
    {
    cerr << "Failed to load pq reader config file. Aborting." << endl;
    return 1;
    }
  // As we process each plugin the coresponding XML will be written
  // to this stream, After all plugins have been processed this
  // stream is used to configure the above file.
  ostringstream pqReaders;
  // The cmake lists configuration file has been provided in the
  // fourth command tail argument. We will configure this file and
  // write the results.
  string cmakeConfigFileIn(argv[5]);
  string cmakeConfigText;
  if (!LoadText(cmakeConfigFileIn,cmakeConfigText))
    {
    cerr << "Failed to load cmake config file. Aborting." << endl;
    return 1;
    }
  // As we process each plugin the name of each plugin is cat'ed to
  // this stream. We need to provide this list in the cmake file
  // so that custom panel association get created when cmake runs.
  ostringstream panelAssociations;
  // A list of database plugins to skip may be provided in the 
  // fifth command tail  argument. The format of this file is 
  // expected to be one plugin name per line.
  vector<string> skipSet;
  if (argc>6)
    {
    LoadLines(argv[6],skipSet);
    }
  // A flag that indicates we should use a path relative to the 
  // paraview binary.

  // Report pre run summary. Run time info is sent to stderr.
  cerr << "Configuring:" << endl
       << "\t" << smConfigFileIn << endl
       << "\t" << pqReadersConfigFileIn << endl
       << "\t" << cmakeConfigFileIn << endl
       << "Searching:" << endl
       << "\t" << pluginPath <<  endl
       << "Runtime path:" << endl
       << "\t" << runTimePluginPath << endl
       << "Will skip:" << endl
       << "\t" << skipSet << endl;

  // Statistics to track for end of run summary.
  int nProcessed=0;
  int nSkipped=0;
  int nFailed=0;

  try
    {
    // Pre-load all of the plugins. Loop over each and write an
    // Server Manager XML descriptor.
    // DatabasePluginManager expects the database dlls to be in
    // pluginPath\databases
    DatabasePluginManager dbm;
    dbm.Initialize(PluginManager::MDServer, false, pluginPath);
    int nPlugins=dbm.GetNAllPlugins();
    cerr << "nPlugins: " << nPlugins << endl;
    dbm.LoadPluginsNow();
    for (int i=0; i<nPlugins; ++i)
      {
      // Get the id that VisIt uses to refer to the given plugin.
      string pluginId=dbm.GetAllID(i);
      // Get detailed information about what it provides.
      CommonDatabasePluginInfo *info=dbm.GetCommonPluginInfo(pluginId);
      if (info) 
        {
        const string &name=info->GetName();
        cerr << "\t" << name;
        // We are only going to process this plugin if it's not
        // in the skip list.
        if (skipSet&name)
          {
          // it was, so keep going.
          cerr << "...skipped." << endl;
          ++nSkipped;
          continue;
          }

        // Generate source proxy XML for this plugin.
        string sourceProxyXML(SOURCE_PROXY_XML_TEMPLATE);
        SearchAndReplace("@NAME@",name,sourceProxyXML);
        SearchAndReplace("@PLUGIN_ID@",pluginId,sourceProxyXML);
        ostringstream description;
        description
             << "Type " << dbTypeStr[info->GetDatabaseType()]
             << ", version " << info->GetVersion()
             << ", extensions  " << dbm.PluginFileExtensions(pluginId)
             << ", associated files " << dbm.PluginFilenames(pluginId)
             << ".";
        SearchAndReplace("@DESCRIPTION@",description.str(),sourceProxyXML);
        sourceProxies << sourceProxyXML << endl;

        // Generate reader XML for this plugin.
        ostringstream readerExt;
        readerExt << dbm.PluginFileExtensions(pluginId);
        string pqReaderXML(PQ_READER_XML_TEMPLATE);
        SearchAndReplace("@NAME@",name,pqReaderXML);
        SearchAndReplace("@FILE_EXT@",readerExt.str(),pqReaderXML);
        pqReaders << pqReaderXML << endl;

        // Generate a custom panel association for this plugin.
        string panelAssociation(PANEL_ASSOCIATION_TEMPLATE);
        SearchAndReplace("@NAME@",name,panelAssociation);
        panelAssociations << panelAssociation << endl;

        cerr << "...ok!" << endl;

        ++nProcessed;
        }
      else
        {
        // The plugin failed to load. It may be due to link error, so
        // we report it, the user can add -l to fix.
        cerr << "\t" << pluginId << "...failed to load." << endl;
        ++nFailed;
        }
      }
    }
  catch(VisItException &e)
    {
    cerr << "An expection occured, process terminating prematurely." << endl;
    cerr << e.Message() << endl;
    return 1;
    }

  // Now that we have processed all of the available plugins and have
  // generated a source proxy tag for each we can configure.
  // First the server manager xml...
  SearchAndReplace("@PLUGIN_PATH@",pluginPath,smConfigText);
  SearchAndReplace("@SOURCE_PROXIES@",sourceProxies.str(),smConfigText);
  // Write configured XML to disk.
  size_t extStart=smConfigFileIn.find(".in");
  if (extStart==string::npos)
    {
    // The file didn't have ".in" extension, so to avoid trashing
    // the input file we won't write.
    cerr << "ERROR: config file did not have the \".in\" extension, configuration not written." << endl;
    // cout << smConfigText << endl;
    }
  else
    {
    // Write the configuration back to disk, using the filename with
    // the ".in" extension stripped.
    string smConfigFile=smConfigFileIn.substr(0,extStart);
    WriteText(smConfigFile,smConfigText);
    cerr << "Wrote server manager configuration to " << smConfigFile << "." << endl;
    }
  // ... and the PQ readers xml....
  SearchAndReplace("@PQ_READERS@",pqReaders.str(),pqReadersConfigText);
  // Write configured XML to disk.
  extStart=pqReadersConfigFileIn.find(".in");
  if (extStart==string::npos)
    {
    // The file didn't have ".in" extension, so to avoid trashing
    // the input file we won't write.
    cerr << "ERROR: config file did not have the \".in\" extension, configuration not written." << endl;
    // cout << smConfigText << endl;
    }
  else
    {
    // Write the configuration back to disk, using the filename with
    // the ".in" extension stripped.
    string pqReadersConfigFile=pqReadersConfigFileIn.substr(0,extStart);
    WriteText(pqReadersConfigFile,pqReadersConfigText);
    cerr << "Wrote pq readers configuration to " << pqReadersConfigFile << "." << endl;
    }
  // ... and the CMakeFile.
  SearchAndReplace("@PANEL_ASSOCIATIONS@",panelAssociations.str(),cmakeConfigText);
  // Write configured XML to disk.
  extStart=cmakeConfigFileIn.find(".in");
  if (extStart==string::npos)
    {
    // The file didn't have ".in" extension, so to avoid trashing
    // the input file we won't write.
    cerr << "ERROR: config file did not have the \".in\" extension, configuration not written." << endl;
    // cout << smConfigText << endl;
    }
  else
    {
    // Write the configuration back to disk, using the filename with
    // the ".in" extension stripped.
    string cmakeConfigFile=cmakeConfigFileIn.substr(0,extStart);
    WriteText(cmakeConfigFile,cmakeConfigText);
    cerr << "Wrote cmake configuration to " << cmakeConfigFile << "." << endl;
    }


  // Post run summary statistics.
  cerr << nProcessed << " plugins processed." << endl;
  cerr << nSkipped << " were intentionally skipped." << endl;
  cerr << nFailed << " failed to load." << endl;

  return 0;
}

