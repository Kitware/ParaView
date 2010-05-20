/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPluginInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
cxx     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPluginInformation.h"


#include "vtkClientServerStream.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkObjectFactory.h"
#include "vtkPVPluginLoader.h"

vtkStandardNewMacro(vtkPVPluginInformation);

//----------------------------------------------------------------------------
vtkPVPluginInformation::vtkPVPluginInformation()
{
  this->Loaded = 0;
  this->PluginVersion = NULL;
  this->PluginName = NULL;
  this->FileName = NULL;
  this->SearchPaths = NULL;
  this->Error = NULL;
  this->ServerURI = NULL;
  this->RequiredPlugins = NULL;
  this->AutoLoad = 0;
  this->RequiredOnClient = 1;
  this->RequiredOnServer = 1;
}

//----------------------------------------------------------------------------
vtkPVPluginInformation::~vtkPVPluginInformation()
{
  this->ClearInfo();
}

//----------------------------------------------------------------------------
void vtkPVPluginInformation::ClearInfo()
{
  this->Loaded = 0;
  this->SetPluginVersion(0);
  this->SetPluginName(0);
  this->SetFileName(0);
  this->SetSearchPaths(0);
  this->SetError(0);
  this->SetServerURI(0);
  this->SetRequiredPlugins(0);
  this->AutoLoad = 0;
  this->RequiredOnClient = 1;
  this->RequiredOnServer = 1;
  
}

//----------------------------------------------------------------------------
void vtkPVPluginInformation::AddInformation(vtkPVInformation *info)
{
  if(!info)
    {
    return;
    }
  this->DeepCopy(vtkPVPluginInformation::SafeDownCast(info));
}

//----------------------------------------------------------------------------
void vtkPVPluginInformation::DeepCopy(vtkPVPluginInformation *info)
{
  if(!info)
    {
    return;
    }
  this->ClearInfo();
    
  this->SetPluginName(info->GetPluginName());
  this->SetFileName(info->GetFileName());
  this->SetSearchPaths(info->GetSearchPaths());
  this->SetError(info->GetError());
  this->SetPluginVersion(info->GetPluginVersion());
  this->SetServerURI(info->GetServerURI());
  this->Loaded = info->GetLoaded();
  this->SetAutoLoad(info->GetAutoLoad());
  this->SetRequiredOnClient(info->GetRequiredOnClient());
  this->SetRequiredOnServer(info->GetRequiredOnServer());
  this->SetRequiredPlugins(info->GetRequiredPlugins());
}

//----------------------------------------------------------------------------
int vtkPVPluginInformation::Compare(vtkPVPluginInformation *info)
{
  if (info == NULL)
    {
    return 0;
    }
  if (this->CompareInfoString(info->GetServerURI(), this->ServerURI) &&
      this->CompareInfoString(info->GetFileName(), this->FileName) &&
      this->CompareInfoString(info->GetPluginName(), this->PluginName) &&
      this->CompareInfoString(info->GetPluginVersion(), this->PluginVersion))
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVPluginInformation::CopyFromObject(vtkObject* obj)
{
  if (!obj)
    {
    return;
    }

  vtkPVPluginLoader* const pluginLoader = vtkPVPluginLoader::SafeDownCast(obj);
  if(!pluginLoader)
    {
    vtkErrorMacro("Cannot downcast to vtkPVPluginLoader.");
    return;
    }
  
  this->DeepCopy(pluginLoader->GetPluginInfo());
}

//----------------------------------------------------------------------------
bool vtkPVPluginInformation::CompareInfoString(
  const char* str1, const char* str2)
{
  return (str1 && *str1 && str2 && *str2 && !strcmp(str1,str2)) ? true : false;
}

//----------------------------------------------------------------------------
void vtkPVPluginInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->PluginName
       << this->PluginVersion
       << this->FileName
       << this->ServerURI
       << this->SearchPaths
       << this->RequiredPlugins
       << this->RequiredOnServer
       << this->RequiredOnClient
       << this->Error
       << this->Loaded;

  *css << vtkClientServerStream::End;
}

// Macros used to make it easy to insert/remove entries when serializing
// to/from a stream.

#define CSS_ARGUMENT_BEGIN() \
  {\
  int __vtk__css_argument_int_counter = 0

#define CSS_GET_NEXT_ARGUMENT(css, msg, var)\
  css->GetArgument(msg, __vtk__css_argument_int_counter++, var)

#define CSS_ARGUMENT_END() \
  }


//----------------------------------------------------------------------------
void vtkPVPluginInformation::CopyFromStream(const vtkClientServerStream* css)
{
  CSS_ARGUMENT_BEGIN();

  const char* pluginname = 0;
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &pluginname))
    {
    vtkErrorMacro("Error parsing plugin name of data.");
    return;
    }
  this->SetPluginName(pluginname);
  const char* pluginversion = 0;
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &pluginversion))
    {
    vtkErrorMacro("Error parsing plugin version of data.");
    return;
    }
  this->SetPluginVersion(pluginversion);
  const char* pluginfilename = 0;
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &pluginfilename))
    {
    vtkErrorMacro("Error parsing plugin filename of data.");
    return;
    }
  this->SetFileName(pluginfilename);
  const char* serveruri = 0;
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &serveruri))
    {
    vtkErrorMacro("Error parsing plugin server URI of data.");
    return;
    }
  this->SetServerURI(serveruri);
  const char* searchpaths = 0;
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &searchpaths))
    {
    vtkErrorMacro("Error parsing plugin search paths of data.");
    return;
    }
  this->SetSearchPaths(searchpaths);

  const char* requiredplugins = 0;
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &requiredplugins))
    {
    vtkErrorMacro("Error parsing plugin depended plugins of data.");
    return;
    }
  this->SetRequiredPlugins(requiredplugins);

  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->RequiredOnServer))
    {
    vtkErrorMacro("Error parsing RequiredOnServer of data.");
    return;
    }
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->RequiredOnClient))
    {
    vtkErrorMacro("Error parsing RequiredOnClient of data.");
    return;
    }

  const char* loaderror = 0;
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &loaderror))
    {
    vtkErrorMacro("Error parsing plugin load error of data.");
    return;
    }
  this->SetError(loaderror);
  if (!CSS_GET_NEXT_ARGUMENT(css, 0, &this->Loaded))
    {
    vtkErrorMacro("Error parsing Loaded of data.");
    return;
    }

  CSS_ARGUMENT_END();
}

//----------------------------------------------------------------------------
void vtkPVPluginInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " 
    << (this->FileName? this->FileName : "(none)") << endl;
  os << indent << "PluginName: " 
    << (this->PluginName? this->PluginName : "(none)") << endl;
  os << indent << "PluginVersion: " 
    << (this->PluginVersion? this->PluginVersion : "(none)") << endl;
  os << indent << "ServerURI: "
    << (this->ServerURI? this->ServerURI : "(none)") << endl;  
  os << indent << "Loaded: " << this->Loaded << endl;
  os << indent << "SearchPaths: " << (this->SearchPaths ? 
    this->SearchPaths : "(none)") << endl;
  os << indent << "RequiredPlugins: " << (this->RequiredPlugins ? 
    this->RequiredPlugins : "(none)") << endl;
  os << indent << "AutoLoad: " << this->AutoLoad << endl;
  os << indent << "RequiredOnClient: " << this->RequiredOnClient << endl;
  os << indent << "RequiredOnServer: " << this->RequiredOnServer << endl;
  os << indent << "Error: "
    << (this->Error? this->Error : "(none)") << endl;
  
}
