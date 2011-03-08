/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPluginsInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPluginsInformation.h"

#include "vtkObjectFactory.h"
#include "vtkPVPlugin.h"
#include "vtkPVPluginLoader.h"
#include "vtkPVPluginTracker.h"
#include "vtkClientServerStream.h"

#include <vtkstd/string>
#include <vtkstd/vector>

namespace
{
  class vtkItem
    {
  public:
    vtkstd::string Name;
    vtkstd::string FileName;
    vtkstd::string RequiredPlugins;
    vtkstd::string Version;
    bool AutoLoad;
    bool Loaded;
    bool RequiredOnClient;
    bool RequiredOnServer;

    bool Load(const vtkClientServerStream& stream, int &offset)
      {
      const char* temp_ptr;
      if (!stream.GetArgument(0, offset++, &temp_ptr))
        {
        return false;
        }
      this->Name = temp_ptr;

      if (!stream.GetArgument(0, offset++, &temp_ptr))
        {
        return false;
        }
      this->FileName = temp_ptr;

      if (!stream.GetArgument(0, offset++, &temp_ptr))
        {
        return false;
        }
      this->RequiredPlugins = temp_ptr;

      if (!stream.GetArgument(0, offset++, &temp_ptr))
        {
        return false;
        }
      this->Version = temp_ptr;

      if (!stream.GetArgument(0, offset++, &this->AutoLoad))
        {
        return false;
        }
      if (!stream.GetArgument(0, offset++, &this->Loaded))
        {
        return false;
        }
      if (!stream.GetArgument(0, offset++, &this->RequiredOnClient))
        {
        return false;
        }
      if (!stream.GetArgument(0, offset++, &this->RequiredOnServer))
        {
        return false;
        }
      return true;
      }
    };

  void operator << (vtkClientServerStream& stream, const vtkItem& item)
    {
    stream << item.Name.c_str()
      << item.FileName.c_str()
      << item.RequiredPlugins.c_str()
      << item.Version.c_str()
      << item.AutoLoad
      << item.Loaded
      << item.RequiredOnClient
      << item.RequiredOnServer;
    }
}

class vtkPVPluginsInformation::vtkInternals :
  public vtkstd::vector<vtkItem>
{
};


vtkStandardNewMacro(vtkPVPluginsInformation);
//----------------------------------------------------------------------------
vtkPVPluginsInformation::vtkPVPluginsInformation()
{
  this->RootOnly = 1;
  this->SearchPaths = NULL;
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkPVPluginsInformation::~vtkPVPluginsInformation()
{
  delete this->Internals;
  this->Internals = NULL;
  this->SetSearchPaths(0);
}

//----------------------------------------------------------------------------
void vtkPVPluginsInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPlugins: " << this->GetNumberOfPlugins() << endl;
  for (unsigned int cc=0; cc < this->GetNumberOfPlugins(); cc++)
    {
    os << indent << this->GetPluginName(cc) << ": " << endl;
    }

}

//----------------------------------------------------------------------------
void vtkPVPluginsInformation::AddInformation(vtkPVInformation* other)
{
  vtkPVPluginsInformation* pvother =
    vtkPVPluginsInformation::SafeDownCast(other);
  if (pvother)
    {
    (*this->Internals) = (*pvother->Internals);
    this->SetSearchPaths(pvother->SearchPaths);
    }
}

