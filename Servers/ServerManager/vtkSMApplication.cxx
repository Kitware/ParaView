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
#include "vtkKWArguments.h"
#include "vtkObjectFactory.h"
#include "vtkSMProcessModule.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSimpleCommunicationModule.h"
#include "vtkSMXMLParser.h"
#include "vtkString.h"

#include "vtkProcessModule.h"

#include "vtkSMGeneratedModules.h"

vtkStandardNewMacro(vtkSMApplication);
vtkCxxRevisionMacro(vtkSMApplication, "1.3");

//---------------------------------------------------------------------------
vtkSMApplication::vtkSMApplication()
{
}

//---------------------------------------------------------------------------
vtkSMApplication::~vtkSMApplication()
{
}

extern "C" { void vtkPVServerManager_Initialize(vtkClientServerInterpreter*); }

//---------------------------------------------------------------------------
void vtkSMApplication::Initialize()
{
//   args->AddCallback("--configuration-path", 
//                     vtkKWArguments::EQUAL_ARGUMENT, 
//                     NULL, 
//                     NULL, 
//                     "Directory where all configuration files are stored");

//   args->Parse();

  //vtkSMProcessModule* pm = vtkSMProcessModule::New();
  //pm->InitializeInterpreter();
  //this->SetProcessModule(pm);
  //pm->Delete();

  vtkPVServerManager_Initialize(
    vtkProcessModule::GetProcessModule()->GetInterpreter());

  vtkSMSimpleCommunicationModule* cm = vtkSMSimpleCommunicationModule::New();
  //cm->Connect();
  this->SetCommunicationModule(cm);
  cm->Delete();

  vtkSMProxyManager* proxyM = vtkSMProxyManager::New();
  this->SetProxyManager(proxyM);

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

  // TODO revise this
  vtkClientServerStream str;

  vtkClientServerID gf = cm->NewStreamObject("vtkGraphicsFactory", str);
  str << vtkClientServerStream::Invoke
      << gf << "SetUseMesaClasses" << 1
      << vtkClientServerStream::End;
  cm->DeleteStreamObject(gf, str);
  vtkClientServerID imf = cm->NewStreamObject("vtkImagingFactory", str);
  str << vtkClientServerStream::Invoke
      << imf << "SetUseMesaClasses" << 1
      << vtkClientServerStream::End;
  cm->DeleteStreamObject(imf, str);

  int serverids = 1;
  cm->SendStreamToServers(&str, 1, &serverids);

}

//---------------------------------------------------------------------------
void vtkSMApplication::Finalize()
{
  vtkSMSimpleCommunicationModule::SafeDownCast(this->GetCommunicationModule())
    ->Disconnect();
  this->SetCommunicationModule(0);
  this->GetProcessModule()->FinalizeInterpreter();
  this->SetProcessModule(0);
  this->SetProxyManager(0);

}

//---------------------------------------------------------------------------
void vtkSMApplication::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
