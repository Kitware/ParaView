/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPropAndCellSelect.cxx

  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPropAndCellSelect.h"

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
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkPVClientServerIdCollectionInformation.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkPVTraceHelper.h"
#include <vtksys/ios/sstream>
#include <stdio.h>
#include "vtkSMDoubleVectorProperty.h"

#include "vtkPVSelectionList.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPropAndCellSelect);
vtkCxxRevisionMacro(vtkPVPropAndCellSelect, "1.2");

//----------------------------------------------------------------------------
vtkPVPropAndCellSelect::vtkPVPropAndCellSelect()
{
  this->SelectReady = 0;
  this->InPickState = 0;
  this->Xs = 0;
  this->Ys = 0;
  this->Xe = 0;
  this->Ye = 0;
  for (int i = 0; i < 8; i++)
    {
    this->Verts[i*4+0] = 0.0;
    this->Verts[i*4+1] = 0.0;
    this->Verts[i*4+2] = 0.0;
    this->Verts[i*4+3] = 0.0;
    }
  memcpy((void*)this->SavedVerts, (void*)this->Verts, 32*sizeof(double));  

  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this); 
  this->EventCallbackCommand->SetCallback(vtkPVPropAndCellSelect::ProcessEvents);
  this->RubberBand = vtkInteractorStyleRubberBandPick::New();
  this->SavedStyle = NULL;

  this->SelectButton = vtkKWPushButton::New();
  this->SelectionType = 0;
  this->LastSelectType = 0;
}

