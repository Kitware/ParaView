/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLineSourceWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLineSourceWidget.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVLineWidget.h"
#include "vtkPVSource.h"
#include "vtkPVProcessModule.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLineSourceWidget);
vtkCxxRevisionMacro(vtkPVLineSourceWidget, "1.19");

int vtkPVLineSourceWidgetCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVLineSourceWidget::vtkPVLineSourceWidget()
{
  this->CommandFunction = vtkPVLineSourceWidgetCommand;
  this->LineWidget = vtkPVLineWidget::New();
  this->LineWidget->SetParent(this);
  this->LineWidget->SetTraceReferenceObject(this);
  this->LineWidget->SetTraceReferenceCommand("GetLineWidget");
  this->LineWidget->SetUseLabel(0);
}

//----------------------------------------------------------------------------
vtkPVLineSourceWidget::~vtkPVLineSourceWidget()
{
  this->LineWidget->Delete();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  if (pvApp)
    {
    this->SourceID = pm->NewStreamObject("vtkLineSource");
    this->OutputID = pm->NewStreamObject("vtkPolyData");
    pm->GetStream() << vtkClientServerStream::Invoke << this->SourceID 
                    << "SetOutput" << this->OutputID << vtkClientServerStream::End;
    pm->SendStreamToServer();
    }



  this->LineWidget->SetPoint1VariableName("Point1");
  this->LineWidget->SetPoint2VariableName("Point2");
  this->LineWidget->SetResolutionVariableName("Resolution");
  this->LineWidget->SetPVSource(this->GetPVSource());
  this->LineWidget->SetModifiedCommand(this->GetPVSource()->GetTclName(), 
                                       "SetAcceptButtonColorToModified");
  
  this->LineWidget->Create(this->Application);
  this->Script("pack %s -side top -fill both -expand true",
               this->LineWidget->GetWidgetName());
}


//----------------------------------------------------------------------------
int vtkPVLineSourceWidget::GetModifiedFlag()
{
  if (this->ModifiedFlag || this->LineWidget->GetModifiedFlag())
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::ResetInternal()
{
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
  // Ignore the source passed in.  Modify our one source.
  this->LineWidget->ResetInternal();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::AcceptInternal(vtkClientServerID)
{
  // Ignore the source passed in.  Modify our one source.
  this->LineWidget->AcceptInternal(this->SourceID);
  
  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVLineSourceWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  this->LineWidget->Trace(file);
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Select()
{
  this->LineWidget->Select();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Deselect()
{
  this->LineWidget->Deselect();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::SaveInBatchScript(ofstream *file)
{
  double pt[3];
  
  if (this->OutputID.ID == 0 || this->LineWidget == NULL)
    {
    vtkErrorMacro(<< this->GetClassName() << " must not have SaveInBatchScript method.");
    return;
    } 

  *file << endl;
  *file << "set pvTemp" <<  this->OutputID.ID
        << " [$proxyManager NewProxy sources LineSource]"
        << endl;
  *file << "  $proxyManager RegisterProxy sources pvTemp"
        << this->OutputID.ID << " $pvTemp" << this->OutputID.ID
        << endl;
  *file << " $pvTemp" << this->OutputID.ID << " UnRegister {}" << endl;

  this->LineWidget->GetPoint1(pt);
  *file << "  [$pvTemp" << this->OutputID.ID << " GetProperty Point1] "
        << "SetElements3 " << pt[0] << " " << pt[1] << " " << pt[2] << endl;
  this->LineWidget->GetPoint2(pt);
  *file << "  [$pvTemp" << this->OutputID.ID << " GetProperty Point2] "
        << "SetElements3 " << pt[0] << " " << pt[1] << " " << pt[2] << endl;
  *file << "  [$pvTemp" << this->OutputID.ID << " GetProperty Resolution] "
        << "SetElements1 " << this->LineWidget->GetResolution() << endl;
  *file << "  $pvTemp" << this->OutputID.ID << " UpdateVTKObjects" << endl;
  *file << endl;
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LineWidget);
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Line widget: " << this->LineWidget << endl;
}
