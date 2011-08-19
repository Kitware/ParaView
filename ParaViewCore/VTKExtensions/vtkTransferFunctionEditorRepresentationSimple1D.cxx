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
#include "vtkClipPolyData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkPlane.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
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
  this->LinesMapper->InterpolateScalarsBeforeMappingOn();
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

  int bkndDepth = -12;
  int histDepth = -10;
  int linesDepth = -8;

  // Add lines between the handles if there is at least 1.
  if (this->Handles->size() > 0)
    {
    int minX = this->BorderWidth;
    int maxX = this->DisplaySize[0] - this->BorderWidth;
    int minY = this->BorderWidth;
    int maxY = this->DisplaySize[1] - this->BorderWidth;

    vtkPlaneSource *plane = vtkPlaneSource::New();
    plane->SetOrigin(minX, minY, histDepth);
    plane->SetPoint1(maxX, minY, histDepth);
    plane->SetPoint2(minX, maxY, histDepth);
    plane->SetCenter(this->DisplaySize[0]*0.5, this->DisplaySize[1]*0.5,
                     histDepth);
    plane->Update();
    this->HistogramGeometry->DeepCopy(plane->GetOutput());
    plane->Delete();

    this->BackgroundImage->Initialize();
    this->BackgroundImage->Allocate();
    vtkDoubleArray *bkndScalars = vtkDoubleArray::New();
    bkndScalars->SetNumberOfComponents(1);
    bkndScalars->SetNumberOfTuples(2*this->Handles->size()+4);
    vtkPoints *bkndPts = vtkPoints::New();
    bkndPts->InsertNextPoint(minX, minY, bkndDepth);
    bkndPts->InsertNextPoint(minX, maxY, bkndDepth);
    bkndScalars->SetValue(0, this->VisibleScalarRange[0]);
    bkndScalars->SetValue(1, this->VisibleScalarRange[0]);
    vtkIdType *bkndIds = new vtkIdType[4];
    bkndIds[0] = 1;
    bkndIds[1] = 0;
    
    this->Lines->Initialize();
    this->Lines->Allocate();

    vtkDoubleArray *scalars = vtkDoubleArray::New();
    scalars->SetNumberOfComponents(1);
    scalars->SetNumberOfTuples(this->Handles->size());

    double scalar;
    unsigned int i = 1;
    unsigned int bi = 2;
    double lastPos[3], pos[3];
    vtkHandleListIterator hiter = this->Handles->begin();
    (*hiter)->GetDisplayPosition(lastPos);
    vtkPointHandleRepresentationSphere *rep =
      vtkPointHandleRepresentationSphere::SafeDownCast(*hiter);
    if (rep)
      {
      scalar = rep->GetScalar();
      if (scalar < this->VisibleScalarRange[0] ||
          scalar > this->VisibleScalarRange[1])
        {
        rep->VisibilityOff();
        }
      else
        {
        rep->VisibilityOn();
        if (scalar > this->VisibleScalarRange[0] &&
            scalar < this->VisibleScalarRange[1])
          {
          bkndScalars->SetValue(2, scalar);
          bkndScalars->SetValue(3, scalar);
          bkndPts->InsertNextPoint(lastPos[0], minY, bkndDepth);
          bkndPts->InsertNextPoint(lastPos[0], maxY, bkndDepth);
          bkndIds[2] = bi++;
          bkndIds[3] = bi++;
          this->BackgroundImage->InsertNextCell(VTK_QUAD, 4, bkndIds);
          bkndIds[0] = bkndIds[3];
          bkndIds[1] = bkndIds[2];
          }
        }
      scalars->SetValue(0, rep->GetScalar());
      }
    hiter++;
    vtkPoints *pts = vtkPoints::New();
    lastPos[2] = linesDepth;
    pts->InsertNextPoint(lastPos);
    vtkIdType *ids = new vtkIdType[2];

    for ( ; hiter != this->Handles->end(); hiter++, i++)
      {
      ids[0] = i-1;
      ids[1] = i;
      (*hiter)->GetDisplayPosition(pos);
      rep = vtkPointHandleRepresentationSphere::SafeDownCast(*hiter);
      if (rep)
        {
        scalar = rep->GetScalar();
        if (scalar < this->VisibleScalarRange[0] ||
            scalar > this->VisibleScalarRange[1])
          {
          rep->VisibilityOff();
          }
        else
          {
          rep->VisibilityOn();
          if (scalar > this->VisibleScalarRange[0] &&
              scalar < this->VisibleScalarRange[1])
            {
            bkndIds[2] = bi++;
            bkndIds[3] = bi++;
            bkndScalars->SetValue(bkndIds[2], scalar);
            bkndScalars->SetValue(bkndIds[3], scalar);
            bkndPts->InsertNextPoint(pos[0], minY, bkndDepth);
            bkndPts->InsertNextPoint(pos[0], maxY, bkndDepth);
            this->BackgroundImage->InsertNextCell(VTK_QUAD, 4, bkndIds);
            bkndIds[0] = bkndIds[3];
            bkndIds[1] = bkndIds[2];
            }
          }
        scalars->SetValue(i, scalar);
        }
      pos[2] = linesDepth;
      pts->InsertNextPoint(pos);
      this->Lines->InsertNextCell(VTK_LINE, 2, ids);
      lastPos[0] = pos[0];
      lastPos[1] = pos[0];
      lastPos[2] = pos[0];
      }
    if (this->Handles->size() > 1)
      {
      this->Lines->SetPoints(pts);
      this->Lines->GetPointData()->SetScalars(scalars);

      // Clip the Lines so they don't run into the border.
      // X minimum
      vtkPlane *minXPlane = vtkPlane::New();
      minXPlane->SetOrigin(minX, 0, 0);
      minXPlane->SetNormal(1, 0, 0);

      vtkClipPolyData *minXClip = vtkClipPolyData::New();
      minXClip->SetInput(this->Lines);
      minXClip->SetClipFunction(minXPlane);
    
      // X maximum
      vtkPlane *maxXPlane = vtkPlane::New();
      maxXPlane->SetOrigin(maxX, 0, 0);
      maxXPlane->SetNormal(-1, 0, 0);

      vtkClipPolyData *maxXClip = vtkClipPolyData::New();
      maxXClip->SetInputConnection(minXClip->GetOutputPort());
      maxXClip->SetClipFunction(maxXPlane);
      this->LinesMapper->SetInputConnection(maxXClip->GetOutputPort());

      minXPlane->Delete();
      minXClip->Delete();
      maxXPlane->Delete();
      maxXClip->Delete();
      }

    bkndIds[2] = bi;
    bkndIds[3] = bi+1;
    bkndPts->InsertNextPoint(maxX, minY, bkndDepth);
    bkndPts->InsertNextPoint(maxX, maxY, bkndDepth);
    bkndScalars->SetValue(bkndIds[2], this->VisibleScalarRange[1]);
    bkndScalars->SetValue(bkndIds[3], this->VisibleScalarRange[1]);
    this->BackgroundImage->InsertNextCell(VTK_QUAD, 4, bkndIds);
    this->BackgroundImage->SetPoints(bkndPts);
    this->BackgroundImage->GetPointData()->SetScalars(bkndScalars);

    pts->Delete();
    scalars->Delete();
    bkndPts->Delete();
    bkndScalars->Delete();
    delete [] ids;
    delete [] bkndIds;
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
int vtkTransferFunctionEditorRepresentationSimple1D::RenderTranslucentPolygonalGeometry(
  vtkViewport *viewport)
{
  int ret = this->Superclass::RenderTranslucentPolygonalGeometry(viewport);

  if (this->Handles->size() > 1)
    {
    ret += this->LinesActor->RenderTranslucentPolygonalGeometry(viewport);
    }

  return ret;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionEditorRepresentationSimple1D::HasTranslucentPolygonalGeometry()
{
  int ret = this->Superclass::HasTranslucentPolygonalGeometry();

  if (this->Handles->size() > 1)
    {
    ret |= this->LinesActor->HasTranslucentPolygonalGeometry();
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
void vtkTransferFunctionEditorRepresentationSimple1D::ReleaseGraphicsResources(
  vtkWindow *window)
{
  if (this->LinesActor)
    {
    this->LinesActor->ReleaseGraphicsResources(window);
    }
  this->Superclass::ReleaseGraphicsResources(window);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::SetColorLinesByScalar(
  int color)
{
  this->LinesMapper->SetScalarVisibility(color);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::SetLinesColor(
  double r, double g, double b)
{
  this->LinesActor->GetProperty()->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::SetColorElementsByColorFunction(int color)
{
  this->Superclass::SetColorElementsByColorFunction(color);

  this->ColorAllElements();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::SetElementsColor(
  double r, double g, double b)
{
  this->Superclass::SetElementsColor(r, g, b);
  this->ColorAllElements();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::ColorAllElements()
{
  unsigned int i;
  if (!this->ColorElementsByColorFunction)
    {
    for (i = 0; i < this->Handles->size(); i++)
      {
      this->SetHandleColor(i, this->ElementsColor[0], this->ElementsColor[1],
                           this->ElementsColor[2]);
      }
    }
  else if (this->ColorFunction)
    {
    double color[3];
    vtkHandleListIterator iter;
    vtkPointHandleRepresentationSphere *sphereHandle;
    for (iter = this->Handles->begin(), i = 0; iter != this->Handles->end();
         iter++, i++)
      {
      sphereHandle = vtkPointHandleRepresentationSphere::SafeDownCast(*iter);
      if (sphereHandle)
        {
        this->ColorFunction->GetColor(sphereHandle->GetScalar(), color);
        this->SetHandleColor(i, color[0], color[1], color[2]);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::SetElementLighting(
  double ambient, double diffuse, double specular, double specularPower)
{
  vtkHandleListIterator iter;

  vtkProperty *prop;
  for (iter = this->Handles->begin(); iter != this->Handles->end(); iter++)
    {
    vtkPointHandleRepresentationSphere *handle =
      vtkPointHandleRepresentationSphere::SafeDownCast(*iter);
    if (handle)
      {
      prop = handle->GetProperty();
      prop->SetAmbient(ambient);
      prop->SetDiffuse(diffuse);
      prop->SetSpecular(specular);
      prop->SetSpecularPower(specularPower);
      }
    }

  prop = this->HandleRepresentation->GetProperty();
  prop->SetAmbient(ambient);
  prop->SetDiffuse(diffuse);
  prop->SetSpecular(specular);
  prop->SetSpecularPower(specularPower);
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
  double displayPos[3], double scalar)
{
  vtkHandleRepresentation *rep = this->HandleRepresentation->NewInstance();
  rep->ShallowCopy(this->HandleRepresentation);
  vtkProperty *property = vtkProperty::New();
  property->DeepCopy(this->HandleRepresentation->GetProperty());
  vtkPointHandleRepresentationSphere *pointRep =
    static_cast<vtkPointHandleRepresentationSphere*>(rep);
  pointRep->SetProperty(property);
  pointRep->SetSelectedProperty(property);
  pointRep->SetScalar(scalar);
  pointRep->SetAddCircleAroundSphere(1);
  property->Delete();

  rep->SetDisplayPosition(displayPos);
 
  vtkHandleListIterator hiter;
  unsigned int i;
  double tmpPos[3];
  int inserted = 0;
  for (hiter = this->Handles->begin(), i=0; hiter != this->Handles->end();
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

  this->SetHandleColor(i, this->ElementsColor[0], this->ElementsColor[1],
                       this->ElementsColor[2]);

  return i;
}

//----------------------------------------------------------------------
unsigned int
vtkTransferFunctionEditorRepresentationSimple1D::GetNumberOfHandles()
{
  return static_cast<unsigned int>(this->Handles->size());
}

//----------------------------------------------------------------------
double vtkTransferFunctionEditorRepresentationSimple1D::GetHandleScalar(
  unsigned int idx, int &valid)
{
  vtkPointHandleRepresentationSphere *handleRep =
    vtkPointHandleRepresentationSphere::SafeDownCast(
      this->GetHandleRepresentation(idx));
  if (handleRep)
    {
    valid = 1;
    return handleRep->GetScalar();
    }

  valid = 0;
  return 0;
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
int vtkTransferFunctionEditorRepresentationSimple1D::SetHandleDisplayPosition(
  unsigned int nodeNum, double pos[3], double scalar)
{
  if ( nodeNum >= this->Handles->size() )
    {
    vtkErrorMacro("Trying to access non-existent handle");
    return 0;
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
        vtkPointHandleRepresentationSphere *rep =
          vtkPointHandleRepresentationSphere::SafeDownCast(*hiter);
        if (rep)
          {
          rep->SetScalar(scalar);
          }
        this->BuildRepresentation();
        this->InvokeEvent(vtkCommand::WidgetValueChangedEvent, NULL);
        return 1;
        }
      }
    }
  return 0;
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
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, NULL);
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
        rep->Highlight(1);
        }
      else
        {
        rep->SetCursorShape(this->HandleRepresentation->GetCursorShape());
        rep->Highlight(0);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::SetColorFunction(
  vtkColorTransferFunction *color)
{
  this->Superclass::SetColorFunction(color);
  this->LinesMapper->SetLookupTable(color);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionEditorRepresentationSimple1D::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ActiveHandle: " << this->ActiveHandle << endl;
  os << indent << "Tolerance: " << this->Tolerance << endl;
}
