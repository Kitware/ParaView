/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModuleUI.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderModuleUI.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkSMRenderModuleProxy.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderModuleUI);
vtkCxxRevisionMacro(vtkPVRenderModuleUI, "1.16");
vtkCxxSetObjectMacro(vtkPVRenderModuleUI, RenderModuleProxy, vtkSMRenderModuleProxy);
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkPVRenderModuleUI::vtkPVRenderModuleUI()
{
  this->RenderModuleProxy = 0;
  this->OutlineThreshold = 5000000.0;
}


//----------------------------------------------------------------------------
vtkPVRenderModuleUI::~vtkPVRenderModuleUI()
{
  this->SetRenderModuleProxy(0);
}

//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::PrepareForDelete()
{
  this->SetRenderModuleProxy(0);
}

//----------------------------------------------------------------------------
vtkPVApplication* vtkPVRenderModuleUI::GetPVApplication()
{
  if (this->GetApplication() == NULL)
    {
    return NULL;
    }
  
  if (this->GetApplication()->IsA("vtkPVApplication"))
    {  
    return (vtkPVApplication*)(this->GetApplication());
    }
  else
    {
    vtkErrorMacro("Bad typecast");
    return NULL;
    } 
}

//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();
}
//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::ResetSettingsToDefault()
{
  
}

//----------------------------------------------------------------------------
void vtkPVRenderModuleUI::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OutlineThreshold: " << this->OutlineThreshold << endl;
  os << indent << "RenderModuleProxy: " << this->RenderModuleProxy << endl;
}

