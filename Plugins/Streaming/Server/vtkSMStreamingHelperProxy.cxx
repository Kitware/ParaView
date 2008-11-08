/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingHelperProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStreamingHelperProxy.h"

#include "vtkStreamingFactory.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyIterator.h"

#include <vtkstd/vector>

//-----------------------------------------------------------------------------
int vtkSMStreamingHelperProxy::StreamingFactoryRegistered = 0;

//-----------------------------------------------------------------------------
class vtkSMStreamingHelperProxy::vtkSMStreamingHelperObserver : public vtkCommand
{
public:
  static vtkSMStreamingHelperObserver* New()
    { 
    return new vtkSMStreamingHelperObserver; 
    }
  
  void SetTarget(vtkSMStreamingHelperProxy* t)
    {
    this->Target = t;
    }

  virtual void Execute(vtkObject* caller, unsigned long eventid, void* data)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(caller, eventid, data);
      }
    }

private:
  vtkSMStreamingHelperObserver()
    {
    this->Target = 0;
    }

  vtkSMStreamingHelperProxy* Target;
};

//-----------------------------------------------------------------------------
class vtkSMStreamingHelperProxy::vtkInternal
{
public:
  vtkInternal()
  {

  }

  vtkstd::vector<vtkSMProxy*> Readers;
};


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMStreamingHelperProxy);
vtkCxxRevisionMacro(vtkSMStreamingHelperProxy, "1.1");

//----------------------------------------------------------------------------
vtkSMStreamingHelperProxy::vtkSMStreamingHelperProxy()
{
  //this->StreamedPasses = 0;
  this->StreamedPassesNeedsUpdate = true;
  this->EnableStreamMessages = false;


  this->StreamedPasses = 16;
  this->UseCulling = true;
  this->UseViewOrdering = true;
  this->PieceCacheLimit = 16;
  this->PieceRenderCutoff = 16;


  this->Internal = new vtkInternal;

  this->Observer = vtkSMStreamingHelperObserver::New();
  this->Observer->SetTarget(this);

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  if (!pxm)
    {
    vtkErrorMacro("vtkSMStreamingHelper must be created only"
       << " after the ProxyManager has been created.");
    }
  else
    {
    pxm->AddObserver(vtkCommand::RegisterEvent, this->Observer, 1);
    pxm->AddObserver(vtkCommand::UnRegisterEvent, this->Observer, 1);
    }

  // Register the streaming object factory
  if (!vtkSMStreamingHelperProxy::StreamingFactoryRegistered)
    {
    vtkStreamingFactory* sf = vtkStreamingFactory::New();
    vtkObjectFactory::RegisterFactory(sf);
    vtkSMStreamingHelperProxy::StreamingFactoryRegistered = 1;
    sf->Delete();
    }

}

//----------------------------------------------------------------------------
vtkSMStreamingHelperProxy::~vtkSMStreamingHelperProxy()
{
  delete this->Internal;
  this->Observer->Delete();
}

//----------------------------------------------------------------------------
void vtkSMStreamingHelperProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
const char* vtkSMStreamingHelperProxy::GetInstanceName()
{
  static const char* name = "StreamingHelperInstance";
  return name;
}

