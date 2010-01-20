/*=========================================================================

  Program:   ParaView
  Module:    vtkSMApplication.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMApplication.h"

#include "vtkPVConfig.h" // To get PARAVIEW_USE_*
#include "vtkPVEnvironmentInformation.h"

#include "vtkClientServerStream.h"
#include "vtkDirectory.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkSMPluginManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMXMLParser.h"

#include "vtkProcessModule.h"

#include "vtkToolkits.h"
#include "vtkSMGeneratedModules.h"

#include <vtkstd/vector>
#include <vtksys/ios/sstream>

#include "vtkStdString.h"

#ifdef VTK_USE_QVTK
# include "vtkQtInitialization.h"
#endif

struct vtkSMApplicationInternals
{
  struct ConfFile
  {
    vtkStdString FileName;
    vtkStdString Dir;
  };

  vtkstd::vector<ConfFile> Files;
  vtkSmartPointer<vtkPVEnvironmentInformation> EnvInfo ;
  vtkSmartPointer<vtkSMPluginManager> PluginManager ;
};

vtkStandardNewMacro(vtkSMApplication);
vtkCxxRevisionMacro(vtkSMApplication, "1.22");

//---------------------------------------------------------------------------
vtkSMApplication::vtkSMApplication()
{
  this->Internals = new vtkSMApplicationInternals;
  this->Internals->EnvInfo = vtkSmartPointer<vtkPVEnvironmentInformation>::New();
#ifdef VTK_USE_QVTK
  this->QtInitializer = NULL;
#endif
}

//---------------------------------------------------------------------------
vtkSMApplication::~vtkSMApplication()
{
  delete this->Internals;

#ifdef VTK_USE_QVTK
  if (this->QtInitializer)
    {
    this->QtInitializer->Delete();
    this->QtInitializer = 0;
    }
#endif
}

extern "C" { void vtkPVServerManager_Initialize(vtkClientServerInterpreter*); }

//---------------------------------------------------------------------------
void vtkSMApplication::AddConfigurationFile(const char* fname, const char* dir)
{
  vtkSMApplicationInternals::ConfFile file;
  file.FileName = fname;
  file.Dir = dir;
  this->Internals->Files.push_back(file);
}

//---------------------------------------------------------------------------
unsigned int vtkSMApplication::GetNumberOfConfigurationFiles()
{
  return static_cast<unsigned int>(this->Internals->Files.size());
}

//---------------------------------------------------------------------------
void vtkSMApplication::GetConfigurationFile(
  unsigned int idx, const char*& fname, const char*& dir)
{
  fname = this->Internals->Files[idx].FileName.c_str();
  dir = this->Internals->Files[idx].Dir.c_str();
}

//---------------------------------------------------------------------------
void vtkSMApplication::ParseConfigurationFiles()
{
  unsigned int numFiles = this->GetNumberOfConfigurationFiles();
  for (unsigned int i=0; i<numFiles; i++)
    {
    this->ParseConfigurationFile(this->Internals->Files[i].FileName.c_str(),
                                 this->Internals->Files[i].Dir.c_str());
    }
}

//---------------------------------------------------------------------------
void vtkSMApplication::Initialize()
{
#ifdef VTK_USE_QVTK
  this->QtInitializer  = vtkQtInitialization::New();
#endif

  vtkPVServerManager_Initialize(
    vtkProcessModule::GetProcessModule()->GetInterpreter());

  vtkSMProxyManager* proxyM = vtkSMProxyManager::New();
  this->SetProxyManager(proxyM);
  this->SetApplication(this);

  this->Internals->PluginManager = vtkSmartPointer<vtkSMPluginManager>::New();
  
  // Load the generated modules
#include "vtkParaViewIncludeModulesToSMApplication.h"

// //  const char* directory = args->GetValue("--configuration-path");
//   const char* directory =  "/home/berk/etc/servermanager";
//   if (directory)
//     {
//     vtkDirectory* dir = vtkDirectory::New();
//     if(!dir->Open(directory))
//       {
//       dir->Delete();
//       return;
//       }
    
//     for(int i=0; i < dir->GetNumberOfFiles(); ++i)
//       {
//       const char* file = dir->GetFile(i);
//       int extPos = strlen(file)-4;
      
//       // Look for the ".xml" extension.
//       if((extPos > 0) && !strcmp(file+extPos, ".xml"))
//         {
//         char* fullPath 
//           = new char[strlen(file) + strlen(directory)+2];
//         strcpy(fullPath, directory);
//         strcat(fullPath, "/");
//         strcat(fullPath, file);
        
//         cerr << fullPath << endl;
        
//         vtkSMXMLParser* parser = vtkSMXMLParser::New();
//         parser->SetFileName(fullPath);
//         parser->Parse();
//         parser->ProcessConfiguration(proxyM);
//         parser->Delete();
        
//         delete [] fullPath;
//         }
//       }
//     dir->Delete();
//     }
  
  proxyM->Delete();
}

//---------------------------------------------------------------------------
int vtkSMApplication::ParseConfigurationFile(const char* fname, const char* dir)
{
  vtkSMProxyManager* proxyM = this->GetProxyManager();
  if (!proxyM)
    {
    vtkErrorMacro("No global proxy manager defined. Can not parse file");
    return 0;
    }

  vtksys_ios::ostringstream tmppath;
  tmppath << dir << "/" << fname << ends;
  vtkSMXMLParser* parser = vtkSMXMLParser::New();
  parser->SetFileName(tmppath.str().c_str());
  int res = parser->Parse();
  parser->ProcessConfiguration(proxyM);
  parser->Delete();
  return res;
}

//---------------------------------------------------------------------------
int vtkSMApplication::ParseConfiguration(const char* configuration)
{
  vtkSMProxyManager* proxyM = this->GetProxyManager();
  if (!proxyM)
    {
    vtkErrorMacro("No global proxy manager defined. Can not parse file");
    return 0;
    }

  vtkSMXMLParser* parser = vtkSMXMLParser::New();
  int res = parser->Parse(configuration);
  parser->ProcessConfiguration(proxyM);
  parser->Delete();
  return res;
}

//---------------------------------------------------------------------------
void vtkSMApplication::Finalize()
{
  //this->GetProcessModule()->FinalizeInterpreter();
  this->SetProxyManager(0);

#ifdef VTK_USE_QVTK
  if (this->QtInitializer)
    {
    this->QtInitializer->Delete();
    this->QtInitializer = 0;
    }
#endif
}

//---------------------------------------------------------------------------
const char* vtkSMApplication::GetSettingsRoot(
  vtkIdType connectionId)
{
  vtkSMProxyManager* pxm = this->GetProxyManager();
  if (!pxm)
    {
    vtkErrorMacro("No global proxy manager defined. Can not parse file");
    return 0;
    }
  vtkSMProxy* helper = pxm->NewProxy("misc", "EnvironmentInformationHelper");
  helper->SetConnectionID(connectionId);
  helper->SetServers(vtkProcessModule::DATA_SERVER_ROOT);
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    helper->GetProperty("Variable"));
  if(!svp)
    {
    helper->UnRegister(NULL);
    return NULL;
    }  
#ifdef _WIN32
  svp->SetElement(0,"APPDATA");
#else
  svp->SetElement(0,"HOME");
#endif
  helper->UpdateVTKObjects();
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(helper->GetConnectionID(),
    vtkProcessModule::DATA_SERVER, this->Internals->EnvInfo, helper->GetID());
  helper->UnRegister(NULL);
  return this->Internals->EnvInfo->GetVariable();
}

//---------------------------------------------------------------------------
vtkSMPluginManager* vtkSMApplication::GetPluginManager()
{
  return this->Internals->PluginManager;
}

//---------------------------------------------------------------------------
void vtkSMApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
