/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClip.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVClip.h"
#include "vtkPVSelectWidget.h"
#include "vtkPVPlaneWidget.h"
#include "vtkPVSphereWidget.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVVectorEntry.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkPVWindow.h"
#include "vtkKWCompositeCollection.h"
#include "vtkPVData.h"
#include "vtkPVInputMenu.h"
#include "vtkObjectFactory.h"
#include "vtkKWFrame.h"

int vtkPVClipCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVClip::vtkPVClip()
{
  this->CommandFunction = vtkPVClipCommand;
  this->ReplaceInputOn();
}

//----------------------------------------------------------------------------
vtkPVClip::~vtkPVClip()
{
}

//----------------------------------------------------------------------------
vtkPVClip* vtkPVClip::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVClip");
  if(ret)
    {
    return (vtkPVClip*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVClip;
}


//----------------------------------------------------------------------------
void vtkPVClip::CreateProperties()
{
  vtkPVInputMenu *inputMenu;
  vtkPVSelectWidget *selectWidget;
  vtkPVPlaneWidget *planeWidget;
  vtkPVSphereWidget *sphereWidget;
  vtkPVArrayMenu *arrayMenu;
  vtkPVVectorEntry *offsetEntry;

  // This makes the assumption that the vtkClipDataSet filter 
  // has already been set.  In the future we could also implement 
  // the virtual method "SetVTKSource" to catch the condition when
  // the VTKSource is set after the properties are created.
  if (this->VTKSourceTclName == NULL)
    {
    vtkErrorMacro("VTKSource must be set before properties are created.");
    return;
    }

  this->vtkPVSource::CreateProperties();
 
  inputMenu = this->AddInputMenu("Input", "PVInput", "vtkDataSet",
                                 "Set the input to this filter.",
                                 this->GetPVWindow()->GetSources());

  this->AddBoundsDisplay(inputMenu);

  
  selectWidget = vtkPVSelectWidget::New();
  selectWidget->UseWidgetCommandOn();
  selectWidget->SetParent(this->GetParameterFrame()->GetFrame());
  selectWidget->Create(this->Application);
  selectWidget->SetLabel("Clip Function");
  selectWidget->SetModifiedCommand(this->GetTclName(), "SetAcceptButtonColorToRed");
  selectWidget->SetObjectVariable(this->VTKSourceTclName, "ClipFunction");
  this->AddPVWidget(selectWidget);
  this->Script("pack %s -side top -fill x -expand 1", 
               selectWidget->GetWidgetName());

  
  // The Plane widget makes its owns VTK object (vtkPlane) so we do not
  // need to make the association.
  planeWidget = vtkPVPlaneWidget::New();
  planeWidget->SetParent(selectWidget->GetFrame());
  planeWidget->SetPVSource(this);
  planeWidget->SetModifiedCommand(this->GetTclName(),"SetAcceptButtonColorToRed");
  planeWidget->Create(this->Application);  
  selectWidget->AddItem("Plane", planeWidget, "GetPlaneTclName");
  
  // The Plane widget makes its owns VTK object (vtkPlane) so we do not
  // need to make the association.
  sphereWidget = vtkPVSphereWidget::New();
  sphereWidget->SetParent(selectWidget->GetFrame());
  sphereWidget->SetPVSource(this);
  sphereWidget->SetModifiedCommand(this->GetTclName(),"SetAcceptButtonColorToRed");
  sphereWidget->Create(this->Application);
  selectWidget->AddItem("Sphere", sphereWidget, "GetSphereTclName");

  // Put an option to clip by scalar array.
  arrayMenu = vtkPVArrayMenu::New();
  arrayMenu->SetNumberOfComponents(1);
  arrayMenu->SetInputName("Input");
  arrayMenu->SetAttributeType(vtkDataSetAttributes::SCALARS);
  arrayMenu->SetObjectTclName(this->VTKSourceTclName);
  arrayMenu->SetLabel("Scalars");
  arrayMenu->ShowScalarRangeLabelOn();

  arrayMenu->SetParent(selectWidget->GetFrame());
  arrayMenu->SetModifiedCommand(this->GetTclName(),"SetAcceptButtonColorToRed");
  arrayMenu->Create(this->Application);
  arrayMenu->SetBalloonHelpString("Choose the clipping scalar array.");
  selectWidget->AddItem("Scalars", arrayMenu, 0);

  // Set up the dependancy so that the array menu updates when the input changes.
  inputMenu->AddDependent(arrayMenu);
  arrayMenu->SetInputMenu(inputMenu);  

  arrayMenu->Delete();
  arrayMenu = NULL;
  planeWidget->Delete();
  planeWidget = NULL;
  sphereWidget->Delete();
  sphereWidget = NULL;
  selectWidget->Delete();
  selectWidget = NULL;

  // Offset -------------------------
  offsetEntry = vtkPVVectorEntry::New();
  offsetEntry->SetParent(this->GetParameterFrame()->GetFrame());
  offsetEntry->SetObjectVariable(this->GetVTKSourceTclName(), "Value");
  offsetEntry->SetModifiedCommand(this->GetTclName(), 
                                  "SetAcceptButtonColorToRed");
  offsetEntry->SetLabel("Offset");
  offsetEntry->Create(this->Application);
  this->AddPVWidget(offsetEntry);
  this->Script("pack %s -side top -fill x", offsetEntry->GetWidgetName());
  offsetEntry->Delete();
  offsetEntry = NULL;

  // Inside out -------------------------
  this->AddLabeledToggle("Inside Out", "InsideOut", "Switches which part to keep."); 

  this->UpdateProperties();
  this->UpdateParameterWidgets();
}






