/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPickSphereWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPickSphereWidgetProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkCommand.h"
// ATTRIBUTE EDITOR
//#include "vtkSphereWidget.h"
#include "vtkPickSphereWidget.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkCommand.h"

vtkStandardNewMacro(vtkSMPickSphereWidgetProxy);
vtkCxxRevisionMacro(vtkSMPickSphereWidgetProxy, "1.3");

//----------------------------------------------------------------------------
vtkSMPickSphereWidgetProxy::vtkSMPickSphereWidgetProxy()
{
//  ATTRIBUTE EDITOR
//  this->SetVTKClassName("vtkSphereWidget");
  this->SetVTKClassName("vtkPickSphereWidget");
  this->MouseControlToggle = 0;
}

//----------------------------------------------------------------------------
vtkSMPickSphereWidgetProxy::~vtkSMPickSphereWidgetProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMPickSphereWidgetProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

// ATTRIBUTE EDITOR
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream str;
  unsigned int cc;
  unsigned int numObjects = this->GetNumberOfIDs();
  for(cc=0;cc < numObjects; cc++)
    {
    str << vtkClientServerStream::Invoke << this->GetID(cc)
      << "SetMouseControlToggle" << this->MouseControlToggle << vtkClientServerStream::End;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->ConnectionID, this->Servers,str,0);
    } 

}

//----------------------------------------------------------------------------
void vtkSMPickSphereWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SetMouseControlToggle" << this->GetMouseControlToggle() << endl;
}
