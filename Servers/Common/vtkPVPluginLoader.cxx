/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPluginLoader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPluginLoader.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerInterpreter.h"
#include "vtkDynamicLoader.h"
#include "vtkPVOptions.h"
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkPVPluginLoader);
vtkCxxRevisionMacro(vtkPVPluginLoader, "1.1.2.1");

#ifdef _WIN32
// __cdecl gives an unmangled name
#define C_DECL __cdecl
#else
#define C_DECL
#endif

typedef const char* (C_DECL *PluginXML)();
typedef void (C_DECL *PluginInit)(vtkClientServerInterpreter*);


//-----------------------------------------------------------------------------
vtkPVPluginLoader::vtkPVPluginLoader()
{
  this->Loaded = 0;
  this->FileName = 0;
  this->ServerManagerXML = NULL;
  this->Error = NULL;
  this->SearchPaths = NULL;

  vtksys::String paths;
  const char* env = vtksys::SystemTools::GetEnv("PV_PLUGIN_PATH");
  if(env)
    {
    paths += env;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* opt = pm ? pm->GetOptions() : NULL;
  if(opt)
    {
    const char* path = opt->GetApplicationPath();
    vtksys::String appDir = vtksys::SystemTools::GetProgramPath(path);
    if(appDir.size())
      {
      appDir += "/plugins";
      paths = paths.size() ? paths + ";" : paths;
      paths += appDir;
      }
    }

  this->SearchPaths = new char[paths.size() + 1];
  strcpy(this->SearchPaths, paths.c_str());
}

//-----------------------------------------------------------------------------
vtkPVPluginLoader::~vtkPVPluginLoader()
{
  if(this->ServerManagerXML)
    {
    delete [] this->ServerManagerXML;
    }

  this->SetFileName(0);
  
  if(this->SearchPaths)
    {
    delete [] this->SearchPaths;
    }
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::SetFileName(const char* file)
{
  if(this->Loaded)
    {
    return;
    }
  
  if(this->FileName)
    {
    delete [] this->FileName;
    this->FileName = NULL;
    }
  if(file && file[0] != '\0')
    {
    size_t len = strlen(file);
    this->FileName = new char[len+1];
    strcpy(this->FileName, file);
    }

  if(!this->Loaded && FileName && FileName[0] != '\0')
    {
    vtkLibHandle lib = vtkDynamicLoader::OpenLibrary(FileName);
    if(lib)
      {
      PluginXML xml = 
        (PluginXML)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginXML");
      PluginInit init = 
        (PluginInit)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginInit");
      if(xml || init)
        {
        this->Loaded = 1;
        if(init)
          {
          (*init)(vtkProcessModule::GetProcessModule()->GetInterpreter());
          }
        if(xml)
          {
          const char* xmlString = (*xml)();
          if(xmlString)
            {
            size_t len = strlen(xmlString);
            this->ServerManagerXML = new char[len+1];
            strcpy(this->ServerManagerXML, xmlString);
            }
          }
        this->Modified();
        }
      else
        {
        // toss it out if it isn't a server manager plugin
        vtkDynamicLoader::CloseLibrary(lib);
        this->SetError("This is not a ParaView plugin.");
        }
      }
    else
      {
      this->SetError(vtkDynamicLoader::LastError());
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVPluginLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " 
    << (this->FileName? this->FileName : "(none)") << endl;
  os << indent << "ServerManagerXML: " 
    << (this->ServerManagerXML? this->ServerManagerXML : "(none)") << endl;
  os << indent << "Error: " 
    << (this->Error? this->Error : "(none)") << endl;
  os << indent << "Loaded: " << this->Loaded << endl;
  os << indent << "SearchPaths: " << (this->SearchPaths ? 
    this->SearchPaths : "(none)") << endl;
}

