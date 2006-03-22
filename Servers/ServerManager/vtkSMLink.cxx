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

vtkCxxRevisionMacro(vtkSMLink, "1.2");
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
    }
  ~vtkSMLinkObserver()
    {
      this->Link = 0;
    }
  
  virtual void Execute(vtkObject *c, unsigned long event, void* pname)
    {
    vtkSMProxy* caller = vtkSMProxy::SafeDownCast(c);
    if (this->Link && caller)
      {
      if (event == vtkCommand::UpdateEvent)
        {
        this->Link->UpdateVTKObjects(caller);
        }
      else if (event == vtkCommand::PropertyModifiedEvent)
        {
        this->Link->UpdateProperties(caller, (const char*)pname);
        }
      }
    }

  vtkSMLink* Link;
};

//-----------------------------------------------------------------------------
vtkSMLink::vtkSMLink()
{
  vtkSMLinkObserver* obs = vtkSMLinkObserver::New();
  obs->Link = this;
  this->Observer = obs;
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
}

//-----------------------------------------------------------------------------
void vtkSMLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
