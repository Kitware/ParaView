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
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTransferFunctionEditorRepresentationSimple1D.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

#include <list>

vtkStandardNewMacro(vtkTransferFunctionEditorWidgetSimple1D);

// The vtkNodeList is a PIMPLed list<T>.
class vtkNodeList : public std::list<vtkHandleWidget*> {};
typedef std::list<vtkHandleWidget*>::iterator vtkNodeListIterator;

//----------------------------------------------------------------------------
vtkTransferFunctionEditorWidgetSimple1D::vtkTransferFunctionEditorWidgetSimple1D()
{
  this->Nodes = new vtkNodeList;
  this->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::Start;
  this->InitialMinimumColor[0] = this->InitialMinimumColor[1] = 0;
  this->InitialMinimumColor[2] = 1;
  this->InitialMaximumColor[0] = 1;
  this->InitialMaximumColor[1] = this->InitialMaximumColor[2] = 0;
  this->LockEndPoints = 0;
  this->LeftClickEventPosition[0] = this->LeftClickEventPosition[1] = 0;
  this->LeftClickCount = 0;
  this->BorderWidth = 8;

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
  this->RemoveAllNodes();
  delete this->Nodes;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RemoveAllNodes()
{
  vtkNodeListIterator niter;
  for (niter = this->Nodes->begin(); niter != this->Nodes->end();)
    {
    (*niter)->Delete();
    this->Nodes->erase(niter++);
    }
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
    vtkTransferFunctionEditorRepresentationSimple1D *rep =
      static_cast<vtkTransferFunctionEditorRepresentationSimple1D*>(
        this->WidgetRep);
    rep->SetColorFunction(this->ColorFunction);
    if (this->ModificationType == COLOR ||
        this->ModificationType == COLOR_AND_OPACITY)
      {
      rep->SetColorLinesByScalar(1);
      }
    else
      {
      rep->SetColorLinesByScalar(0);
      }
    this->Superclass::CreateDefaultRepresentation();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetModificationType(int type)
{
  int modified = 0;
  if (this->ModificationType != (type < 0 ? 0 : (type > 2 ? 2 : type)))
    {
    modified = 1;
    }
  this->Superclass::SetModificationType(type);
  if (modified)
    {
    vtkTransferFunctionEditorRepresentationSimple1D *rep =
      vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
        this->WidgetRep);
    if (rep)
      {
      if (this->ModificationType == COLOR ||
          this->ModificationType == COLOR_AND_OPACITY)
        {
        rep->SetColorLinesByScalar(1);
        }
      else
        {
        rep->SetColorLinesByScalar(0);
        }
      }
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

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      self->WidgetRep);
  unsigned int oldActiveHandle = rep->GetActiveHandle();
  int state = self->WidgetRep->ComputeInteractionState(x, y);
  if (state == vtkTransferFunctionEditorRepresentationSimple1D::NearNode)
    {
    // move an existing node
    self->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::MovingNode;
    self->Superclass::StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent, NULL);
    if (oldActiveHandle == rep->GetActiveHandle())
      {
      self->LeftClickCount++;
      }
    else
      {
      self->LeftClickCount = 0;
      }
    self->LeftClickEventPosition[0] = x;
    self->LeftClickEventPosition[1] = y;
    }
  else
    {
    if (self->WholeScalarRange[0] != self->WholeScalarRange[1])
      {
      // add a new node
      self->WidgetState = vtkTransferFunctionEditorWidgetSimple1D::PlacingNode;
      self->AddNewNode(x, y);
      }
    self->LeftClickCount = 0;
    self->LeftClickEventPosition[0] = x;
    self->LeftClickEventPosition[1] = y;
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
  int size[2];
  rep->GetDisplaySize(size);
  if (this->ModificationType == COLOR)
    {
    e[1] = size[1] / 2;
    }
  double scalar = this->ComputeScalar(e[0], size[0]);
  this->ClampToWholeRange(e, size, scalar);
  if (!this->AllowInteriorElements)
    {
    double pct = (scalar - this->WholeScalarRange[0]) /
      (this->WholeScalarRange[1] - this->WholeScalarRange[0]);
    if (pct < 0.5)
      {
      scalar = this->WholeScalarRange[0];
      }
    else
      {
      scalar = this->WholeScalarRange[1];
      }
    e[0] = this->ComputePositionFromScalar(scalar, size[0]);
    }
  int nodeId = this->NodeExists(scalar);
  unsigned int currentHandleNumber;
  if (nodeId >= 0)
    {
    currentHandleNumber = static_cast<unsigned int>(nodeId);
    rep->GetHandleRepresentation(currentHandleNumber)->
      SetDisplayPosition(e);
    }
  else
    {
    currentHandleNumber = rep->CreateHandle(e, scalar);
    }

  vtkHandleWidget *newHandleWidget = NULL;
  if (rep->GetNumberOfHandles() > this->Nodes->size())
    {
    newHandleWidget = this->CreateHandleWidget(
      this, rep, currentHandleNumber);
    }
  if (this->ModificationType != COLOR)
    {
    this->AddOpacityPoint(e[0], e[1]);
    }
  if (this->ModificationType != OPACITY)
    {
    this->AddColorPoint(e[0]);
    }

  rep->SetActiveHandle(currentHandleNumber);
  this->LeftClickEventPosition[0] = x;
  this->LeftClickEventPosition[1] = y;
  if (newHandleWidget)
    {
    newHandleWidget->SetEnabled(1);
    }
  rep->BuildRepresentation();
  this->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(currentHandleNumber));
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::ClampToWholeRange(
  double pos[2], int size[2], double &scalar)
{
  scalar = (scalar < this->WholeScalarRange[0] ? this->WholeScalarRange[0] :
            (scalar > this->WholeScalarRange[1] ? this->WholeScalarRange[1] :
             scalar));
  pos[0] = this->ComputePositionFromScalar(scalar, size[0]);

  pos[1] =
    (pos[1] < this->BorderWidth ? this->BorderWidth :
     (pos[1] > size[1] - this->BorderWidth ? size[1] - this->BorderWidth :
      pos[1]));
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::AddNewNode(double scalar)
{
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      this->WidgetRep);
  if (!rep)
    {
    return;
    }

  // determine display position
  double displayPos[3], opacity;
  int displaySize[2];
  rep->GetDisplaySize(displaySize);

  if (this->WholeScalarRange[0] != this->WholeScalarRange[1] ||
      this->Nodes->size() == 0)
    {
    displayPos[0] = this->ComputePositionFromScalar(scalar, displaySize[0]);
    }
  else
    {
    displayPos[0] = displaySize[0] - this->BorderWidth;
    }
  
  if (this->ModificationType != COLOR)
    {
    if (this->WholeScalarRange[0] != this->WholeScalarRange[1] ||
        this->Nodes->size() == 0)
      {
      opacity = this->OpacityFunction->GetValue(scalar);
      }
    else
      {
      double opacityNode[4];
      this->OpacityFunction->GetNodeValue(1, opacityNode);
      opacity = opacityNode[1];
      }
    displayPos[1] = (displaySize[1] - 2*this->BorderWidth) * opacity
      + this->BorderWidth;
    }
  else
    {
    displayPos[1] = displaySize[1] / 2;
    }
  displayPos[2] = 0;

  double clampedScalar = scalar;
  if (this->WholeScalarRange[0] != this->WholeScalarRange[1] ||
      this->Nodes->size() == 0)
    {
    this->ClampToWholeRange(displayPos, displaySize, clampedScalar);
    }

  int nodeId;
  if (clampedScalar != scalar)
    {
    if (this->ModificationType != COLOR)
      {
      double opacityValue[4];
      for (nodeId = 0; nodeId < this->OpacityFunction->GetSize(); nodeId++)
        {
        this->OpacityFunction->GetNodeValue(nodeId, opacityValue);
        if (opacityValue[0] == scalar)
          {
          this->OpacityFunction->RemovePoint(scalar);
          this->OpacityFunction->AddPoint(clampedScalar, opacityValue[1],
                                          opacityValue[2], opacityValue[3]);
          break;
          }
        }
      }
    if (this->ModificationType != OPACITY)
      {
      double colorValue[6];
      for (nodeId = 0; nodeId < this->ColorFunction->GetSize(); nodeId++)
        {
        this->ColorFunction->GetNodeValue(nodeId, colorValue);
        if (colorValue[0] == scalar)
          {
          this->ColorFunction->RemovePoint(scalar);
          this->ColorFunction->AddRGBPoint(
            clampedScalar, colorValue[1], colorValue[2], colorValue[3],
            colorValue[4], colorValue[5]);
          break;
          }
        }
      }
    this->UpdateTransferFunctionMTime();
    }

  double nodePos[3];
  double rgbNode[6];
  unsigned int i;
  for (i = 0; i < this->Nodes->size(); i++)
    {
    rep->GetHandleDisplayPosition(i, nodePos);
    if (nodePos[0] == displayPos[0] && nodePos[1] == displayPos[1])
      {
      // A node already exists at this position; don't add another one.
      if (this->ModificationType != OPACITY)
        {
        // color the node based on the CTF
        this->ColorFunction->GetNodeValue(i, rgbNode);
        if (rep->GetColorElementsByColorFunction())
          {
          rep->SetHandleColor(i, rgbNode[1], rgbNode[2], rgbNode[3]);
          }
        }
      return;
      }
    }

  unsigned int currentHandleNumber = rep->CreateHandle(displayPos, scalar);
  if (this->ModificationType != OPACITY)
    {
    // color the node based on the CTF
    double colorNode[6];
    this->ColorFunction->GetNodeValue(currentHandleNumber, colorNode);
    if (rep->GetColorElementsByColorFunction())
      {
      rep->SetHandleColor(currentHandleNumber, colorNode[1], colorNode[2], colorNode[3]);
      }
    }
  vtkHandleWidget *newHandleWidget = NULL;
  if (rep->GetNumberOfHandles() > this->Nodes->size())
    {
    newHandleWidget = this->CreateHandleWidget(
      this, rep, currentHandleNumber);
    newHandleWidget->SetEnabled(1);
    }
  rep->SetActiveHandle(currentHandleNumber);
  this->LeftClickEventPosition[0] = vtkMath::Round(displayPos[0]);
  this->LeftClickEventPosition[1] = vtkMath::Round(displayPos[1]);
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
    self->EventCallbackCommand->SetAbortFlag(1);
    self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
    self->Superclass::EndInteraction();
    self->Render();

    int x = self->Interactor->GetEventPosition()[0];
    int y = self->Interactor->GetEventPosition()[1];
    if (x == self->LeftClickEventPosition[0] &&
        y == self->LeftClickEventPosition[1] &&
        self->LeftClickCount > 0 &&
        self->ModificationType != OPACITY)
      {
      // Fire an event indicating that a node has been selected
      // so we can know when to display a color chooser.
      self->InvokeEvent(vtkCommand::PickEvent, NULL);
      self->EventCallbackCommand->SetAbortFlag(1);
      }
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

  if (self->WholeScalarRange[0] == self->WholeScalarRange[1])
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
  int size[2];
  rep->GetDisplaySize(size);
  if (self->ModificationType == COLOR)
    {
    pos[1] = size[1]/2;
    }
  double scalar = self->ComputeScalar(pos[0], size[0]);
  self->ClampToWholeRange(pos, size, scalar);
  if (!self->AllowInteriorElements)
    {
    pos[0] = rep->GetHandleDisplayPosition(nodeId)[0];
    }
  if (self->LockEndPoints)
    {
    if (nodeId == 0 || nodeId == self->Nodes->size()-1)
      {
      pos[0] = rep->GetHandleDisplayPosition(nodeId)[0];
      int valid;
      double oldScalar = rep->GetHandleScalar(nodeId, valid);
      if (valid)
        {
        scalar = oldScalar;
        }
      }
    }
  if (rep->SetHandleDisplayPosition(nodeId, pos, scalar))
    {
    if (self->ModificationType != COLOR)
      {
      self->RemoveOpacityPoint(nodeId);
      self->AddOpacityPoint(pos[0], pos[1]);
      }
    if (self->ModificationType != OPACITY)
      {
      self->RepositionColorPoint(nodeId, scalar);
      }

    self->EventCallbackCommand->SetAbortFlag(1);
    self->InvokeEvent(vtkCommand::InteractionEvent, NULL);
    self->Render();
    }
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::OnChar()
{
  this->Superclass::OnChar();

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    static_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!this->Interactor || !rep)
    {
    return;
    }

  char keyCode = this->Interactor->GetKeyCode();

  const char DelKey = 127;
  const char BackSpaceKey = 8;
  const char TabKey = 9;
  
  if(keyCode == 'd' || keyCode == 'D' ||
     keyCode == DelKey || keyCode == BackSpaceKey)
    {
    this->RemoveNode(rep->GetActiveHandle());
    }
  else if(keyCode == TabKey)
    {
    if(this->Interactor->GetShiftKey())
      {
      this->MoveToPreviousElement();
      }
    else
      {
      this->MoveToNextElement();
      }
    }
}

//-------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RemoveNode(unsigned int id)
{
  if (id > this->Nodes->size()-1 ||
      (this->LockEndPoints && (id == 0 || id == this->Nodes->size()-1)))
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
      this->InvokeEvent(vtkCommand::PlacePointEvent, (void*)&(i));
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
void vtkTransferFunctionEditorWidgetSimple1D::UpdateFromTransferFunctions()
{
  this->RemoveAllNodes();

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      this->WidgetRep);
  unsigned int activeHandle = 0;
  if (rep)
    {
    activeHandle = rep->GetActiveHandle();
    rep->RemoveAllHandles();
    }

  int i, idx, size;

  if (this->OpacityFunction->GetSize() == 0)
    {
    this->OpacityFunction->AddPoint(this->WholeScalarRange[0], 0);
    this->OpacityFunction->AddPoint(this->WholeScalarRange[1], 1);
    }
  if (this->ColorFunction->GetSize() == 0)
    {
    this->ColorFunction->AddRGBPoint(this->WholeScalarRange[0],
                                     this->InitialMinimumColor[0],
                                     this->InitialMinimumColor[1],
                                     this->InitialMinimumColor[2]);
    this->ColorFunction->AddRGBPoint(this->WholeScalarRange[1],
                                     this->InitialMaximumColor[0],
                                     this->InitialMaximumColor[1],
                                     this->InitialMaximumColor[2]);
    }

  if (this->ModificationType != COLOR)
    {
    double oNode[4];
    size = this->OpacityFunction->GetSize();
    for (i = 0, idx = 0; i < size; i++)
      {
      this->OpacityFunction->GetNodeValue(idx, oNode);
      idx++;
      if (this->AllowInteriorElements)
        {
        if (this->ModificationType == COLOR_AND_OPACITY &&
            this->WholeScalarRange[0] != this->WholeScalarRange[1])
          {
          double rgb[3];
          this->ColorFunction->GetColor(oNode[0], rgb);
          this->ColorFunction->AddRGBPoint(oNode[0], rgb[0], rgb[1], rgb[2]);
          }
        this->AddNewNode(oNode[0]);
        }
      else
        {
        this->OpacityFunction->RemovePoint(oNode[0]);
        if (i == 0)
          {
          this->OpacityFunction->AddPoint(this->WholeScalarRange[0],
                                          oNode[1], oNode[2], oNode[3]);
          if (this->ModificationType == COLOR_AND_OPACITY)
            {
            double rgb[3];
            this->ColorFunction->GetColor(this->WholeScalarRange[0], rgb);
            this->ColorFunction->AddRGBPoint(this->WholeScalarRange[0],
                                             rgb[0], rgb[1], rgb[2]);
            }
          this->AddNewNode(this->WholeScalarRange[0]);
          }
        else if (i == size-1)
          {
          this->OpacityFunction->AddPoint(this->WholeScalarRange[1],
                                          oNode[1], oNode[2], oNode[3]);
          if (this->ModificationType == COLOR_AND_OPACITY)
            {
            double rgb[3];
            this->ColorFunction->GetColor(this->WholeScalarRange[1], rgb);
            this->ColorFunction->AddRGBPoint(this->WholeScalarRange[1],
                                             rgb[0], rgb[1], rgb[2]);
            }
          this->AddNewNode(this->WholeScalarRange[1]);
          }
        else
          {
          idx--;
          }
        }
      }
    }

  if (this->ModificationType != OPACITY)
    {
    double cNode[6];
    size = this->ColorFunction->GetSize();
    for (i = 0, idx = 0; i < size; i++)
      {
      this->ColorFunction->GetNodeValue(idx, cNode);
      idx++;
      if (this->AllowInteriorElements)
        {
        if (this->ModificationType == COLOR_AND_OPACITY &&
            this->WholeScalarRange[0] != this->WholeScalarRange[1])
          {
          double opacity = this->OpacityFunction->GetValue(cNode[0]);
          this->OpacityFunction->AddPoint(cNode[0], opacity);
          }
        this->AddNewNode(cNode[0]);
        }
      else
        {
        this->ColorFunction->RemovePoint(cNode[0]);
        if (i == 0)
          {
          this->ColorFunction->AddRGBPoint(
            this->WholeScalarRange[0],
            cNode[1], cNode[2], cNode[3], cNode[4], cNode[5]);
          if (this->ModificationType == COLOR_AND_OPACITY)
            {
            double opacity = this->OpacityFunction->GetValue(
              this->WholeScalarRange[0]);
            this->OpacityFunction->AddPoint(this->WholeScalarRange[0],
                                            opacity);
            }
          this->AddNewNode(this->WholeScalarRange[0]);
          }
        else if (i == size-1)
          {
          this->ColorFunction->AddRGBPoint(
            this->WholeScalarRange[1],
            cNode[1], cNode[2], cNode[3], cNode[4], cNode[5]);
          if (this->ModificationType == COLOR_AND_OPACITY)
            {
            double opacity = this->OpacityFunction->GetValue(
              this->WholeScalarRange[1]);
            this->OpacityFunction->AddPoint(this->WholeScalarRange[1],
                                            opacity);
            }
          this->AddNewNode(this->WholeScalarRange[1]);
          }
        else
          {
          idx--;
          }
        }
      }
    }

  if (activeHandle < this->Nodes->size() && rep)
    {
    rep->SetActiveHandle(activeHandle);
    }
  this->UpdateTransferFunctionMTime();
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
  int oldSize[2], int newSize[2], int changeBorder, int oldWidth, int newWidth)
{
  // recompute transfer function node positions based on a change
  // in renderer size or border width
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
    if (changeBorder)
      {
      displayPctX = (oldPos[0] - oldWidth) / (double)(oldSize[0] - 2*oldWidth);
      displayPctY = (oldPos[1] - oldWidth) / (double)(oldSize[1] - 2*oldWidth);
      newPos[0] = displayPctX * (newSize[0]-2*newWidth) + newWidth;
      newPos[1] = displayPctY * (newSize[1]-2*newWidth) + newWidth;
      }
    else
      {
      displayPctX = oldPos[0] / (double)(oldSize[0]);
      displayPctY = oldPos[1] / (double)(oldSize[1]);
      newPos[0] = displayPctX * newSize[0];
      newPos[1] = displayPctY * newSize[1];
      }

    newPos[2] = oldPos[2];
    handle->SetDisplayPosition(newPos);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetBorderWidth(int width)
{
  int oldWidth = this->BorderWidth;
  this->Superclass::SetBorderWidth(width);
  int displaySize[2];
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      this->WidgetRep);
  if (oldWidth != this->BorderWidth && rep)
    {
    rep->GetDisplaySize(displaySize);
    this->RecomputeNodePositions(displaySize, displaySize, 1, oldWidth, width);
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
void vtkTransferFunctionEditorWidgetSimple1D::AddOpacityPoint(double x,
                                                              double y)
{
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!rep)
    {
    return;
    }

  int windowSize[2];
  double opacity, newScalar;

  rep->GetDisplaySize(windowSize);
  opacity =
    (y - this->BorderWidth) / (double)(windowSize[1] - 2*this->BorderWidth);
  newScalar = this->ComputeScalar(x, windowSize[0]);
  
  this->OpacityFunction->AddPoint(newScalar, opacity);
  this->UpdateTransferFunctionMTime();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::AddColorPoint(double x)
{
  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    reinterpret_cast<vtkTransferFunctionEditorRepresentationSimple1D*>
    (this->WidgetRep);

  if (!rep)
    {
    return;
    }

  int windowSize[2];
  double newScalar;

  rep->GetDisplaySize(windowSize);
  newScalar = this->ComputeScalar(x, windowSize[0]);

  int idx;
  double color[3];
  this->ColorFunction->GetColor(newScalar, color);
  idx = this->ColorFunction->AddRGBPoint(newScalar,
                                         color[0], color[1], color[2]);
  this->UpdateTransferFunctionMTime();
  this->SetElementRGBColor(idx, color[0], color[1], color[2]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RepositionColorPoint(
  unsigned int idx, double scalar)
{
  double value[6];
  this->ColorFunction->GetNodeValue(idx, value);
  this->RemoveColorPoint(idx);
  this->ColorFunction->AddRGBPoint(scalar, value[1], value[2], value[3]);
  this->UpdateTransferFunctionMTime();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RemoveOpacityPoint(
  unsigned int id)
{
  double value[4];
  this->OpacityFunction->GetNodeValue(id, value);
  this->OpacityFunction->RemovePoint(value[0]);
  this->UpdateTransferFunctionMTime();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::RemoveColorPoint(
  unsigned int id)
{
  double value[6];
  this->ColorFunction->GetNodeValue(id, value);
  this->ColorFunction->RemovePoint(value[0]);
  this->UpdateTransferFunctionMTime();
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorWidgetSimple1D::NodeExists(double scalar)
{
  int i;
  if (this->ModificationType != COLOR)
    {
    double opacityValue[4];
    for (i = 0; i < this->OpacityFunction->GetSize(); i++)
      {
      this->OpacityFunction->GetNodeValue(i, opacityValue);
      if (scalar == opacityValue[0])
        {
        return i;
        }
      }
    }
  else
    {
    double colorValue[6];
    for (i = 0; i < this->ColorFunction->GetSize(); i++)
      {
      this->ColorFunction->GetNodeValue(i, colorValue);
      if (scalar == colorValue[0])
        {
        return i;
        }
      }
    }
  return -1;
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
  value[1] = opacity;
  this->OpacityFunction->SetNodeValue(idx, value);
  this->UpdateTransferFunctionMTime();

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
  pos[1] = opacity * (size[1] - 2*this->BorderWidth) + this->BorderWidth;
  rep->SetHandleDisplayPosition(
    idx, pos, this->ComputeScalar(pos[0], size[0]));
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
  value[1] = r;
  value[2] = g;
  value[3] = b;
  this->ColorFunction->SetNodeValue(idx, value);
  this->UpdateTransferFunctionMTime();

  vtkTransferFunctionEditorRepresentationSimple1D *rep =
    vtkTransferFunctionEditorRepresentationSimple1D::SafeDownCast(
      this->WidgetRep);
  if (rep)
    {
    if (rep->GetColorElementsByColorFunction())
      {
      rep->SetHandleColor(idx, r, g, b);
      }
    if (rep->GetShowColorFunctionInBackground())
      {
      rep->BuildRepresentation();
      }
    this->Render();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetElementHSVColor(
  unsigned int idx, double h, double s, double v)
{
  double rgb[3];
  vtkMath::HSVToRGB(h, s, v, rgb, rgb+1, rgb+2);
  this->SetElementRGBColor(idx, rgb[0], rgb[1], rgb[2]);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetElementScalar(
  unsigned int idx, double value)
{
  unsigned int size = static_cast<unsigned int>(this->Nodes->size());
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
  double oldDispPos[3];
  rep->GetHandleDisplayPosition(idx, oldDispPos);
  double prevScalar, nextScalar, displayPos[3];
  displayPos[1] = oldDispPos[1];
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
      this->RepositionColorPoint(idx, value);
      rep->GetDisplaySize(displaySize);
      displayPos[0] = this->ComputePositionFromScalar(value, displaySize[0]);
      rep->SetHandleDisplayPosition(idx, displayPos, value);
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
      displayPos[0] = this->ComputePositionFromScalar(value, displaySize[0]);
      this->AddOpacityPoint(displayPos[0], displayPos[1]);
      rep->SetHandleDisplayPosition(idx, displayPos, value);
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      }
    }

  if (!allowSet)
    {
    vtkErrorMacro("Cannot move a transfer function node horizontally past the ones on either side of it.");
    }
}

//----------------------------------------------------------------------------
double vtkTransferFunctionEditorWidgetSimple1D::GetElementOpacity(
  unsigned int idx)
{
  if (idx >= static_cast<unsigned int>(this->OpacityFunction->GetSize()) ||
      this->ModificationType == COLOR)
    {
    return 0;
    }

  double value[4];
  this->OpacityFunction->GetNodeValue(idx, value);
  return value[1];
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorWidgetSimple1D::GetElementRGBColor(
  unsigned int idx, double color[3])
{
  if (idx >= static_cast<unsigned int>(this->ColorFunction->GetSize()) ||
      this->ModificationType == OPACITY)
    {
    return 0;
    }

  double value[6];
  this->ColorFunction->GetNodeValue(idx, value);
  color[0] = value[1];
  color[1] = value[2];
  color[2] = value[3];
  return 1;
}
  
//----------------------------------------------------------------------------
int vtkTransferFunctionEditorWidgetSimple1D::GetElementHSVColor(
  unsigned int idx, double color[3])
{
  if (idx >= static_cast<unsigned int>(this->ColorFunction->GetSize()) ||
      this->ModificationType == OPACITY)
    {
    return 0;
    }

  double value[6];
  this->ColorFunction->GetNodeValue(idx, value);
  color[0] = value[1];
  color[1] = value[2];
  color[2] = value[3];
  vtkMath::RGBToHSV(color, color);
  return 1;
}
  
//----------------------------------------------------------------------------
double vtkTransferFunctionEditorWidgetSimple1D::GetElementScalar(
  unsigned int idx)
{
  if (this->ModificationType != COLOR)
    {
    if (idx >= static_cast<unsigned int>(this->OpacityFunction->GetSize()))
      {
      return 0;
      }
    double opacity[4];
    this->OpacityFunction->GetNodeValue(idx, opacity);
    return opacity[0];
    }
  else
    {
    if (idx >= static_cast<unsigned int>(this->ColorFunction->GetSize()))
      {
      return 0;
      }
    double color[6];
    this->ColorFunction->GetNodeValue(idx, color);
    return color[0];
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::SetColorSpace(int space)
{
  if (space < 0 || space > 4)
    {
    vtkWarningMacro("Invalid color space.");
    return;
    }

  switch (space)
    {
    case 0:
      this->ColorFunction->SetColorSpaceToRGB();
      break;
    case 1:
      this->ColorFunction->SetColorSpaceToHSV();
      this->ColorFunction->HSVWrapOff();
      break;
    case 2:
      this->ColorFunction->SetColorSpaceToHSV();
      this->ColorFunction->HSVWrapOn();
      break;
    case 3:
      this->ColorFunction->SetColorSpaceToLab();
      break;
    case 4:
      this->ColorFunction->SetColorSpaceToDiverging();
      break;
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorWidgetSimple1D::PrintSelf(ostream& os,
                                                        vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
