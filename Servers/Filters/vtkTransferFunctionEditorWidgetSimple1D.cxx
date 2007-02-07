/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorWidgetSimple1D.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorWidgetSimple1D.h"

#include "vtkCallbackCommand.h"
#include "vtkHandleRepresentation.h"
#include "vtkHandleWidget.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTransferFunctionEditorRepresentationSimple1D.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

#include <vtkstd/list>

vtkCxxRevisionMacro(vtkTransferFunctionEditorWidgetSimple1D, "1.2");
vtkStandardNewMacro(vtkTransferFunctionEditorWidgetSimple1D);

// The vtkNodeList is a PIMPLed list<T>.
class vtkNodeList : public vtkstd::list<vtkHandleWidget*> {};
typedef vtkstd::list<vtkHandleWidget*>::iterator vtkNodeListIterator;

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidgetSimple1D::vtkTransferFunctionEditorWidgetSimple1D()
{
  this->Nodes = new vtkNodeList;
  this->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::Start;

  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::LeftButtonPressEvent,
    vtkWidgetEvent::AddPoint,
    this,
    vtkTransferFunctionEditorWidgetSimple1D::AddNodeAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect,
    this,
    vtkTransferFunctionEditorWidgetSimple1D::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent,
    vtkWidgetEvent::Move,
    this,
    vtkTransferFunctionEditorWidgetSimple1D::MoveNodeAction);
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidgetSimple1D::~vtkTransferFunctionEditorWidgetSimple1D()
{
  vtkNodeListIterator niter;
  for (niter = this->Nodes->begin(); niter != this->Nodes->end(); niter++)
    {
    (*niter)->Delete();
    }
  delete this->Nodes;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);

  if ( ! enabling )
    {
    this->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::Start;
    vtkNodeListIterator niter;
    for (niter = this->Nodes->begin(); niter != this->Nodes->end(); niter++ )
      {
      (*niter)->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
    {
    this->WidgetRep = vtkTransferFunctionEditorRepresentationSimple1D::New();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::AddNodeAction(
  vtkAbstractWidget *widget)
{
  vtkTransferFunctionEditorWidgetSimple1D *self =
    reinterpret_cast<vtkTransferFunctionEditorWidgetSimple1D*>(widget);

  if (self->WidgetState ==
      vtkTransferFunctionEditorWidgetSimple1D::MovingNode ||
      !self->WidgetRep)
    {
    return;
    }

  int x = self->Interactor->GetEventPosition()[0];
  int y = self->Interactor->GetEventPosition()[1];

  int state = self->WidgetRep->ComputeInteractionState(x, y);
  if (state == vtkTransferFunctionEditorRepresentationSimple1D::NearNode)
    {
    // move an existing node
    self->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::MovingNode;
    self->InvokeEvent(vtkCommand::LeftButtonPressEvent, NULL);
    self->Superclass::StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
    }
  else
    {
    // add a new node
    self->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::PlacingNode;
    double e[3]; e[2]=0.0;
    e[0] = static_cast<double>(x);
    e[1] = static_cast<double>(y);
    vtkTransferFunctionEditorRepresentationSimple1D *rep =
      reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
      (self->WidgetRep);
    unsigned int currentHandleNumber = rep->CreateHandle(e);
    vtkHandleWidget *currentHandle = self->CreateHandleWidget(
      self,rep,currentHandleNumber);
    rep->SetHandleDisplayPosition(currentHandleNumber,e);
    currentHandle->SetEnabled(1);
    self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(currentHandleNumber));
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::EndSelectAction(
  vtkAbstractWidget *widget)
{
  vtkTransferFunctionEditorWidgetSimple1D *self =
    reinterpret_cast<vtkTransferFunctionEditorWidgetSimple1D*>(widget);

  if (self->WidgetState == vtkTransferFunctionEditorWidgetSimple1D::MovingNode)
    {
    self->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::Start;
    self->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
    self->EventCallbackCommand->SetAbortFlag(1);
    self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    self->Superclass::EndInteraction();
    self->Render();
    }
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::MoveNodeAction(
  vtkAbstractWidget *widget)
{
  vtkTransferFunctionEditorWidgetSimple1D *self =
    reinterpret_cast<vtkTransferFunctionEditorWidgetSimple1D*>(widget);

  if (self->WidgetState == vtkTransferFunctionEditorWidgetSimple1D::Start ||
      self->WidgetState == vtkTransferFunctionEditorWidgetSimple1D::PlacingNode)
    {
    return;
    }

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (self->WidgetRep);
  if (!rep)
    {
    return;
    }

  int x = self->Interactor->GetEventPosition()[0];
  int y = self->Interactor->GetEventPosition()[1];
  unsigned int nodeId = rep->GetActiveHandle();
  double pos[3];
  pos[0] = static_cast<double>(x);
  pos[1] = static_cast<double>(y);
  pos[2] = 0.0;
  rep->SetHandleDisplayPosition(nodeId, pos);

  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent, NULL);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::OnChar()
{
  if (!this->Interactor || !this->WidgetRep)
    {
    return;
    }

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    static_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  unsigned char keyCode = this->Interactor->GetKeyCode();
  if (keyCode == 'D' || keyCode == 'd')
    {
    this->RemoveNode(rep->GetActiveHandle());
    }
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RemoveNode(unsigned int id)
{
  if (id > this->Nodes->size()-1)
    {
    return;
    }

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    static_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  vtkNodeListIterator iter;
  unsigned int i = 0;
  for (iter = this->Nodes->begin(); iter != this->Nodes->end(); iter++, i++)
    {
    if (i == id)
      {
      (*iter)->SetEnabled(0);
      (*iter)->RemoveAllObservers();
      (*iter)->Delete();
      this->Nodes->erase(iter);
      rep->RemoveHandle(id);
      return;
      }
    }
}

//-------------------------------------------------------------------------
vtkHandleWidget* vtkTransferFunctionEditorWidgetSimple1D::CreateHandleWidget(
  vtkTransferFunctionEditorWidgetSimple1D *self, 
  vtkTransferFunctionEditorRepresentationSimple1D *rep,
  unsigned int currentHandleNumber)
{
  vtkHandleRepresentation *handleRep =
    rep->GetHandleRepresentation(currentHandleNumber);
  if (!handleRep)
    {
    return NULL;
    }

  // Create the handle widget.
  vtkHandleWidget *widget = vtkHandleWidget::New();

  // Configure the handle widget
  widget->SetParent(self);
  widget->SetInteractor(self->Interactor);

  handleRep->SetRenderer(self->CurrentRenderer);
  widget->SetRepresentation(handleRep);

  // Now place the widget into the list of handle widgets.
  vtkNodeListIterator niter;
  int i = 0;
  for (niter = self->Nodes->begin(); niter != self->Nodes->end();
       niter++, i++)
    {
    if (i == currentHandleNumber)
      {
      self->Nodes->insert(niter, widget);
      return widget;
      }
    }

  if (currentHandleNumber == self->Nodes->size())
    {
    self->Nodes->insert(self->Nodes->end(), widget);
    return widget;
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetScalarRange(
  double min, double max)
{
  if (this->ScalarRange[0] == min && this->ScalarRange[1] == max)
    {
    return;
    }

  double oldRange[2];
  this->GetScalarRange(oldRange);

  this->Superclass::SetScalarRange(min, max);

  this->RecomputeNodePositions(oldRange, this->ScalarRange);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RecomputeNodePositions(
  double oldRange[2], double newRange[2])
{
  // recompute transfer function node positions based on a change
  // in scalar range
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!rep)
    {
    return;
    }

  int displaySize[2];
  rep->GetDisplaySize(displaySize);

  double newDisplayMin =
    (oldRange[0] - newRange[0]) / (newRange[1] - newRange[0]) * displaySize[0];
  double newDisplayMax =
    (oldRange[1] - newRange[0]) / (newRange[1] - newRange[0]) * displaySize[0];
  double newWidth = newDisplayMax - newDisplayMin;

  unsigned int i;
  vtkHandleRepresentation *handle;
  double oldPos[3], newPos[3], displayPct;

  for (i = 0; i < this->Nodes->size(); i++)
    {
    handle = rep->GetHandleRepresentation(i);
    handle->GetDisplayPosition(oldPos);
    displayPct = oldPos[0] / (double)(displaySize[0]);
    newPos[0] = (displayPct * newWidth) + newDisplayMin;
    newPos[1] = oldPos[1];
    newPos[2] = oldPos[2];
    handle->SetDisplayPosition(newPos);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RecomputeNodePositions(
  int oldSize[2], int newSize[2])
{
  // recompute transfer function node positions based on a change
  // in renderer size
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!rep)
    {
    return;
    }

  unsigned int i;
  vtkHandleRepresentation *handle;
  double oldPos[3], newPos[3], displayPctX, displayPctY;

  for (i = 0; i < this->Nodes->size(); i++)
    {
    handle = rep->GetHandleRepresentation(i);
    handle->GetDisplayPosition(oldPos);
    displayPctX = oldPos[0] / (double)(oldSize[0]);
    displayPctY = oldPos[1] / (double)(oldSize[1]);
    newPos[0] = displayPctX * newSize[0];
    newPos[1] = displayPctY * newSize[1];
    newPos[2] = oldPos[2];
    handle->SetDisplayPosition(newPos);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::Configure(int size[2])
{
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      this->WidgetRep);
  if (!rep)
    {
    return;
    }

  int oldSize[2];
  rep->GetDisplaySize(oldSize);

  this->Superclass::Configure(size);

  this->RecomputeNodePositions(oldSize, size);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::PrintSelf(ostream& os,
                                                        vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
