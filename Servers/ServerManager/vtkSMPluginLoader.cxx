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
vtkCxxRevisionMacro(vtkSMPluginLoader, "1.1");

//-----------------------------------------------------------------------------
vtkSMPluginLoader::vtkSMPluginLoader()
{
  this->Loaded = 0;
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
void vtkSMPluginLoader::LoadPlugin(const char* FileName)
{
  typedef const char* (*ModuleXML)();
  typedef void (*ModuleInit)(vtkClientServerInterpreter*);

  if(!this->Loaded && FileName && FileName[0] != '\0')
    {
    vtkLibHandle lib = vtkDynamicLoader::OpenLibrary(FileName);
    if(lib)
      {
      ModuleXML modxml = 
        (ModuleXML)vtkDynamicLoader::GetSymbolAddress(lib, "ModuleXML");
      ModuleInit modinit = 
        (ModuleInit)vtkDynamicLoader::GetSymbolAddress(lib, "ModuleInit");
      if(modxml && modinit)
        {
        this->Loaded = 1;
        (*modinit)(vtkProcessModule::GetProcessModule()->GetInterpreter());
        const char* xml = (*modxml)();
        if(xml)
          {
          size_t len = strlen(xml);
          this->ServerManagerXML = new char[len+1];
          strcpy(this->ServerManagerXML, xml);
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

