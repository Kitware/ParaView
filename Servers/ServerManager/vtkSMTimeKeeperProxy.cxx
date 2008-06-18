/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeKeeperProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTimeKeeperProxy.h"

#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMViewProxy.h"

vtkStandardNewMacro(vtkSMTimeKeeperProxy);
vtkCxxRevisionMacro(vtkSMTimeKeeperProxy, "1.2");
//----------------------------------------------------------------------------
vtkSMTimeKeeperProxy::vtkSMTimeKeeperProxy()
{
  this->Views = vtkCollection::New();
  this->Time = 0.0;
}

//----------------------------------------------------------------------------
vtkSMTimeKeeperProxy::~vtkSMTimeKeeperProxy()
{
  this->Views->Delete();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::AddView(vtkSMViewProxy* view)
{
  if (view && !this->Views->IsItemPresent(view))
    {
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      view->GetProperty("ViewTime"));
    if (!dvp)
      {
      vtkErrorMacro("Failed to locate ViewTime property.");
      }
    else
      {
      this->Views->AddItem(view);
      dvp->SetElement(0, this->Time);
      view->UpdateProperty("ViewTime");
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::RemoveView(vtkSMViewProxy* view)
{
  if (view)
    {
    this->Views->RemoveItem(view);
    }
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::RemoveAllViews()
{
  this->Views->RemoveAllItems();
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::SetTime(double time)
{
  if (this->Time != time)
    {
    this->Time = time;
    for (int cc=0; cc < this->Views->GetNumberOfItems(); cc++)
      {
      vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(
        this->Views->GetItemAsObject(cc));
      if (view)
        {
        vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
          view->GetProperty("ViewTime"));
        dvp->SetElement(0, this->Time);
        view->UpdateProperty("ViewTime");
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMTimeKeeperProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Time: " << this->Time << endl;
}


