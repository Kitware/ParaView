/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraManipulatorGUIHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCameraManipulatorGUIHelper.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVInteractorStyleCenterOfRotation.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"

vtkStandardNewMacro(vtkPVCameraManipulatorGUIHelper);
vtkCxxRevisionMacro(vtkPVCameraManipulatorGUIHelper, "1.1");
vtkCxxSetObjectMacro(vtkPVCameraManipulatorGUIHelper, PVApplication, 
  vtkPVApplication);
//-----------------------------------------------------------------------------
vtkPVCameraManipulatorGUIHelper::vtkPVCameraManipulatorGUIHelper()
{
  this->PVApplication = 0;
}

//-----------------------------------------------------------------------------
vtkPVCameraManipulatorGUIHelper::~vtkPVCameraManipulatorGUIHelper()
{
  this->SetPVApplication(0);
}

//-----------------------------------------------------------------------------
void vtkPVCameraManipulatorGUIHelper::UpdateGUI()
{
  if (!this->PVApplication)
    {
    vtkErrorMacro("PVApplication not set!");
    return;
    }
  this->PVApplication->Script("update");
}

//-----------------------------------------------------------------------------
int vtkPVCameraManipulatorGUIHelper::GetActiveSourceBounds(
  double bounds[6])
{
  if (!this->PVApplication)
    {
    vtkErrorMacro("PVApplication not set!");
    return 0;
    }
  vtkPVWindow *window = this->PVApplication->GetMainWindow();
  vtkPVSource *pvs = window->GetCurrentPVSource();
  if (pvs)
    {
    pvs->GetDataInformation()->GetBounds(bounds);
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkPVCameraManipulatorGUIHelper::GetActiveActorTranslate(double pos[3])
{
  if (!this->PVApplication)
    {
    vtkErrorMacro("PVApplication not set!");
    return 0;
    }
  vtkPVWindow *window = this->PVApplication->GetMainWindow();
  vtkPVSource *pvs = window->GetCurrentPVSource();
  if (pvs)
    {
    pvs->GetPVOutput()->GetActorTranslate(pos);
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkPVCameraManipulatorGUIHelper::SetActiveActorTranslate(double pos[3])
{
  if (!this->PVApplication)
    {
    vtkErrorMacro("PVApplication not set!");
    return 0;
    }
  vtkPVWindow *window = this->PVApplication->GetMainWindow();
  vtkPVSource *pvs = window->GetCurrentPVSource();
  if (pvs)
    {
    pvs->GetPVOutput()->SetActorTranslate(pos);
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkPVCameraManipulatorGUIHelper::GetCenterOfRotation(double center[3])
{
  if (!this->PVApplication)
    {
    vtkErrorMacro("PVApplication not set!");
    return 0;
    } 
  vtkPVWindow *window = this->PVApplication->GetMainWindow();
  float f_c[3];
  window->GetCenterOfRotationStyle()->GetCenter(f_c);
  center[0] = f_c[0];
  center[1] = f_c[1];
  center[2] = f_c[2];
  return 1;

}
//-----------------------------------------------------------------------------
void vtkPVCameraManipulatorGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PVApplication: " << this->PVApplication << endl;
}
