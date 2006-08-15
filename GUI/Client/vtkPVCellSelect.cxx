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
#include "vtkSMRenderModuleProxy.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include <vtksys/ios/sstream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCellSelect);
vtkCxxRevisionMacro(vtkPVCellSelect, "1.4");

//----------------------------------------------------------------------------
vtkPVCellSelect::vtkPVCellSelect()
{
  this->SelectReady = 0;
  this->InPickState = 0;
  this->Xs = 0;
  this->Ys = 0;
  this->Xe = 0;
  this->Ye = 0;

  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this); 
  this->EventCallbackCommand->SetCallback(vtkPVCellSelect::ProcessEvents);
  this->RubberBand = vtkInteractorStyleRubberBandPick::New();
  this->SavedStyle = NULL;

  this->SelectButton = vtkKWPushButton::New();
  this->SelectionType = 0;
  this->LastSelectType = 0;
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

  this->Xs = (int)x0;
  this->Ys = (int)y0;
  this->Xe = (int)x1;
  this->Ye = (int)y1;
 
  if (this->Xs < 1)
    {
    this->Xs = 1;
    }
  if (this->Ys < 1)
    {
    this->Ys = 1;
    }
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
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::AcceptCallbackInternal()
{
  this->DoSelect();
  
  this->SelectButton->EnabledOn();
  this->Superclass::AcceptCallbackInternal();
}

//----------------------------------------------------------------------------
void vtkPVCellSelect::DoSelect()
{
  vtkPVApplication *application = NULL;
  vtkSMRenderModuleProxy *renderModuleProxy = NULL;

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

  //got everything we need, now do the pick ----------------------------

  vtkSelection *selection;

  selection = renderModuleProxy->SelectVisibleCells(this->Xs, this->Ys, this->Xe, this->Ye);

  cerr << "Selection Result:" << endl;
  vtkSelectionSerializer *ser = vtkSelectionSerializer::New();
  ser->PrintXML(1, selection);
  ser->Delete();
  selection->Delete();
  cerr << endl;

}


