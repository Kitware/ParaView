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

#include "vtkClientServerStream.h"
#include "vtkDirectory.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkSMXMLParser.h"
#include "vtkString.h"

#include "vtkProcessModule.h"

// Needed for VTK_USE_PATENTED
#include "vtkToolkits.h"
#include "vtkSMGeneratedModules.h"

#include <vtkstd/vector>

#include "vtkStdString.h"

struct vtkSMApplicationInternals
{
  struct ConfFile
  {
    vtkStdString FileName;
    vtkStdString Dir;
  };

  vtkstd::vector<ConfFile> Files;
};

vtkStandardNewMacro(vtkSMApplication);
vtkCxxRevisionMacro(vtkSMApplication, "1.12");

//---------------------------------------------------------------------------
vtkSMApplication::vtkSMApplication()
{
  this->Internals = new vtkSMApplicationInternals;
}

//---------------------------------------------------------------------------
vtkSMApplication::~vtkSMApplication()
{
  delete this->Internals;
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
  return this->Internals->Files.size();
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
  vtkPVServerManager_Initialize(
    vtkProcessModule::GetProcessModule()->GetInterpreter());

  vtkSMProxyManager* proxyM = vtkSMProxyManager::New();
  this->SetProxyManager(proxyM);
  this->SetApplication(this);

  vtkSMXMLParser* parser = vtkSMXMLParser::New();

  char* init_string;

  init_string =  vtkSMDefaultModulesfiltersGetInterfaces();
  parser->Parse(init_string);
  parser->ProcessConfiguration(proxyM);
  delete[] init_string;

  init_string =  vtkSMDefaultModulesreadersGetInterfaces();
  parser->Parse(init_string);
  parser->ProcessConfiguration(proxyM);
  delete[] init_string;

  init_string =  vtkSMDefaultModulessourcesGetInterfaces();
  parser->Parse(init_string);
  parser->ProcessConfiguration(proxyM);
  delete[] init_string;

  init_string =  vtkSMDefaultModulesutilitiesGetInterfaces();
  parser->Parse(init_string);
  parser->ProcessConfiguration(proxyM);
  delete[] init_string;

  init_string =  vtkSMDefaultModulesrenderingGetInterfaces();
  parser->Parse(init_string);
  parser->ProcessConfiguration(proxyM);
  delete[] init_string;

  parser->Delete();

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
//       int extPos = vtkString::Length(file)-4;
      
//       // Look for the ".xml" extension.
//       if((extPos > 0) && vtkString::Equals(file+extPos, ".xml"))
//         {
//         char* fullPath 
//           = new char[vtkString::Length(file)+vtkString::Length(directory)+2];
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

  ostrstream tmppath;
  tmppath << dir << "/" << fname << ends;
  vtkSMXMLParser* parser = vtkSMXMLParser::New();
  parser->SetFileName(tmppath.str());
  delete[] tmppath.str();
  int res = parser->Parse();
  parser->ProcessConfiguration(proxyM);
  parser->Delete();
  return res;
}

//---------------------------------------------------------------------------
void vtkSMApplication::Finalize()
{
  //this->GetProcessModule()->FinalizeInterpreter();
  this->SetProxyManager(0);

}

//---------------------------------------------------------------------------
void vtkSMApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
