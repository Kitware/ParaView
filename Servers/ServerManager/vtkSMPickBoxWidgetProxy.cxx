/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPickBoxWidgetProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPickBoxWidgetProxy.h"

#include "vtkImplicitPlaneWidget.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkTransform.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMDoubleVectorProperty.h"
// ATTRIBUTE EDITOR
//#include "vtkBoxWidget.h"
#include "vtkPickBoxWidget.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkClientServerID.h"
#include "vtkSMRenderModuleProxy.h"

vtkStandardNewMacro(vtkSMPickBoxWidgetProxy);
vtkCxxRevisionMacro(vtkSMPickBoxWidgetProxy, "1.2");

//----------------------------------------------------------------------------
vtkSMPickBoxWidgetProxy::vtkSMPickBoxWidgetProxy()
{
// ATTRIBUTE EDITOR
  this->SetVTKClassName("vtkPickBoxWidget");
  this->MouseControlToggle = 0;
}

//----------------------------------------------------------------------------
vtkSMPickBoxWidgetProxy::~vtkSMPickBoxWidgetProxy()
{

}

//----------------------------------------------------------------------------
void vtkSMPickBoxWidgetProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

// ATTRIBUTE EDITOR
  if (!this->CurrentRenderModuleProxy)
    {
    return; // widgets are not enabled till rendermodule is set.
    }
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
void vtkSMPickBoxWidgetProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SetMouseControlToggle" << this->GetMouseControlToggle() << endl;
}