//----------------------------------------------------------------------------
void vtkPVPluginsInformation::CopyToStream(vtkClientServerStream* stream)
{
  stream->Reset();
  *stream << vtkClientServerStream::Reply
    << this->SearchPaths
    << this->GetNumberOfPlugins();
  for (unsigned int cc=0; cc < this->GetNumberOfPlugins(); cc++)
    {
    *stream << (*this->Internals)[cc];
    }
  *stream << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVPluginsInformation::CopyFromStream(const vtkClientServerStream* stream)
{
  int offset=0;
  const char* search_paths=NULL;
  if (!stream->GetArgument(0, offset++, &search_paths))
    {
    vtkErrorMacro("Error parsing SearchPaths.");
    return;
    }
  this->SetSearchPaths(search_paths);

  unsigned int count;
  if (!stream->GetArgument(0, offset++, &count))
    {
    vtkErrorMacro("Error parsing count.");
    return;
    }
  this->Internals->clear();
  this->Internals->resize(count);
  for (unsigned int cc=0; cc < count; cc++)
    {
    (*this->Internals)[cc].Load(*stream, offset);
    }
}

//----------------------------------------------------------------------------
void vtkPVPluginsInformation::CopyFromObject(vtkObject*)
{
  this->Internals->clear();
  vtkPVPluginLoader* loader = vtkPVPluginLoader::New();
  this->SetSearchPaths(loader->GetSearchPaths());
  loader->Delete();

  vtkPVPluginTracker* tracker = vtkPVPluginTracker::GetInstance();
  for (unsigned int cc=0; cc < tracker->GetNumberOfPlugins(); cc++)
    {
    vtkItem item;
    item.Name = tracker->GetPluginName(cc);
    item.FileName = tracker->GetPluginFileName(cc);
    item.AutoLoad = tracker->GetPluginAutoLoad(cc);

    vtkPVPlugin* plugin = tracker->GetPlugin(cc);
    item.Loaded = plugin != NULL;
    if (plugin)
      {
      item.RequiredPlugins = plugin->GetRequiredPlugins();
      item.RequiredOnClient = plugin->GetRequiredOnClient();
      item.RequiredOnServer = plugin->GetRequiredOnServer();
      }
    else
      {
      item.RequiredOnClient = false;
      item.RequiredOnServer = false;
      }
    this->Internals->push_back(item);
    }
}

//----------------------------------------------------------------------------
void vtkPVPluginsInformation::Update(vtkPVPluginsInformation* other)
{
  // This is N^2, but we don't expect to have hundreds of plugins to cause
  // serious issues.
  vtkInternals::iterator other_iter;
  for (other_iter = other->Internals->begin();
    other_iter != other->Internals->end(); ++other_iter)
    {
    vtkInternals::iterator self_iter;
    for (self_iter = this->Internals->begin();
      self_iter != this->Internals->end(); ++self_iter)
      {
      if (other_iter->Name == self_iter->Name ||
        other_iter->FileName == self_iter->FileName)
        {
        bool prev_autoload = self_iter->AutoLoad;
        (*self_iter) = (*other_iter);
        self_iter->AutoLoad = prev_autoload;
        break;
        }
      }
    if (self_iter == this->Internals->end())
      {
      this->Internals->push_back(*other_iter);
      }
    }
}

//----------------------------------------------------------------------------
unsigned int vtkPVPluginsInformation::GetNumberOfPlugins()
{
  return static_cast<unsigned int>(this->Internals->size());
}
//----------------------------------------------------------------------------
const char* vtkPVPluginsInformation::GetPluginName(unsigned int cc)
{
  if (cc < this->GetNumberOfPlugins())
    {
    return (*this->Internals)[cc].Name.c_str();
    }
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkPVPluginsInformation::GetPluginFileName(unsigned int cc)
{
  if (cc < this->GetNumberOfPlugins())
    {
    return (*this->Internals)[cc].FileName.c_str();
    }
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkPVPluginsInformation::GetPluginVersion(unsigned int cc)
{
  if (cc < this->GetNumberOfPlugins())
    {
    return (*this->Internals)[cc].Version.c_str();
    }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkPVPluginsInformation::GetPluginLoaded(unsigned int cc)
{
  if (cc < this->GetNumberOfPlugins())
    {
    return (*this->Internals)[cc].Loaded;
    }
  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkPVPluginsInformation::GetRequiredPlugins(unsigned int cc)
{
  if (cc < this->GetNumberOfPlugins())
    {
    return (*this->Internals)[cc].RequiredPlugins.c_str();
    }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkPVPluginsInformation::GetRequiredOnServer(unsigned int cc)
{
  if (cc < this->GetNumberOfPlugins())
    {
    return (*this->Internals)[cc].RequiredOnServer;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkPVPluginsInformation::GetRequiredOnClient(unsigned int cc)
{
  if (cc < this->GetNumberOfPlugins())
    {
    return (*this->Internals)[cc].RequiredOnClient;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkPVPluginsInformation::SetAutoLoad(unsigned int cc, bool val)
{
  if (cc < this->GetNumberOfPlugins())
    {
    (*this->Internals)[cc].AutoLoad = val;
    }
  else
    {
    vtkWarningMacro("Invalid index: " << cc);
    }
}

//----------------------------------------------------------------------------
bool vtkPVPluginsInformation::GetAutoLoad(unsigned int cc)
{
  if (cc < this->GetNumberOfPlugins())
    {
    return (*this->Internals)[cc].AutoLoad;
    }
  return false;
}
