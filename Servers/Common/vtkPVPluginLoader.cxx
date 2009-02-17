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
#include "vtkIntArray.h"
#include "vtkPVOptions.h"
#include "vtkStringArray.h"
#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkPVPluginLoader);
vtkCxxRevisionMacro(vtkPVPluginLoader, "1.9");

#ifdef _WIN32
// __cdecl gives an unmangled name
#define C_DECL __cdecl
#else
#define C_DECL
#endif

typedef const char* (C_DECL *PluginXML1)();
typedef void (C_DECL *PluginXML2)(int&, char**&);
typedef void (C_DECL *PluginPython)(int&, const char **&, const char **&,
                                    const int *&);
typedef void (C_DECL *PluginInit)(vtkClientServerInterpreter*);


//-----------------------------------------------------------------------------
vtkPVPluginLoader::vtkPVPluginLoader()
{
  this->Loaded = 0;
  this->FileName = 0;
  this->Error = NULL;
  this->SearchPaths = NULL;
  
  this->ServerManagerXML = vtkStringArray::New();
  this->PythonModuleNames = vtkStringArray::New();
  this->PythonModuleSources = vtkStringArray::New();
  this->PythonPackageFlags = vtkIntArray::New();

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
      if(paths.size())
        {
        paths += ";";
        }
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
    this->ServerManagerXML->Delete();
    }
  if(this->PythonModuleNames)
    {
    this->PythonModuleNames->Delete();
    }
  if(this->PythonModuleSources)
    {
    this->PythonModuleSources->Delete();
    }
  if(this->PythonPackageFlags)
    {
    this->PythonPackageFlags->Delete();
    }

  if(this->Error)
    {
    delete [] this->Error;
    }
  
  if(this->SearchPaths)
    {
    delete [] this->SearchPaths;
    }
  
  if(this->FileName)
    {
    delete [] this->FileName;
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
      // old SM xml, new plugins don't have this method
      PluginXML1 xml1 = 
        (PluginXML1)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginXML");

      // new SM xml
      PluginXML2 xml2 = 
        (PluginXML2)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginXMLList");

      PluginPython python =
        (PluginPython)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginPythonSourceList");

      PluginInit init = 
        (PluginInit)vtkDynamicLoader::GetSymbolAddress(lib, "ParaViewPluginInit");
      if(xml1 || xml2 || python || init)
        {
        this->Loaded = 1;
        if(init)
          {
          (*init)(vtkProcessModule::GetProcessModule()->GetInterpreter());
          }
        if(xml1)
          {
          const char* xmlString = (*xml1)();
          if(xmlString)
            {
            this->ServerManagerXML->SetNumberOfTuples(1);
            this->ServerManagerXML->SetValue(0, vtkStdString(xmlString));
            }
          }
        if(xml2)
          {
          int num;
          char** xml;
          (*xml2)(num, xml);
          this->ServerManagerXML->SetNumberOfTuples(num);
          for(int i=0; i<num; i++)
            {
            this->ServerManagerXML->SetValue(i, vtkStdString(xml[i]));
            }
          }
        if (python)
          {
          int num;
          const char **name;
          const char **source;
          const int *packages;
          (*python)(num, name, source, packages);
          this->PythonModuleNames->SetNumberOfTuples(num);
          this->PythonModuleSources->SetNumberOfTuples(num);
          this->PythonPackageFlags->SetNumberOfTuples(num);
          for (int i = 0; i < num; i++)
            {
            this->PythonModuleNames->SetValue(i, vtkStdString(name[i]));
            this->PythonModuleSources->SetValue(i, vtkStdString(source[i]));
            this->PythonPackageFlags->SetValue(i, packages[i]);
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
  os << indent << "Loaded: " << this->Loaded << endl;
  os << indent << "FileName: " 
    << (this->FileName? this->FileName : "(none)") << endl;
  os << indent << "ServerManagerXML: " 
    << (this->ServerManagerXML ? "(exists)" : "(none)") << endl;
  os << indent << "Error: " 
    << (this->Error? this->Error : "(none)") << endl;
  os << indent << "SearchPaths: " << (this->SearchPaths ? 
    this->SearchPaths : "(none)") << endl;
}

