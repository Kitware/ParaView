/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPluginLoader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPluginLoader.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerInterpreter.h"
#include "vtkDynamicLoader.h"

vtkStandardNewMacro(vtkSMPluginLoader);
vtkCxxRevisionMacro(vtkSMPluginLoader, "1.4");

#ifdef _WIN32
// __cdecl gives an unmangled name
#define C_DECL __cdecl
#else
#define C_DECL
#endif

typedef const char* (C_DECL *PluginXML)();
typedef void (C_DECL *PluginInit)(vtkClientServerInterpreter*);


//-----------------------------------------------------------------------------
vtkSMPluginLoader::vtkSMPluginLoader()
{
  this->Loaded = 0;
  this->FileName = 0;
  this->ServerManagerXML = NULL;
}

//-----------------------------------------------------------------------------
vtkSMPluginLoader::~vtkSMPluginLoader()
{
  if(this->ServerManagerXML)
    {
    delete [] this->ServerManagerXML;
    }
}

//-----------------------------------------------------------------------------
void vtkSMPluginLoader::SetFileName(const char* file)
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
      if(xml && init)
        {
        this->Loaded = 1;
        (*init)(vtkProcessModule::GetProcessModule()->GetInterpreter());
        const char* xmlString = (*xml)();
        if(xmlString)
          {
          size_t len = strlen(xmlString);
          this->ServerManagerXML = new char[len+1];
          strcpy(this->ServerManagerXML, xmlString);
          }
        this->Modified();
        }
      else
        {
        // toss it out if it isn't a server manager plugin
        vtkDynamicLoader::CloseLibrary(lib);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkSMPluginLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

