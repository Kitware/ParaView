/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClipPlane.cxx
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
#include "vtkPVClipPlane.h"
#include "vtkPVPlaneWidget.h"
#include "vtkPVVectorEntry.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkPVWindow.h"
#include "vtkKWCompositeCollection.h"
#include "vtkPVData.h"
#include "vtkObjectFactory.h"

int vtkPVClipPlaneCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVClipPlane::vtkPVClipPlane()
{
  this->CommandFunction = vtkPVClipPlaneCommand;

  this->BoundsDisplay = vtkKWBoundsDisplay::New();

  this->ReplaceInputOn();
}

//----------------------------------------------------------------------------
vtkPVClipPlane::~vtkPVClipPlane()
{
  this->BoundsDisplay->Delete();
  this->BoundsDisplay = NULL;
}

//----------------------------------------------------------------------------
vtkPVClipPlane* vtkPVClipPlane::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVClipPlane");
  if(ret)
    {
    return (vtkPVClipPlane*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVClipPlane;
}


//----------------------------------------------------------------------------
void vtkPVClipPlane::CreateProperties()
{
  vtkPVPlaneWidget *planeWidget;
  vtkPVVectorEntry *offsetEntry;

  this->vtkPVSource::CreateProperties();
 
  this->AddInputMenu("Input", "PVInput", "vtkDataSet",
                     "Set the input to this filter.",
                     this->GetPVWindow()->GetSources());

  this->BoundsDisplay->SetParent(this->GetParameterFrame()->GetFrame());
  this->BoundsDisplay->Create(this->Application);
  this->BoundsDisplay->SetLabel("Input Bounds");
  this->Script("pack %s -side top -fill x",
               this->BoundsDisplay->GetWidgetName());

  // The Plane widget makes its owns VTK object (vtkPlane) so we do not
  // need to make the association.
  planeWidget = vtkPVPlaneWidget::New();
  planeWidget->SetParent(this->GetParameterFrame()->GetFrame());
  planeWidget->SetPVSource(this);
  planeWidget->SetModifiedCommand(this->GetTclName(),"ChangeAcceptButtonColor");
  planeWidget->Create(this->Application);
  this->Script("pack %s -side top -fill x", planeWidget->GetWidgetName());
  planeWidget->SetTraceName("Plane");
  this->Widgets->AddItem(planeWidget);
  // This makes the assumption that the vtkClipDataSet filter 
  // has already been set.  In the future we could also implement 
  // the virtual method "SetVTKSource" to catch the condition when
  // the VTKSource is set after the properties are created.
  if (this->VTKSourceTclName != NULL)
    {
    this->Script("%s SetClipFunction %s", this->VTKSourceTclName,
                 planeWidget->GetPlaneTclName());
    }
  else
    {
    vtkErrorMacro("VTKSource must be set before properties are created.");
    }
  planeWidget->Delete();
  planeWidget = NULL;

  // Offset -------------------------
  offsetEntry = vtkPVVectorEntry::New();
  offsetEntry->SetParent(this->GetParameterFrame()->GetFrame());
  offsetEntry->SetObjectVariable(this->GetVTKSourceTclName(), "Value");
  offsetEntry->SetModifiedCommand(this->GetTclName(), 
                                  "ChangeAcceptButtonColor");
  offsetEntry->Create(this->Application, "Offset", 1, NULL, NULL);
  this->Widgets->AddItem(offsetEntry);
  this->Script("pack %s -side top -fill x", offsetEntry->GetWidgetName());
  offsetEntry->Delete();
  offsetEntry = NULL;

  this->UpdateProperties();
  this->UpdateParameterWidgets();
}


//----------------------------------------------------------------------------
void vtkPVClipPlane::UpdateParameterWidgets()
{
  vtkPVData *input;
  float bds[6];

  this->vtkPVSource::UpdateParameterWidgets();
  input = this->GetPVInput();
  if (input == NULL)
    {
    bds[0] = bds[2] = bds[4] = VTK_LARGE_FLOAT;
    bds[1] = bds[3] = bds[5] = -VTK_LARGE_FLOAT;
    }
  else
    {
    input->GetBounds(bds);
    }

  this->BoundsDisplay->SetBounds(bds);
}





