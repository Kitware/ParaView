/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionEditorRepresentationSimple1D.cxx,v

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionEditorRepresentationSimple1D.h"

#include "vtkActor.h"
#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkPointHandleRepresentationSphere.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPropCollection.h"
#include "vtkProperty.h"
#include "vtkObjectFactory.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkViewport.h"

#include <vtkstd/list>

vtkCxxRevisionMacro(vtkTransferFunctionEditorRepresentationSimple1D, "1.7");
vtkStandardNewMacro(vtkTransferFunctionEditorRepresentationSimple1D);

// The vtkHandleList is a PIMPLed list<T>.
class vtkHandleList : public vtkstd::list<vtkHandleRepresentation*> {};
typedef vtkstd::list<vtkHandleRepresentation*>::iterator vtkHandleListIterator;

//----------------------------------------------------------------------------
class vtkTFERSimple1DDisplayXLess 
{
public:
  bool operator()(vtkHandleRepresentation* r1,
                  vtkHandleRepresentation* r2) const
    {
      double pos1[3], pos2[3];
      r1->GetDisplayPosition(pos1);
      r2->GetDisplayPosition(pos2);
      return pos1[0] < pos2[0];
    }
};

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentationSimple1D::vtkTransferFunctionEditorRepresentationSimple1D()
{
  this->Handles = new vtkHandleList;
  this->HandleRepresentation = vtkPointHandleRepresentationSphere::New();
  this->HandleRepresentation->SetSelectedProperty(
    this->HandleRepresentation->GetProperty());
  this->ActiveHandle = VTK_UNSIGNED_INT_MAX;
  this->Tolerance = 5;

  vtkTransform *xform = vtkTransform::New();
  xform->Scale(1.35, 1.35, 1.35);
  this->ActiveHandleFilter = vtkTransformPolyDataFilter::New();
  this->ActiveHandleFilter->SetInput(
    this->HandleRepresentation->GetCursorShape());
  this->ActiveHandleFilter->SetTransform(xform);
  xform->Delete();

  this->Lines = vtkPolyData::New();
  this->LinesMapper = vtkPolyDataMapper::New();
  this->LinesMapper->SetInput(this->Lines);
  this->LinesActor = vtkActor::New();
  this->LinesActor->SetMapper(this->LinesMapper);
}

