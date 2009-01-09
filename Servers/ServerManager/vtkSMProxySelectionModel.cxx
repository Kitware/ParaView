/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxySelectionModel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxySelectionModel.h"

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"

#include <vtkstd/vector>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMProxySelectionModel);
vtkCxxRevisionMacro(vtkSMProxySelectionModel, "1.1");

//-----------------------------------------------------------------------------
class vtkSMProxySelectionModel::vtkInternal
{
public:
  typedef  vtkstd::vector< vtkSmartPointer<vtkSMProxy> >   vtkSMSelection;
  vtkSMSelection  Selection;
  vtkSmartPointer<vtkSMProxy>  Current;

  bool Contains(vtkSMProxy*  proxy)
    {
    vtkSMSelection::iterator  iterator; 
    for (iterator = this->Selection.begin(); 
      iterator != this->Selection.end();  iterator ++)
      {
      if ( *iterator == proxy )
        {
        return  true;
        }
      }

    return  false;
    }
  
  /// Remove all occurrences, just if possible, of a proxy.
  void Remove(vtkSMProxy*  proxy)
    {
    vtkSMSelection::iterator  iterator = this->Selection.begin();
    while ( iterator != this->Selection.end() )
      {
      if ( *iterator == proxy )
        { 
        iterator = this->Selection.erase(iterator);
        }
      else
        {
        iterator ++;
        }
      }
    }
    
  void ExportSelection(vtkCollection*  collection)
    {
    
    for ( vtkSMSelection::iterator  iterator = this->Selection.begin();
      iterator != this->Selection.end();  iterator ++ )
      {
      collection->AddItem( *iterator );
      }
    }
};

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel::vtkSMProxySelectionModel()
{
  this->NewlySelected = vtkCollection::New();
  this->NewlyDeselected = vtkCollection::New();
  this->Internal = new vtkSMProxySelectionModel::vtkInternal();
}

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel::~vtkSMProxySelectionModel()
{
  this->NewlySelected->Delete();
  this->NewlyDeselected->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxySelectionModel::GetCurrentProxy()
{
  return this->Internal->Current;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::SetCurrentProxy(vtkSMProxy*  proxy,  int  command)
{ 
  if (this->Internal->Current != proxy)
    {
    this->Internal->Current = proxy;
    this->Select(proxy, command);
    this->InvokeCurrentChanged(proxy);
    }
}

//-----------------------------------------------------------------------------
bool vtkSMProxySelectionModel::IsSelected(vtkSMProxy*  proxy)
{
  return this->Internal->Contains(proxy);
}

//-----------------------------------------------------------------------------
unsigned int vtkSMProxySelectionModel::GetNumberOfSelectedProxies()
{
  return this->Internal->Selection.size();
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxySelectionModel::GetSelectedProxy(unsigned int idx)
{
  if (idx >= 0 && idx < this->GetNumberOfSelectedProxies())
    {
    return this->Internal->Selection[idx];
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::Select(vtkSMProxy* proxy, int command)
{
  vtkCollection* collection = vtkCollection::New();
  if (proxy)
    {
    collection->AddItem(proxy);
    }
  this->Select(collection, command);
  collection->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::Select(vtkCollection*  proxies, int command)
{ 
  if (command == vtkSMProxySelectionModel::NO_UPDATE)
    {
    return;
    }

  bool changed = false;

  this->NewlyDeselected->RemoveAllItems();
  this->NewlySelected->RemoveAllItems();

  if (command & vtkSMProxySelectionModel::CLEAR)
    {
    this->Internal->ExportSelection( this->NewlyDeselected );
    this->Internal->Selection.clear();
    changed = true;
    }

  vtkSMProxy* proxy;
  for (proxies->InitTraversal();
    (proxy = vtkSMProxy::SafeDownCast(proxies->GetNextItemAsObject())) != 0; )
    {
    if ((command & vtkSMProxySelectionModel::SELECT)  &&
      !this->Internal->Contains(proxy))
      {
      this->Internal->Selection.push_back(proxy);
      if (!this->NewlySelected->IsItemPresent(proxy))
        {
        this->NewlySelected->AddItem(proxy);
        changed = true;
        }
      }

    if ((command & vtkSMProxySelectionModel::DESELECT)  &&
      this->Internal->Contains(proxy))
      {
      this->Internal->Remove(proxy);
      if (!this->NewlyDeselected->IsItemPresent(proxy))
        {
        this->NewlyDeselected->AddItem(proxy);
        changed = true;
        }
      }
    }

  if (changed)
    {
    this->InvokeSelectionChanged();
    }

  this->NewlyDeselected->RemoveAllItems();
  this->NewlySelected->RemoveAllItems();
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::InvokeCurrentChanged(vtkSMProxy*  proxy)
{
  this->InvokeEvent(vtkCommand::CurrentChangedEvent, proxy);
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::InvokeSelectionChanged()
{
  this->InvokeEvent(vtkCommand::SelectionChangedEvent);
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::PrintSelf(ostream&  os,  vtkIndent  indent)
{
  this->Superclass::PrintSelf(os, indent);
}


