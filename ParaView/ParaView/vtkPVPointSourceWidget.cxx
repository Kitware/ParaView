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
#include "vtkPVWidgetProperty.h"

int vtkPVPointSourceWidget::InstanceCount = 0;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPointSourceWidget);
vtkCxxRevisionMacro(vtkPVPointSourceWidget, "1.11");

int vtkPVPointSourceWidgetCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//-----------------------------------------------------------------------------
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
  this->RadiusProperty = NULL;
  
  this->NumberOfPointsWidget = vtkPVVectorEntry::New();
  this->NumberOfPointsWidget->SetParent(this);
  this->NumberOfPointsWidget->SetTraceReferenceObject(this);
  this->NumberOfPointsWidget->SetTraceReferenceCommand(
    "GetNumberOfPointsWidget");
  this->NumberOfPointsProperty = NULL;
  
  // Start out modified so that accept will set the source
  this->ModifiedFlag = 1;
}

//-----------------------------------------------------------------------------
vtkPVPointSourceWidget::~vtkPVPointSourceWidget()
{
  this->PointWidget->Delete();
  this->RadiusWidget->Delete();
  this->NumberOfPointsWidget->Delete();
  if (this->RadiusProperty)
    {
    this->RadiusProperty->Delete();
    }
  if (this->NumberOfPointsProperty)
    {
    this->NumberOfPointsProperty->Delete();
    }
}


//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::SaveInBatchScript(ofstream *file)
{
  float pt[3];
  float rad;
  float num;
  
  if (this->SourceTclName == NULL || this->PointWidget == NULL)
    {
    vtkErrorMacro(<< this->GetClassName() << " must not have SaveInBatchScript method.");
    return;
    } 

  *file << "vtkPointSource " << this->SourceTclName << "\n";
  this->PointWidget->GetPosition(pt);
  *file << "\t" << this->SourceTclName << " SetCenter " 
        << pt[0] << " " << pt[1] << " " << pt[2] << endl; 

  this->NumberOfPointsWidget->GetValue(&num, 1);
  *file << "\t" << this->SourceTclName << " SetNumberOfPoints " 
        << (int)(num) << endl; 
  this->RadiusWidget->GetValue(&rad, 1);
  *file << "\t" << this->SourceTclName << " SetRadius " 
        << rad << endl; 
}


//-----------------------------------------------------------------------------
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
    //special for saving in tcl scripts.
    sprintf(outputName, "[%s GetOutput]", name);
    this->SetOutputTclName(outputName);
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
  this->RadiusProperty = this->RadiusWidget->CreateAppropriateProperty();
  this->RadiusProperty->SetWidget(this->RadiusWidget);
  this->Script("pack %s -side top -fill both -expand true",
               this->RadiusWidget->GetWidgetName());
  
  this->NumberOfPointsWidget->SetObjectTclName(name);
  this->NumberOfPointsWidget->SetVariableName("NumberOfPoints");
  this->NumberOfPointsWidget->SetPVSource(this->GetPVSource());
  this->NumberOfPointsWidget->SetLabel("Number of Points");
  this->NumberOfPointsWidget->SetModifiedCommand(
    this->GetPVSource()->GetTclName(), "SetAcceptButtonColorToRed");
  
  this->NumberOfPointsWidget->Create(this->Application);
  this->NumberOfPointsProperty =
    this->NumberOfPointsWidget->CreateAppropriateProperty();
  this->NumberOfPointsProperty->SetWidget(this->NumberOfPointsWidget);
  this->NumberOfPointsWidget->SetValue("1"); // match value set in point source
  this->Script("pack %s -side top -fill both -expand true",
               this->NumberOfPointsWidget->GetWidgetName());
  
  this->PointWidget->Create(this->Application);
  this->Script("pack %s -side top -fill both -expand true",
               this->PointWidget->GetWidgetName());

  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
int vtkPVPointSourceWidget::GetModifiedFlag()
{
  if (this->ModifiedFlag)
    {
    return 1;
    }
  if (this->PointWidget->GetModifiedFlag() ||
      this->RadiusWidget->GetModifiedFlag() ||
      this->NumberOfPointsWidget->GetModifiedFlag())
    {
    return 1;
    }
  return 0;
}
 

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::ResetInternal()
{
  // Ignore the source passed in.  We are updating our
  // own point source.
  this->PointWidget->ResetInternal();
  this->RadiusWidget->ResetInternal();
  this->NumberOfPointsWidget->ResetInternal();
  this->ModifiedFlag = 0;
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::AcceptInternal(const char* vtkNotUsed(sourceTclName))
{
  // Ignore the source passed in.  We are updating our
  // own point source.
  if (this->GetModifiedFlag())
    {
    this->PointWidget->AcceptInternal(this->SourceTclName);
    this->RadiusWidget->AcceptInternal(this->SourceTclName);
    this->NumberOfPointsWidget->AcceptInternal(this->SourceTclName);
    }
  this->ModifiedFlag = 0;
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  this->PointWidget->Trace(file);
  this->RadiusWidget->Trace(file);
  this->NumberOfPointsWidget->Trace(file);
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Select()
{
  this->PointWidget->Select();
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::Deselect()
{
  this->PointWidget->Deselect();
}

//-----------------------------------------------------------------------------
void vtkPVPointSourceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Point widget: " << this->PointWidget << endl;
  os << indent << "RadiusWidget: " << this->RadiusWidget << endl;
  os << indent << "NumberOfPointsWidget: " << this->NumberOfPointsWidget << endl;
}
