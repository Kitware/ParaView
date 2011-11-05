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

#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMOutputPort.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMProxySelectionModel);
//-----------------------------------------------------------------------------
vtkSMProxySelectionModel::vtkSMProxySelectionModel()
{
}

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel::~vtkSMProxySelectionModel()
{
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxySelectionModel::GetCurrentProxy()
{
  return this->Current;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::SetCurrentProxy(vtkSMProxy*  proxy,  int  command)
{ 
  if (this->Current != proxy)
    {
    this->Current = proxy;
    this->Select(proxy, command);
    this->InvokeCurrentChanged(proxy);
    }
}

//-----------------------------------------------------------------------------
bool vtkSMProxySelectionModel::IsSelected(vtkSMProxy*  proxy)
{
  return this->Selection.find(proxy) != this->Selection.end();
}

//-----------------------------------------------------------------------------
unsigned int vtkSMProxySelectionModel::GetNumberOfSelectedProxies()
{
  return static_cast<unsigned int>(this->Selection.size());
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxySelectionModel::GetSelectedProxy(unsigned int idx)
{
  if (idx < this->GetNumberOfSelectedProxies())
    {
    SelectionType::iterator iter = this->Selection.begin();
    for (unsigned int cc=0; cc < idx; ++cc, ++iter)
      {
      }
    return vtkSMProxy::SafeDownCast(iter->GetPointer());
    }

  return NULL;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::Select(vtkSMProxy* proxy, int command)
{
  SelectionType selection;
  if (proxy)
    {
    selection.insert(proxy);
    }
  this->Select(selection, command);
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::Select(
  const vtkSMProxySelectionModel::SelectionType& proxies, int command)
{ 
  if (command == vtkSMProxySelectionModel::NO_UPDATE)
    {
    return;
    }

  SelectionType new_selection;

  if (command & vtkSMProxySelectionModel::CLEAR)
    {
    // everything from old-selection needs to be removed. 
    }
  else
    {
    // start with existing selection.
    new_selection = this->Selection;
    }

  for (SelectionType::iterator iter = proxies.begin();
    iter != proxies.end(); ++iter)
    {
    vtkSMProxy* proxy = iter->GetPointer();
    if (proxy && (command & vtkSMProxySelectionModel::SELECT) !=0)
      {
      new_selection.insert(proxy);
      }
    if (proxy && (command & vtkSMProxySelectionModel::DESELECT) !=0)
      {
      new_selection.erase(proxy);
      }
    }

  bool changed = (this->Selection != new_selection);
  if (changed)
    {
    this->Selection = new_selection;
    this->InvokeSelectionChanged();
    }
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
bool vtkSMProxySelectionModel::GetSelectionDataBounds(double bounds[6])
{
  vtkBoundingBox bbox;
  for (SelectionType::iterator iter = this->Selection.begin();
    iter != this->Selection.end(); ++iter)
    {
    vtkSMProxy* proxy = iter->GetPointer();
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(proxy);
    vtkSMOutputPort* opPort = vtkSMOutputPort::SafeDownCast(proxy);
    if (source)
      {
      for (unsigned int kk=0; kk <  source->GetNumberOfOutputPorts(); kk++)
        {
        bbox.AddBounds(source->GetDataInformation(kk)->GetBounds());
        }
      }
    else if (opPort)
      {
      bbox.AddBounds(opPort->GetDataInformation()->GetBounds());
      }
    }
  if (bbox.IsValid())
    {
    bbox.GetBounds(bounds);
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::PrintSelf(ostream&  os,  vtkIndent  indent)
{
  this->Superclass::PrintSelf(os, indent);
}