//----------------------------------------------------------------------------
vtkTransferFunctionEditorRepresentationSimple1D::~vtkTransferFunctionEditorRepresentationSimple1D()
{
  this->RemoveAllHandles();
  delete this->Handles;

  this->HandleRepresentation->Delete();
  this->ActiveHandleFilter->Delete();

  this->Lines->Delete();
  this->LinesMapper->Delete();
  this->LinesActor->Delete();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::BuildRepresentation()
{
  this->Superclass::BuildRepresentation();

  // Add lines between the handles if there is more than 1.
  if (this->Handles->size() > 1)
    {
    this->Lines->Initialize();
    this->Lines->Allocate();

    unsigned int i = 1;
    double lastPos[3], pos[3];
    vtkHandleListIterator hiter = this->Handles->begin();
    (*hiter)->GetDisplayPosition(lastPos);
    hiter++;
    vtkPoints *pts = vtkPoints::New();
    lastPos[2] = -8;
    pts->InsertNextPoint(lastPos);
    vtkIdType *ids = new vtkIdType[2];

    for ( ; hiter != this->Handles->end(); hiter++, i++)
      {
      ids[0] = i-1;
      ids[1] = i;
      (*hiter)->GetDisplayPosition(pos);
      pos[2] = -8;
      pts->InsertNextPoint(pos);
      this->Lines->InsertNextCell(VTK_LINE, 2, ids);
      lastPos[0] = pos[0];
      lastPos[1] = pos[0];
      lastPos[2] = pos[0];
      }
    this->Lines->SetPoints(pts);
    pts->Delete();
    delete [] ids;
    }
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentationSimple1D::RenderOpaqueGeometry(
  vtkViewport *viewport)
{
  int ret = this->Superclass::RenderOpaqueGeometry(viewport);

  if (this->Handles->size() > 1)
    {
    ret += this->LinesActor->RenderOpaqueGeometry(viewport);
    }

  return ret;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentationSimple1D::RenderTranslucentGeometry(
  vtkViewport *viewport)
{
  int ret = this->Superclass::RenderTranslucentGeometry(viewport);

  if (this->Handles->size() > 1)
    {
    ret += this->LinesActor->RenderTranslucentGeometry(viewport);
    }

  return ret;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentationSimple1D::RenderOverlay(
  vtkViewport *viewport)
{
  int ret = this->Superclass::RenderOverlay(viewport);

  if (this->Handles->size() > 1)
    {
    ret += this->LinesActor->RenderOverlay(viewport);
    }

  return ret;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentationSimple1D::ComputeInteractionState(
  int x, int y, int vtkNotUsed(modify))
{
    // Loop over all the seeds to see if the point is close to any of them.
  double xyz[3], pos[3];
  double tol2 = this->Tolerance*this->Tolerance;
  xyz[0] = static_cast<double>(x);
  xyz[1] = static_cast<double>(y);
  xyz[2] = 0.0;

  unsigned int i;
  vtkHandleListIterator iter;
  for (i=0, iter=this->Handles->begin(); iter != this->Handles->end(); ++iter, ++i )
    {
    if ( *iter != NULL )
      {
      (*iter)->GetDisplayPosition(pos);
      if ( vtkMath::Distance2BetweenPoints(xyz,pos) <= tol2 )
        {
        this->InteractionState =
          vtkTransferFunctionEditorRepresentationSimple1D::NearNode;
        this->SetActiveHandle(i);
        return this->InteractionState;
        }
      }
    }

  // Nothing found, so it's outside
  this->InteractionState =
    vtkTransferFunctionEditorRepresentationSimple1D::Outside;

  return this->InteractionState;
}

//----------------------------------------------------------------------------
vtkHandleRepresentation* vtkTransferFunctionEditorRepresentationSimple1D::GetHandleRepresentation(unsigned int idx)
{
  if ( idx < this->Handles->size() )
    {
    unsigned int i = 0;
    vtkHandleListIterator hiter;
    for (hiter = this->Handles->begin(); hiter != this->Handles->end();
         hiter++, i++)
      {
      if (i == idx)
        {
        return *hiter;
        }
      }
    return NULL;
    }
  return NULL;
}

//----------------------------------------------------------------------------
unsigned int vtkTransferFunctionEditorRepresentationSimple1D::CreateHandle(
  double displayPos[3])
{
  vtkHandleRepresentation *rep = this->HandleRepresentation->NewInstance();
  rep->ShallowCopy(this->HandleRepresentation);
  vtkProperty *property = vtkProperty::New();
  property->SetDiffuse(0.8);
  property->SetSpecular(0.5);
  property->SetSpecularPower(5);
  property->DeepCopy(this->HandleRepresentation->GetProperty());
  vtkPointHandleRepresentationSphere *pointRep =
    static_cast<vtkPointHandleRepresentationSphere*>(rep);
  pointRep->SetProperty(property);
  pointRep->SetSelectedProperty(property);
  property->Delete();

  rep->SetDisplayPosition(displayPos);
  vtkHandleListIterator hiter;
  double tmpPos[3];
  unsigned int i = 0, inserted = 0;
  for (hiter = this->Handles->begin(); hiter != this->Handles->end();
       hiter++, i++)
    {
    (*hiter)->GetDisplayPosition(tmpPos);
    if (tmpPos[0] > displayPos[0])
      {
      this->Handles->insert(hiter, rep);
      inserted = 1;
      break;
      }
    }

  if (!inserted)
    {
    this->Handles->insert(this->Handles->end(), rep);
    }

  return i;
}

//----------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::SetHandleColor(
  unsigned int idx, double r, double g, double b)
{
  vtkPointHandleRepresentationSphere *handleRep =
    vtkPointHandleRepresentationSphere::SafeDownCast(
      this->GetHandleRepresentation(idx));
  if (handleRep)
    {
    handleRep->GetProperty()->SetColor(r, g, b);
    this->UpdateHandleProperty(handleRep);
    }
}

//----------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::UpdateHandleProperty(
  vtkPointHandleRepresentationSphere *handleRep)
{
  vtkPropCollection *pc = vtkPropCollection::New();
  handleRep->GetActors(pc);
  vtkActor *actor = vtkActor::SafeDownCast(pc->GetItemAsObject(0));
  if (actor)
    {
    actor->SetProperty(handleRep->GetProperty());
    }
  pc->Delete();
}

//----------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::SetHandleDisplayPosition(
  unsigned int nodeNum, double pos[3])
{
  if ( nodeNum >= this->Handles->size() )
    {
    vtkErrorMacro("Trying to access non-existent handle");
    return;
    }

  vtkHandleListIterator hiter, tmpIter;
  unsigned int i = 0;
  int allowSet = 0;
  double lastPos[3] = {0, 0, 0};
  double nextPos[3] = {0, 0, 0};

  for (hiter = this->Handles->begin(); hiter != this->Handles->end();
       hiter++, i++)
    {
    if (i == nodeNum)
      {
      tmpIter = hiter;
      if (this->Handles->size() == 1)
        {
        allowSet = 1;
        }
      else if (i == 0)
        {
        tmpIter++;
        (*tmpIter)->GetDisplayPosition(nextPos);
        if (nextPos[0] > pos[0])
          {
          allowSet = 1;
          }
        }
      else if (i == this->Handles->size()-1)
        {
        tmpIter--;
        (*tmpIter)->GetDisplayPosition(lastPos);
        if (lastPos[0] < pos[0])
          {
          allowSet = 1;
          }
        }
      else
        {
        tmpIter--;
        (*tmpIter)->GetDisplayPosition(lastPos);
        tmpIter++;
        tmpIter++;
        (*tmpIter)->GetDisplayPosition(nextPos);
        if (lastPos[0] < pos[0] && nextPos[0] > pos[0])
          {
          allowSet = 1;
          }
        }
      if (allowSet)
        {
        (*hiter)->SetDisplayPosition(pos);
        this->BuildRepresentation();
        this->InvokeEvent(vtkCommand::WidgetValueChangedEvent, NULL);
        return;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::GetHandleDisplayPosition(
  unsigned int nodeNum, double pos[3])
{
  if (nodeNum > this->Handles->size()-1)
    {
    return;
    }

  vtkHandleListIterator iter;
  unsigned int i = 0;
  for (iter = this->Handles->begin(); iter != this->Handles->end();
       iter++, i++)
    {
    if (i == nodeNum)
      {
      (*iter)->GetDisplayPosition(pos);
      return;
      }
    }  
}

//----------------------------------------------------------------------------
double* vtkTransferFunctionEditorRepresentationSimple1D::GetHandleDisplayPosition(
  unsigned int nodeNum)
{
  if (nodeNum > this->Handles->size()-1)
    {
    return NULL;
    }

  vtkHandleListIterator iter;
  unsigned int i = 0;
  for (iter = this->Handles->begin(); iter != this->Handles->end();
       iter++, i++)
    {
    if (i == nodeNum)
      {
      return (*iter)->GetDisplayPosition();
      }
    }  

  return NULL;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::RemoveHandle(
  unsigned int id)
{
  if (id > this->Handles->size()-1)
    {
    return;
    }

  vtkHandleListIterator iter;
  unsigned int i = 0;
  for (iter = this->Handles->begin(); iter != this->Handles->end();
       iter++, i++)
    {
    if (i == id)
      {
      (*iter)->Delete();
      this->Handles->erase(iter);
      this->BuildRepresentation();
      return;
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::RemoveAllHandles()
{
  vtkHandleListIterator hiter;
  for (hiter = this->Handles->begin(); hiter != this->Handles->end();)
    {
    (*hiter)->Delete();
    this->Handles->erase(hiter++);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::SetActiveHandle(
  unsigned int handle)
{
  this->ActiveHandle = handle;
  this->HighlightActiveHandle();
  this->InvokeEvent(vtkCommand::WidgetValueChangedEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::HighlightActiveHandle()
{
  vtkHandleListIterator iter;
  unsigned int i = 0;
  vtkPointHandleRepresentationSphere *rep;
  for (iter = this->Handles->begin(); iter != this->Handles->end();
       iter++, i++)
    {
    rep = vtkPointHandleRepresentationSphere::SafeDownCast(*iter);
    if (rep)
      {
      if (i == this->ActiveHandle)
        {
        rep->SetCursorShape(this->ActiveHandleFilter->GetOutput());
        }
      else
        {
        rep->SetCursorShape(this->HandleRepresentation->GetCursorShape());
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
