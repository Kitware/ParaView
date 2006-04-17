/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAreaSelect.cxx

  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAreaSelect.h"

#include "vtkObjectFactory.h"
#include "vtkKWPushButton.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkPVWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkAbstractPicker.h"
#include "vtkAreaPicker.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkPlanes.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkPoints.h"
#include "vtkPVSourceNotebook.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVAreaSelect);
vtkCxxRevisionMacro(vtkPVAreaSelect, "1.2");

//----------------------------------------------------------------------------
vtkPVAreaSelect::vtkPVAreaSelect()
{
  this->InPickState = 0;

  this->SelectButton = vtkKWPushButton::New();
  this->EventCallbackCommand = vtkCallbackCommand::New();
  this->EventCallbackCommand->SetClientData(this); 
  this->EventCallbackCommand->SetCallback(vtkPVAreaSelect::ProcessEvents);
  this->RubberBand = vtkInteractorStyleRubberBandPick::New();
}

//----------------------------------------------------------------------------
vtkPVAreaSelect::~vtkPVAreaSelect()
{ 
  this->RubberBand->Delete();
  this->EventCallbackCommand->Delete();
  this->SelectButton->Delete();
}

//----------------------------------------------------------------------------
void vtkPVAreaSelect::CreateProperties()
{
  this->Superclass::CreateProperties();

  //add the Select Button
  this->SelectButton->SetParent(this->ParameterFrame->GetFrame());
  this->SelectButton->Create();
  this->SelectButton->SetCommand(this, "SelectCallback");
  this->SelectButton->SetText("Start Selection");
  this->Script("pack %s -fill x -expand true",
               this->SelectButton->GetWidgetName());

  this->UpdateProperties();
}

//----------------------------------------------------------------------------
void vtkPVAreaSelect::SelectCallback()
{
  vtkPVWindow *window = this->GetPVApplication()->GetMainWindow();
  vtkPVGenericRenderWindowInteractor *rwi = window->GetInteractor();

  if (!this->InPickState)
    {    
    //start watching left mouse actions to get a begin and end pixel
    rwi->AddObserver(vtkCommand::LeftButtonPressEvent, this->EventCallbackCommand);
    rwi->AddObserver(vtkCommand::LeftButtonReleaseEvent, this->EventCallbackCommand);

    //temporarily use the rubber band interactor style just to draw a rectangle
    this->SavedStyle = rwi->GetInteractorStyle();
    rwi->SetInteractorStyle(this->RubberBand);
    //don't allow camera motion while selecting
    this->RubberBand->StartSelect();
    this->SelectButton->SetReliefToSunken();
    }
  else
    {
    this->RubberBand->HighlightProp(NULL);
    rwi->SetInteractorStyle(this->SavedStyle);
    this->SelectButton->SetReliefToRaised();
    rwi->RemoveObserver(this->EventCallbackCommand);
    }

  this->InPickState = !this->InPickState;

}

//----------------------------------------------------------------------------
void vtkPVAreaSelect::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                  unsigned long event,
                                  void* clientdata, 
                                  void* vtkNotUsed(calldata))
{
  vtkPVAreaSelect* self 
    = reinterpret_cast<vtkPVAreaSelect *>( clientdata );

  vtkPVWindow* window = self->GetPVApplication()->GetMainWindow();
  vtkPVGenericRenderWindowInteractor * rwi = window->GetInteractor();
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
void vtkPVAreaSelect::OnLeftButtonDown(int x, int y)
{
  this->X = x;
  this->Y = y;
}

//----------------------------------------------------------------------------
void vtkPVAreaSelect::OnLeftButtonUp(int x, int y, vtkRenderer *renderer)
{
  if (this->InPickState)
    {
    this->InvalidateDataInformation();

    double x0 = (double)((this->X < x) ? this->X : x);
    double y0 = (double)((this->Y < y) ? this->Y : y);
    double x1 = (double)((this->X > x) ? this->X : x);
    double y1 = (double)((this->Y > y) ? this->Y : y);

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

    double verts[24];

    renderer->SetDisplayPoint(x0, y0, 0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&verts[0]);
    
    renderer->SetDisplayPoint(x0, y0, 1);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&verts[3]);
    
    renderer->SetDisplayPoint(x0, y1, 0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&verts[6]);
    
    renderer->SetDisplayPoint(x0, y1, 1);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&verts[9]);
    
    renderer->SetDisplayPoint(x1, y0, 0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&verts[12]);
    
    renderer->SetDisplayPoint(x1, y0, 1);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&verts[15]);
    
    renderer->SetDisplayPoint(x1, y1, 0);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&verts[18]);
    
    renderer->SetDisplayPoint(x1, y1, 1);
    renderer->DisplayToWorld();
    renderer->GetWorldPoint(&verts[21]);    

    //move the result to the servers
    vtkSMSourceProxy *sp = this->GetProxy();
    vtkSMDoubleVectorProperty* pp = vtkSMDoubleVectorProperty::SafeDownCast(
      sp->GetProperty("CreateFrustum"));
    if (!sp)
      {
      vtkErrorMacro("Failed to find property.");
      }
    else 
      {
      pp->SetArgumentIsArray(1);
      pp->SetElements(verts);
      sp->UpdateVTKObjects();
      }

    //turn off the selection mode
    this->SelectCallback();
    }
}
