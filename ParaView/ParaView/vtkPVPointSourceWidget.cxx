/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPointSourceWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPointSourceWidget.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVPointWidget.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWidgetProperty.h"
#include "vtkPVProcessModule.h"

int vtkPVPointSourceWidget::InstanceCount = 0;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPointSourceWidget);
vtkCxxRevisionMacro(vtkPVPointSourceWidget, "1.15");

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
  double pt[3];
  float rad;
  float num;
  
  if (this->SourceID.ID == 0 || this->PointWidget == NULL)
    {
    vtkErrorMacro(<< this->GetClassName() << " must not have SaveInBatchScript method.");
    return;
    } 

  *file << "vtkPointSource " << "pvTemp" << this->SourceID.ID << "\n";
  this->PointWidget->GetPosition(pt);
  *file << "\t" << "pvTemp" << this->SourceID.ID << " SetCenter " 
        << pt[0] << " " << pt[1] << " " << pt[2] << endl; 

  this->NumberOfPointsWidget->GetValue(&num, 1);
  *file << "\t" << "pvTemp" << this->SourceID.ID << " SetNumberOfPoints " 
        << (int)(num) << endl; 
  this->RadiusWidget->GetValue(&rad, 1);
  *file << "\t" << "pvTemp" << this->SourceID.ID << " SetRadius " 
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
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  if (pvApp)
    {
    this->SourceID = pm->NewStreamObject("vtkPointSource");
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->SourceID << "SetNumberOfPoints" << 100 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->SourceID << "SetRadius" << 1.0 
                    << vtkClientServerStream::End;
    this->OutputID = pm->NewStreamObject("vtkPolyData");
    pm->GetStream() << vtkClientServerStream::Invoke 
                    << this->SourceID << "SetOutput" << this->OutputID 
                    << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }

  this->PointWidget->SetObjectID(this->SourceID);
  this->PointWidget->SetVariableName("Center");
  this->PointWidget->SetPVSource(this->GetPVSource());
  this->PointWidget->SetModifiedCommand(this->GetPVSource()->GetTclName(), 
                                       "SetAcceptButtonColorToRed");
  
  this->RadiusWidget->SetObjectID(this->SourceID);
  this->RadiusWidget->SetVariableName("Radius");
  this->RadiusWidget->SetPVSource(this->GetPVSource());
  this->RadiusWidget->SetLabel("Radius");
  this->RadiusWidget->SetModifiedCommand(this->GetPVSource()->GetTclName(), 
                                       "SetAcceptButtonColorToRed");
  
  this->RadiusWidget->Create(this->Application);
  this->RadiusProperty = this->RadiusWidget->CreateAppropriateProperty();
  this->RadiusProperty->SetWidget(this->RadiusWidget);
  this->RadiusWidget->SetValue("1"); // This should really be 1/10 of longest axis
  this->Script("pack %s -side top -fill both -expand true",
               this->RadiusWidget->GetWidgetName());
  
  this->NumberOfPointsWidget->SetObjectID(this->SourceID);
  this->NumberOfPointsWidget->SetVariableName("NumberOfPoints");
  this->NumberOfPointsWidget->SetPVSource(this->GetPVSource());
  this->NumberOfPointsWidget->SetLabel("Number of Points");
  this->NumberOfPointsWidget->SetModifiedCommand(
    this->GetPVSource()->GetTclName(), "SetAcceptButtonColorToRed");
  
  this->NumberOfPointsWidget->Create(this->Application);
  this->NumberOfPointsProperty =
    this->NumberOfPointsWidget->CreateAppropriateProperty();
  this->NumberOfPointsProperty->SetWidget(this->NumberOfPointsWidget);
  this->NumberOfPointsWidget->SetValue("100"); // match value set in point source
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
void vtkPVPointSourceWidget::AcceptInternal(vtkClientServerID)
{
  // Ignore the source passed in.  We are updating our
  // own point source.
  if (this->GetModifiedFlag())
    {
    this->PointWidget->AcceptInternal(this->SourceID);
    this->RadiusWidget->AcceptInternal(this->SourceID);
    this->NumberOfPointsWidget->AcceptInternal(this->SourceID);
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
