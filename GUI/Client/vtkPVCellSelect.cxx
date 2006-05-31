/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCellSelect.cxx

  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCellSelect.h"

#include "vtkObjectFactory.h"
#include "vtkKWPushButton.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkCallbackCommand.h"
#include "vtkPVApplication.h"
#include "vtkCommand.h"
#include "vtkPVWindow.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkPVTraceHelper.h"
#include <vtksys/ios/sstream>
#include <stdio.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCellSelect);
vtkCxxRevisionMacro(vtkPVCellSelect, "1.1");

//----------------------------------------------------------------------------
vtkPVCellSelect::vtkPVCellSelect()
{
  this->SelectReady = 0;
  this->InPickState = 0;
  this->Xs = 0;
  this->Ys = 0;
  this->Xe = 0;
  this->Ye = 0;

  this->SelectButton = vtkKWPushButton::New();
  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this); 
  this->EventCallbackCommand->SetCallback(vtkPVCellSelect::ProcessEvents);
  this->RubberBand = vtkInteractorStyleRubberBandPick::New();
  this->SavedStyle = NULL;

  for (int i = 0; i < 8; i++)
    {
    this->Verts[i*4+0] = 0.0;
    this->Verts[i*4+1] = 0.0;
    this->Verts[i*4+2] = 0.0;
    this->Verts[i*4+3] = 0.0;
    }
  memcpy((void*)this->SavedVerts, (void*)this->Verts, 32*sizeof(double));  

}

