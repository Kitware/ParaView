/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLink.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLink.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <vtkstd/list>

//-----------------------------------------------------------------------------
class vtkSMLinkObserver : public vtkCommand
{
public:

  static vtkSMLinkObserver* New()
    {
      return new vtkSMLinkObserver;
    }
  vtkSMLinkObserver()
    {
      this->Link = 0;
      this->InProgress = false;
    }
  ~vtkSMLinkObserver()
    {
      this->Link = 0;
    }
  
  virtual void Execute(vtkObject *c, unsigned long event, void* pname)
    {
    if(this->InProgress)
      {
      return;
      }

    if (this->Link && !this->Link->GetEnabled())
      {
      return;
      }

    this->InProgress = true;
    vtkSMProxy* caller = vtkSMProxy::SafeDownCast(c);
    if (this->Link && caller)
      {
      if (event == vtkCommand::UpdateEvent && 
        this->Link->GetPropagateUpdateVTKObjects())
        {
        this->Link->UpdateVTKObjects(caller);
        }
      else if (event == vtkCommand::PropertyModifiedEvent)
        {
        this->Link->PropertyModified(caller, (const char*)pname);
        }
      else if (event == vtkCommand::UpdatePropertyEvent)
        {
        this->Link->UpdateProperty(caller, reinterpret_cast<char*>(pname));
        }
      }
    this->InProgress = false;
    }

  vtkSMLink* Link;
  bool InProgress;
};

//-----------------------------------------------------------------------------
vtkSMLink::vtkSMLink()
{
  vtkSMLinkObserver* obs = vtkSMLinkObserver::New();
  obs->Link = this;
  this->Observer = obs;
  this->PropagateUpdateVTKObjects = 1;
  this->Enabled = true;
}

//-----------------------------------------------------------------------------
vtkSMLink::~vtkSMLink()
{
  ((vtkSMLinkObserver*)this->Observer)->Link = NULL;
  this->Observer->Delete();
  this->Observer = NULL;
}

//-----------------------------------------------------------------------------
void vtkSMLink::ObserveProxyUpdates(vtkSMProxy* proxy)
{
  proxy->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer);
  proxy->AddObserver(vtkCommand::UpdateEvent, this->Observer);
  proxy->AddObserver(vtkCommand::UpdatePropertyEvent, this->Observer);
}

//-----------------------------------------------------------------------------
void vtkSMLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "PropagateUpdateVTKObjects: " <<
    this->PropagateUpdateVTKObjects << endl;
}