//-----------------------------------------------------------------------------
void vtkSMStreamingHelperProxy::ExecuteEvent(vtkObject* vtkNotUsed(caller), 
  unsigned long eventid, void* data)
{
  switch (eventid)
    {
  case vtkCommand::RegisterEvent:
      {
      vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
        vtkSMProxyManager::RegisteredProxyInformation*>(data));

      if (!info.IsCompoundProxyDefinition && !info.IsLink)
        {
        this->OnRegisterProxy(info.GroupName, info.ProxyName, info.Proxy);
        }
      }
    break;
  case vtkCommand::UnRegisterEvent:
      {
      vtkSMProxyManager::RegisteredProxyInformation &info =*(reinterpret_cast<
        vtkSMProxyManager::RegisteredProxyInformation*>(data));

      if (!info.IsCompoundProxyDefinition && !info.IsLink)
        {
        this->OnUnRegisterProxy(info.GroupName, info.ProxyName, info.Proxy);
        }
      }
    break;
  default:
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkSMStreamingHelperProxy::OnRegisterProxy(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  if (proxy->IsA("vtkSMFileSeriesReaderProxy") &&
      proxy->GetProperty("NumberOfPieces"))
    {
    // Add reader to list
    this->Internal->Readers.push_back(proxy);

    // Flag that StreamedPasses needs an update.  We cannot recompute
    // StreamedPasses right now becaues the proxy has just been registered
    // and its property information is probably not valid yet.
    this->StreamedPassesNeedsUpdate = true;
    }
}

//-----------------------------------------------------------------------------
void vtkSMStreamingHelperProxy::OnUnRegisterProxy(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  if (proxy->IsA("vtkSMFileSeriesReaderProxy") &&
      proxy->GetProperty("NumberOfPieces"))
    {
    // Look for the reader in our list
    for (unsigned int i = 0; i < this->Internal->Readers.size(); ++i)
      {
      if (this->Internal->Readers[i] == proxy)
        {
        // Reader has been found, remove it from the list.
        this->Internal->Readers[i] = this->Internal->Readers.back();
        this->Internal->Readers.pop_back();
        }
      }
    }
}

//-----------------------------------------------------------------------------
//int vtkSMStreamingHelperProxy::GetStreamedPasses()
//{
//  return this->GetStreamedPasses(this->StreamedPassesNeedsUpdate);
//}

//-----------------------------------------------------------------------------
int vtkSMStreamingHelperProxy::GetStreamedPasses(bool recompute)
{
  if (!recompute)
    {
    return this->StreamedPasses;
    }

  // Default to one
  this->StreamedPasses = 16;

  // Check each reader for number of passes, save the minimum value reported.
  for (unsigned int i = 0; i < this->Internal->Readers.size(); ++i)
    {
    vtkSMIntVectorProperty * prop = vtkSMIntVectorProperty::SafeDownCast(
      this->Internal->Readers[i]->GetProperty("NumberOfPieces"));
    this->Internal->Readers[i]->UpdatePropertyInformation(prop);
    int passes = prop->GetElement(0);
    if (passes < this->StreamedPasses || this->StreamedPasses == 1)
      {
      this->StreamedPasses = passes;
      }
    }
  this->StreamedPassesNeedsUpdate = false;
  return this->StreamedPasses;
}


//----------------------------------------------------------------------------
int vtkSMStreamingHelperProxy::GetPassesFromSource(vtkSMSourceProxy * source)
{

  // Check for null
  if (!source)
    {
    return 0;
    }

  printf("vtkSMStreamingHelperProxy::GetPassesFromSource: %s\n", source->GetVTKClassName());

  // See if the source has NumberOfPieces property
  vtkSMIntVectorProperty * prop = vtkSMIntVectorProperty::SafeDownCast(
    source->GetProperty("NumberOfPieces"));
  if (prop)
    {
    // UpdatePropertyInformation(vtkSMProperty*) will not work
    // if the property is an exposed property...
    //source->UpdatePropertyInformation(prop);
    source->UpdatePropertyInformation();
    return prop->GetElement(0);
    }

  // Didn't find the property, so check each of the source's inputs
  int minPassesFound = 1;
  vtkSmartPointer<vtkSMPropertyIterator> itr;
  itr.TakeReference(source->NewPropertyIterator());
  for (itr->Begin(); !itr->IsAtEnd(); itr->Next())
    {
    vtkSMInputProperty* inputProp = vtkSMInputProperty::SafeDownCast(
      itr->GetProperty());
    if (inputProp)
      {
      int num = inputProp->GetNumberOfProxies();
      for (int j = 0; j < num; ++j)
        {
        vtkSMSourceProxy * proxy = vtkSMSourceProxy::SafeDownCast(
          inputProp->GetProxy(j));
        if (!proxy)
          {
          continue;
          }
        int passes = this->GetPassesFromSource(vtkSMSourceProxy::SafeDownCast(proxy));
        if (passes < minPassesFound || minPassesFound == 1)
          {
          minPassesFound = passes;
          }
        }
      }
    }

  return minPassesFound;
}