//----------------------------------------------------------------------------
vtkPVCellSelect::~vtkPVCellSelect()
{ 
  if (this->SavedStyle)
    {
    this->SavedStyle->Delete();
    }
  this->RubberBand->Delete();
  this->EventCallbackCommand->Delete();
  this->SelectButton->Delete();
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::CreateProperties()
{
  this->Superclass::CreateProperties();

  //add the Select Button
  this->SelectButton->SetParent(this->ParameterFrame->GetFrame());
  this->SelectButton->Create();
  this->SelectButton->SetCommand(this, "SelectCallback");
  this->SelectButton->SetText("Start Selection");
  this->SelectButton->EnabledOff();

  this->Script("pack %s -fill x -expand true",
               this->SelectButton->GetWidgetName());

  this->UpdateProperties();
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::SelectCallback()
{
  vtkPVWindow *window = this->GetPVApplication()->GetMainWindow();
  vtkPVGenericRenderWindowInteractor *rwi = window->GetInteractor();

  if (!this->InPickState)
    {    
    this->GetPVInput(0)->SetVisibility(1);

    this->SelectButton->SetReliefToSunken();
    //start watching left mouse actions to get a begin and end pixel
    rwi->AddObserver(vtkCommand::LeftButtonPressEvent, 
                     this->EventCallbackCommand);
    rwi->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                     this->EventCallbackCommand);
    //temporarily use the rubber band interactor style just to draw a rectangle
    this->SavedStyle = rwi->GetInteractorStyle();
    this->SavedStyle->Register(this);
    rwi->SetInteractorStyle(this->RubberBand);

    //don't allow camera motion while selecting
    this->RubberBand->StartSelect();
    this->SelectReady = 0;
    }
  else
    {
    this->SelectReady = 1;
    this->RubberBand->HighlightProp(NULL);
    rwi->SetInteractorStyle(this->SavedStyle);
    this->SavedStyle->UnRegister(this);
    this->SavedStyle = NULL;
    rwi->RemoveObserver(this->EventCallbackCommand);
    this->SelectButton->SetReliefToRaised();
    this->SetAcceptButtonColorToModified();
    }

  this->InPickState = !this->InPickState;
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                  unsigned long event,
                                  void* clientdata, 
                                  void* vtkNotUsed(calldata))
{
  vtkPVCellSelect* self 
    = reinterpret_cast<vtkPVCellSelect *>( clientdata );

  vtkPVWindow* window = self->GetPVApplication()->GetMainWindow();
  vtkPVGenericRenderWindowInteractor *rwi = window->GetInteractor();
  int *eventpos = rwi->GetEventPosition();
  vtkRenderer *renderer = rwi->FindPokedRenderer(eventpos[0], eventpos[1]);

  switch(event)
    {
    case vtkCommand::LeftButtonPressEvent: 
      self->OnLeftButtonDown(eventpos[0], eventpos[1]);
      break;
    case vtkCommand::LeftButtonReleaseEvent:      
      self->OnLeftButtonUp(eventpos[0], eventpos[1], renderer);
      break;
    }
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::OnLeftButtonDown(int x, int y)
{
  this->Xs = x;
  this->Ys = y;
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::OnLeftButtonUp(int x, int y, vtkRenderer *renderer)
{
  this->Xe = x;
  this->Ye = y;
  
  double x0 = (double)((this->Xs < this->Xe) ? this->Xs : this->Xe);
  double y0 = (double)((this->Ys < this->Ye) ? this->Ys : this->Ye);
  double x1 = (double)((this->Xs > this->Xe) ? this->Xs : this->Xe);
  double y1 = (double)((this->Ys > this->Ye) ? this->Ys : this->Ye);
  
  if (x0 == x1)
    {
    x0 -= 0.5;
    x1 += 0.5;
    }
  if (y0 == y1)
    {
    y0 -= 0.5;
    y1 += 0.5;
    }
  
  this->Xs = (int)x0;
  this->Xe = (int)x1;
  this->Ys = (int)y0;
  this->Ye = (int)y1;

  renderer->SetDisplayPoint(x0, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[0]);
  
  renderer->SetDisplayPoint(x0, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[4]);
  
  renderer->SetDisplayPoint(x0, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[8]);
  
  renderer->SetDisplayPoint(x0, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[12]);
  
  renderer->SetDisplayPoint(x1, y0, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[16]);
  
  renderer->SetDisplayPoint(x1, y0, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[20]);
  
  renderer->SetDisplayPoint(x1, y1, 0);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[24]);
  
  renderer->SetDisplayPoint(x1, y1, 1);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(&this->Verts[28]);    

  //reset GUI
  this->SelectCallback();

  //move the result to the servers to draw wire frame selection area
  this->SetVerts(1);
}
 
//----------------------------------------------------------------------------
void vtkPVCellSelect::DoSelect()
{
  vtkSMSourceProxy *sp = this->GetProxy();
  if (!sp)
    {
    vtkErrorMacro("Failed to find proxy.");
    return;
    }

  vtkSMIntVectorProperty* sb = vtkSMIntVectorProperty::SafeDownCast(
    sp->GetProperty("ShowBounds"));
  if (!sb)
    {
    vtkErrorMacro("Failed to find property.");
    }
  else 
    {
    sb->SetElement(0, 0);
    sp->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::Reset()
{
  this->Superclass::Reset();
  if (this->SelectReady)
    {
    //restore last saved frustum
    memcpy((void*)this->Verts, (void*)this->SavedVerts, 32*sizeof(double));  
    this->SetVerts(0);
    this->GetPVInput(0)->SetVisibility(0);
    }
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::AcceptCallbackInternal()
{
  if (this->SelectReady)
    {
    this->DoSelect();
    this->SelectReady = 0;
    this->GetPVInput(0)->SetVisibility(0);
    this->AdditionalTraceSave();
    }
  
  //save this frustum for next reset
  memcpy((void*)this->SavedVerts, (void*)this->Verts, 32*sizeof(double));

  this->SelectButton->EnabledOn();
  this->Superclass::AcceptCallbackInternal();
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::SetVerts(int wireframe)
{
  vtkSMSourceProxy *sp = this->GetProxy();
  if (!sp)
    {
    vtkErrorMacro("Failed to find proxy.");
    return;
    }

  vtkSMDoubleVectorProperty* cf = vtkSMDoubleVectorProperty::SafeDownCast(
  sp->GetProperty("CreateFrustum"));
  vtkSMIntVectorProperty* sb = vtkSMIntVectorProperty::SafeDownCast(
    sp->GetProperty("ShowBounds"));
  if (!cf || !sb)
    {
    vtkErrorMacro("Failed to find properties.");
    }
  else 
    {
    sb->SetElement(0, wireframe);
    cf->SetElements(&this->Verts[0]);
    sp->UpdateVTKObjects();   
    }
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::AdditionalTraceSave()
{
  ofstream *f = this->GetTraceHelper()->GetFile();
  this->AdditionalStateSave(f);
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::AdditionalStateSave(ofstream *file)
{

  for (int i = 0; i < 8; i++)
    {
    *file << "$kw(" << this->GetTclName() << ")" << " CreateVert " << i << " " 
          << this->Verts[i*4+0] << " " 
          << this->Verts[i*4+1] << " " 
          << this->Verts[i*4+2] << " " 
          << this->Verts[i*4+3] << endl;
    }
                                       
  *file << "$kw(" << this->GetTclName() << ")" << " SetVerts 0" << endl;

  *file << endl;
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::CreateVert(int i,
                                 double v0 , double v1 , double v2 , double v3)
{
  this->Verts[i*4+0] = v0;
  this->Verts[i*4+1] = v1;
  this->Verts[i*4+2] = v2;
  this->Verts[i*4+3] = v3;
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::AdditionalBatchSave(ofstream *file)
{
  *file << "  [$pvTemp" << this->Proxy->GetSelfIDAsString()
        << " GetProperty CreateFrustum]" 
        << " SetArgumentIsArray 1" << endl;
  for (int i = 0; i < 32; i++)
    {
    *file << "  [$pvTemp" << this->Proxy->GetSelfIDAsString()
          << " GetProperty CreateFrustum]" 
          << " SetElement " << i << " " << this->Verts[i] << endl;
    }
}
