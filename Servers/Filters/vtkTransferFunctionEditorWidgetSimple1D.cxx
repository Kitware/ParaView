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
#include "vtkColorTransferFunction.h"
#include "vtkHandleRepresentation.h"
#include "vtkHandleWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTransferFunctionEditorRepresentationSimple1D.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

#include <vtkstd/list>

vtkCxxRevisionMacro(vtkTransferFunctionEditorWidgetSimple1D, "1.7");
vtkStandardNewMacro(vtkTransferFunctionEditorWidgetSimple1D);

// The vtkNodeList is a PIMPLed list<T>.
class vtkNodeList : public vtkstd::list<vtkHandleWidget*> {};
typedef vtkstd::list<vtkHandleWidget*>::iterator vtkNodeListIterator;

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidgetSimple1D::vtkTransferFunctionEditorWidgetSimple1D()
{
  this->Nodes = new vtkNodeList;
  this->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::Start;
  this->InitialMinimumColor[0] = this->InitialMinimumColor[1] = 0;
  this->InitialMinimumColor[2] = 1;
  this->InitialMaximumColor[0] = 1;
  this->InitialMaximumColor[1] = this->InitialMaximumColor[2] = 0;

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
    self->AddNewNode(x, y);
    }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::AddNewNode(int x, int y)
{
  double e[3]; e[2]=0.0;
  e[0] = static_cast<double>(x);
  e[1] = static_cast<double>(y);
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);
  if (this->ModificationType == COLOR)
    {
    int size[2];
    rep->GetDisplaySize(size);
    e[1] = size[1] / 2;
    }
  unsigned int currentHandleNumber = rep->CreateHandle(e);
  vtkHandleWidget *currentHandle = this->CreateHandleWidget(
    this, rep, currentHandleNumber);
  if (this->ModificationType != COLOR)
    {
    this->AddOpacityPoint(x, y);
    }
  if (this->ModificationType != OPACITY)
    {
    this->AddColorPoint(x);
    }

  rep->SetHandleDisplayPosition(currentHandleNumber,e);
  currentHandle->SetEnabled(1);
  this->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(currentHandleNumber));
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
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
  if (self->ModificationType == COLOR)
    {
    int size[2];
    rep->GetDisplaySize(size);
    pos[1] = size[1]/2;
    }
  rep->SetHandleDisplayPosition(nodeId, pos);

  if (self->ModificationType != COLOR)
    {
    self->RemoveOpacityPoint(nodeId);
    self->AddOpacityPoint(x, y);
    }

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

  if (this->ModificationType != COLOR)
    {
    this->RemoveOpacityPoint(id);
    }
  if (this->ModificationType != OPACITY)
    {
    this->RemoveColorPoint(id);
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
  unsigned int i = 0;
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
void vtkTransferFunctionEditorWidgetSimple1D::SetVisibleScalarRange(
  double min, double max)
{
  if (this->VisibleScalarRange[0] == min && this->VisibleScalarRange[1] == max)
    {
    return;
    }

  double oldRange[2];
  this->GetVisibleScalarRange(oldRange);

  this->Superclass::SetVisibleScalarRange(min, max);

  this->RecomputeNodePositions(oldRange, this->VisibleScalarRange);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetWholeScalarRange(double min,
                                                                  double max)
{
  this->Superclass::SetWholeScalarRange(min, max);
  if (this->ColorFunction->GetSize())
    {
    return;
    }

  this->ColorFunction->AddRGBPoint(min, this->InitialMinimumColor[0],
                                   this->InitialMinimumColor[1],
                                   this->InitialMinimumColor[2]);
  this->ColorFunction->AddRGBPoint(max, this->InitialMaximumColor[0],
                                   this->InitialMaximumColor[1],
                                   this->InitialMaximumColor[2]);
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
void vtkTransferFunctionEditorWidgetSimple1D::AddOpacityPoint(int x, int y)
{
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!rep)
    {
    return;
    }

  int windowSize[2];
  double percent[2], newScalar;

  rep->GetDisplaySize(windowSize);
  percent[0] = x / (double)(windowSize[0]);
  percent[1] = y / (double)(windowSize[1]);
  newScalar = this->VisibleScalarRange[0] +
    percent[0] * (this->VisibleScalarRange[1]-this->VisibleScalarRange[0]);

  this->OpacityFunction->AddPoint(newScalar, percent[1]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::AddColorPoint(int x)
{
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!rep)
    {
    return;
    }

  int windowSize[2];
  double percent, newScalar;

  rep->GetDisplaySize(windowSize);
  percent = x / (double)(windowSize[0]);
  newScalar = this->VisibleScalarRange[0] +
    percent * (this->VisibleScalarRange[1]-this->VisibleScalarRange[0]);

  double color[3];
  this->ColorFunction->GetColor(newScalar, color);
  this->ColorFunction->AddRGBPoint(newScalar, color[0], color[1], color[2]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RemoveOpacityPoint(
  unsigned int id)
{
  double value[4];
  this->OpacityFunction->GetNodeValue(id, value);
  this->OpacityFunction->RemovePoint(value[0]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RemoveColorPoint(
  unsigned int id)
{
  double value[6];
  this->ColorFunction->GetNodeValue(id, value);
  this->ColorFunction->RemovePoint(value[0]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetElementOpacity(
  unsigned int idx, double opacity)
{
  if (idx >= static_cast<unsigned int>(this->OpacityFunction->GetSize()))
    {
    return;
    }

  double value[4];
  this->OpacityFunction->GetNodeValue(idx, value);
  this->RemoveOpacityPoint(idx);

  this->OpacityFunction->AddPoint(value[0], opacity);

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      this->WidgetRep);
  if (!rep)
    {
    return;
    }

  double pos[3];
  rep->GetHandleDisplayPosition(idx, pos);
  int size[2];
  rep->GetDisplaySize(size);
  pos[1] = opacity * size[1];
  rep->SetHandleDisplayPosition(idx, pos);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetElementRGBColor(
  unsigned int idx, double r, double g, double b)
{
  if (idx >= static_cast<unsigned int>(this->ColorFunction->GetSize()))
    {
    return;
    }

  double value[6];
  this->ColorFunction->GetNodeValue(idx, value);
  this->ColorFunction->RemovePoint(value[0]);
  this->ColorFunction->AddRGBPoint(value[0], r, g, b);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetElementHSVColor(
  unsigned int idx, double h, double s, double v)
{
  if (idx >= static_cast<unsigned int>(this->ColorFunction->GetSize()))
    {
    return;
    }

  double value[6];
  this->ColorFunction->GetNodeValue(idx, value);
  this->ColorFunction->RemovePoint(value[0]);
  this->ColorFunction->AddHSVPoint(value[0], h, s, v);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetElementScalar(
  unsigned int idx, double value)
{
  unsigned int size = this->Nodes->size();
  if (idx >= this->Nodes->size())
    {
    return;
    }

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);
  if (!rep)
    {
    return;
    }

  int allowSet = 0;
  double prevScalar, nextScalar, displayPos[3], pct;
  displayPos[2] = 0;
  int displaySize[2];
  prevScalar = nextScalar = 0; // initialize to avoid warnings
  if (this->ModificationType != OPACITY)
    {
    allowSet = 0;
    if (idx == 0 && size == 1)
      {
      allowSet = 1;
      }
    else
      {
      double colorNode[6];
      if (idx < size-1)
        {
        this->ColorFunction->GetNodeValue(idx+1, colorNode);
        nextScalar = colorNode[0];
        }
      if (idx > 0)
        {
        this->ColorFunction->GetNodeValue(idx-1, colorNode);
        prevScalar = colorNode[0];
        }
      if (idx == 0 && nextScalar > value)
        {
        allowSet = 1;
        }
      else if (idx == size-1 && prevScalar < value)
        {
        allowSet = 1;
        }
      else if (prevScalar < value && value < nextScalar)
        {
        allowSet = 1;
        }
      }
    if (allowSet)
      {
      this->RemoveColorPoint(idx);
      rep->GetDisplaySize(displaySize);
      pct = (value - this->VisibleScalarRange[0]) /
        (this->VisibleScalarRange[1] - this->VisibleScalarRange[0]);
      int xPos = static_cast<int>(displaySize[0] * pct);
      this->AddColorPoint(xPos);
      displayPos[0] = xPos;
      displayPos[1] = 0;
      rep->SetHandleDisplayPosition(idx, displayPos);
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      }
    }
  if (this->ModificationType != COLOR)
    {
    double opacityNode[4];
    allowSet = 0;
    if (idx == 0 && size == 1)
      {
      allowSet = 1;
      }
    else
      {
      if (idx < size-1)
        {
        this->OpacityFunction->GetNodeValue(idx+1, opacityNode);
        nextScalar = opacityNode[0];
        }
      if (idx > 0)
        {
        this->OpacityFunction->GetNodeValue(idx-1, opacityNode);
        prevScalar = opacityNode[0];
        }
      if (idx == 0 && nextScalar > value)
        {
        allowSet = 1;
        }
      else if (idx == size-1 && prevScalar < value)
        {
        allowSet = 1;
        }
      else if (prevScalar < value && value < nextScalar)
        {
        allowSet = 1;
        }
      }
    if (allowSet)
      {
      this->OpacityFunction->GetNodeValue(idx, opacityNode);
      this->RemoveOpacityPoint(idx);
      rep->GetDisplaySize(displaySize);
      pct = (value - this->VisibleScalarRange[0]) /
        (this->VisibleScalarRange[1] - this->VisibleScalarRange[0]);
      int xPos = static_cast<int>(displaySize[0] * pct);
      int yPos = static_cast<int>(displaySize[1] * opacityNode[1]);
      this->AddOpacityPoint(xPos, yPos);
      displayPos[0] = xPos;
      displayPos[1] = yPos;
      rep->SetHandleDisplayPosition(idx, displayPos);
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      }
    }

  if (!allowSet)
    {
    vtkErrorMacro("Cannot move a transfer function node horizontally past the ones on either side of it.");
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetColorSpace(int space)
{
  if (space <= 0 || space > 2)
    {
    return;
    }

  switch (space)
    {
    case 0:
      this->ColorFunction->SetColorSpace(space);
      break;
    case 1:
      this->ColorFunction->SetColorSpace(space);
      this->ColorFunction->HSVWrapOff();
      break;
    case 2:
      this->ColorFunction->SetColorSpace(VTK_CTF_HSV);
      this->ColorFunction->HSVWrapOn();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::PrintSelf(ostream& os,
                                                        vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
