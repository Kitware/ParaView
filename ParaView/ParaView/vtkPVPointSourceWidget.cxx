/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPointSourceWidget.cxx
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
#include "vtkPVPointSourceWidget.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVPointWidget.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"

int vtkPVPointSourceWidget::InstanceCount = 0;

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPointSourceWidget);

int vtkPVPointSourceWidgetCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPointSourceWidget::vtkPVPointSourceWidget()
{
  this->CommandFunction = vtkPVPointSourceWidgetCommand;
  this->PointWidget = vtkPVPointWidget::New();
  this->PointWidget->SetParent(this);
  this->PointWidget->SetTraceReferenceObject(this);
  this->PointWidget->SetTraceReferenceCommand("GetPointWidget");
  this->PointWidget->SetUseLabel(0);

  this->RadiusWidget = vtkPVVectorEntry::New();
  this->RadiusWidget->SetParent(this);
  this->RadiusWidget->SetTraceReferenceObject(this);
  this->RadiusWidget->SetTraceReferenceCommand("GetRadiusWidget");

  this->NumberOfPointsWidget = vtkPVVectorEntry::New();
  this->NumberOfPointsWidget->SetParent(this);
  this->NumberOfPointsWidget->SetTraceReferenceObject(this);
  this->NumberOfPointsWidget->SetTraceReferenceCommand(
    "GetNumberOfPointsWidget");
}

//----------------------------------------------------------------------------
vtkPVPointSourceWidget::~vtkPVPointSourceWidget()
{
  this->PointWidget->Delete();
  this->RadiusWidget->Delete();
  this->NumberOfPointsWidget->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  char name[256];
  sprintf(name, "PointSourceWidget%d", vtkPVPointSourceWidget::InstanceCount);

  char outputName[256];
  sprintf(outputName, "PointSourceWidgetOutput%d", 
          vtkPVPointSourceWidget::InstanceCount++);

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);
  if (pvApp)
    {
    this->SetSourceTclName(name);
    this->SetOutputTclName(outputName);

    pvApp->BroadcastScript("vtkPointSource %s;"
                           "%s SetNumberOfPoints 1;"
                           "%s SetRadius 0.0;"
                           "vtkPolyData %s;"
                           "%s SetOutput %s;", 
                           name,
                           name,
                           name,
                           outputName,
                           name, outputName);
    }

  this->PointWidget->SetObjectTclName(name);
  this->PointWidget->SetVariableName("Center");
  this->PointWidget->SetPVSource(this->GetPVSource());
  this->PointWidget->SetModifiedCommand(this->GetPVSource()->GetTclName(), 
                                       "SetAcceptButtonColorToRed");
  
  this->RadiusWidget->SetObjectTclName(name);
  this->RadiusWidget->SetVariableName("Radius");
  this->RadiusWidget->SetPVSource(this->GetPVSource());
  this->RadiusWidget->SetLabel("Radius");
  this->RadiusWidget->SetModifiedCommand(this->GetPVSource()->GetTclName(), 
                                       "SetAcceptButtonColorToRed");
  
  this->RadiusWidget->Create(this->Application);
  this->Script("pack %s -side top -fill both -expand true",
               this->RadiusWidget->GetWidgetName());

  this->NumberOfPointsWidget->SetObjectTclName(name);
  this->NumberOfPointsWidget->SetVariableName("NumberOfPoints");
  this->NumberOfPointsWidget->SetPVSource(this->GetPVSource());
  this->NumberOfPointsWidget->SetLabel("Number of Points");
  this->NumberOfPointsWidget->SetModifiedCommand(
    this->GetPVSource()->GetTclName(), "SetAcceptButtonColorToRed");
  
  this->NumberOfPointsWidget->Create(this->Application);
  this->Script("pack %s -side top -fill both -expand true",
               this->NumberOfPointsWidget->GetWidgetName());

  this->PointWidget->Create(this->Application);
  this->Script("pack %s -side top -fill both -expand true",
               this->PointWidget->GetWidgetName());


}

//----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Reset()
{
  this->ModifiedFlag = 0;
  this->PointWidget->Reset();
  this->RadiusWidget->Reset();
  this->NumberOfPointsWidget->Reset();
}

//----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Accept()
{
  this->PointWidget->Accept();
  this->RadiusWidget->Accept();
  this->NumberOfPointsWidget->Accept();
  
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Select()
{
  this->PointWidget->Select();
}

//----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Deselect()
{
  this->PointWidget->Deselect();
}

//----------------------------------------------------------------------------
void vtkPVPointSourceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Point widget: " << this->PointWidget << endl;
}