//----------------------------------------------------------------------------
vtkPVPropAndCellSelect::~vtkPVPropAndCellSelect()
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
void vtkPVPropAndCellSelect::CreateProperties()
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
void vtkPVPropAndCellSelect::SelectCallback()
{
  vtkSMDataObjectDisplayProxy *dp = this->GetDisplayProxy();
  if (dp)
    {
    vtkSMIntVectorProperty *prop = vtkSMIntVectorProperty::SafeDownCast(
      dp->GetProperty("Pickable"));
    if (prop)
      {
      prop->SetElement(0, 0); //Don't allow picking of my own output.
      this->UpdateVTKObjects();
      }
    else
      {
      vtkErrorMacro("Can't find pickable property for select props.");
      }    
    }
  else
    {
    vtkErrorMacro("Can't find display proxy for select props.");
    }    

  vtkPVWindow *window = this->GetPVApplication()->GetMainWindow();
  vtkPVGenericRenderWindowInteractor *rwi = window->GetInteractor();

  if (!this->InPickState)
    {    
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
void vtkPVPropAndCellSelect::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                  unsigned long event,
                                  void* clientdata, 
                                  void* vtkNotUsed(calldata))
{
  vtkPVPropAndCellSelect* self 
    = reinterpret_cast<vtkPVPropAndCellSelect *>( clientdata );

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
void vtkPVPropAndCellSelect::OnLeftButtonDown(int x, int y)
{
  this->Xs = x;
  this->Ys = y;
}

//----------------------------------------------------------------------------
void vtkPVPropAndCellSelect::OnLeftButtonUp(int x, int y, vtkRenderer *renderer)
{
  this->Xe = x;
  this->Ye = y;

  double x0 = (double)((this->Xs < this->Xe) ? this->Xs : this->Xe);
  double y0 = (double)((this->Ys < this->Ye) ? this->Ys : this->Ye);
  double x1 = (double)((this->Xs > this->Xe) ? this->Xs : this->Xe);
  double y1 = (double)((this->Ys > this->Ye) ? this->Ys : this->Ye);

  this->Xs = (int)x0;
  this->Ys = (int)y0;
  this->Xe = (int)x1;
  this->Ye = (int)y1;
  
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

  vtkSMDataObjectDisplayProxy *dop = this->GetDisplayProxy();
  dop->SetRepresentationCM(vtkSMDataObjectDisplayProxy::SURFACE);
  dop->UpdateVTKObjects();
}
 
//----------------------------------------------------------------------------
void vtkPVPropAndCellSelect::DoSelect()
{
  vtkPVApplication *application = NULL;
  vtkSMRenderModuleProxy *renderModuleProxy = NULL;
  vtkPVClientServerIdCollectionInformation *idInfo = NULL;
  vtkSMSourceProxy *sp = NULL;
  vtkSMProperty *initMethod = NULL;
  vtkSMProxyProperty *addDataSetMethod = NULL;
  int numProps;

  //find the proxy for the renderer
  application = this->GetPVApplication();
  if (!application)
    {
    vtkErrorMacro("Failed to find application.");
    return;
    }

  renderModuleProxy = application->GetRenderModuleProxy();    
  if (!renderModuleProxy)
    {
    vtkErrorMacro("Failed to find render module proxy.");
    return;
    }

  sp = this->GetProxy();
  if (!sp)
    {
    vtkErrorMacro("Failed to find proxy.");
    return;
    }

  initMethod = vtkSMProperty::SafeDownCast(
    sp->GetProperty("Initialize"));
  if (!initMethod)
    {
    vtkErrorMacro("Failed to find Init method.");
    return;
    }

  addDataSetMethod = vtkSMProxyProperty::SafeDownCast(
    sp->GetProperty("AddDataSet"));
  if (!addDataSetMethod)
    {
    vtkErrorMacro("Failed to find AddDataSet method.");
    return;
    }

  vtkPVSelectionList *stw = vtkPVSelectionList::SafeDownCast(this->GetPVWidget("SelectionType"));
  if (stw)
    {
    int seltype = stw->GetCurrentValue();
    if (seltype == 2)
      {
      this->SelectionType = 0; //volume
      }
    else
      {
      this->SelectionType = 1; //surface
      }
    }
  this->LastSelectType = this->SelectionType;

  //got everything we need, now do the pick ----------------------------
  idInfo = renderModuleProxy->Pick(this->Xs,this->Ys,this->Xe,this->Ye);

  //now transfer the results over to the server
  initMethod->Modified();
  sp->UpdateVTKObjects();

  numProps = idInfo->GetLength();
  for (int i = 0; i < numProps; i++)
    {
    vtkClientServerID ID = idInfo->GetID(i);
    vtkSMProxy *objProxy = 
      renderModuleProxy->GetProxyFromPropID(&ID, this->SelectionType);

    if (!objProxy)
      {
      continue;
      }

    addDataSetMethod->RemoveAllProxies();
    addDataSetMethod->AddProxy(objProxy);
    sp->UpdateVTKObjects();    
    }

  idInfo->Delete();

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
void vtkPVPropAndCellSelect::Reset()
{
  this->Superclass::Reset();
  if (this->SelectReady)
    {
    //restore last saved frustum
    memcpy((void*)this->Verts, (void*)this->SavedVerts, 32*sizeof(double));  
    this->SetVerts(0);
    }

  this->UpdateUI();
}

//----------------------------------------------------------------------------
void vtkPVPropAndCellSelect::UpdateUI()
{
  vtkPVSelectionList *stw = vtkPVSelectionList::SafeDownCast(this->GetPVWidget("SelectionType"));
  vtkPVSelectionList *etw = vtkPVSelectionList::SafeDownCast(this->GetPVWidget("ExactTest"));     
  vtkPVSelectionList *ptw = vtkPVSelectionList::SafeDownCast(this->GetPVWidget("PassThrough"));     

  if (stw && etw && ptw)
    {
    int seltype = stw->GetCurrentValue();
    if (seltype == 0)
      {
      etw->EnabledOff();
      ptw->EnabledOff();
      }    
    else
      {
      etw->EnabledOn();
      ptw->EnabledOn();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVPropAndCellSelect::AcceptCallbackInternal()
{
  vtkPVSelectionList *stw = vtkPVSelectionList::SafeDownCast(this->GetPVWidget("SelectionType"));     
  int stn = stw->GetCurrentValue();

  if (this->SelectReady || (stn != this->LastSelectType))
    {
    this->DoSelect();
    this->SelectReady = 0;
    this->AdditionalTraceSave();
    this->LastSelectType = stn;
    }

  //save this frustum for next reset
  memcpy((void*)this->SavedVerts, (void*)this->Verts, 32*sizeof(double));
  
  this->SelectButton->EnabledOn();
  this->Superclass::AcceptCallbackInternal();

  this->UpdateUI();
}

//----------------------------------------------------------------------------
void vtkPVPropAndCellSelect::SetVerts(int wireframe)
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
void vtkPVPropAndCellSelect::AdditionalTraceSave()
{
  ofstream *f = this->GetTraceHelper()->GetFile();
  this->AdditionalStateSave(f);
}

//----------------------------------------------------------------------------
void vtkPVPropAndCellSelect::AdditionalStateSave(ofstream *file)
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
void vtkPVPropAndCellSelect::CreateVert(int i,
                                 double v0 , double v1 , double v2 , double v3)
{
  this->Verts[i*4+0] = v0;
  this->Verts[i*4+1] = v1;
  this->Verts[i*4+2] = v2;
  this->Verts[i*4+3] = v3;
}

//----------------------------------------------------------------------------
void vtkPVPropAndCellSelect::AdditionalBatchSave(ofstream *file)
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
