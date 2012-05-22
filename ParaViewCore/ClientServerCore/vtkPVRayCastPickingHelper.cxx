/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRayCastPickingHelper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVRayCastPickingHelper - helper class that used selection and ray
// casting to find the intersection point between the user picking point
// and the concreate cell underneath.
#include "vtkPVRayCastPickingHelper.h"

#include "vtkCell.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVExtractSelection.h"
#include "vtkPVRenderView.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"

#include <assert.h>

vtkStandardNewMacro(vtkPVRayCastPickingHelper);
vtkCxxSetObjectMacro(vtkPVRayCastPickingHelper, Input, vtkAlgorithm);
vtkCxxSetObjectMacro(vtkPVRayCastPickingHelper, Selection, vtkAlgorithm);
//----------------------------------------------------------------------------
vtkPVRayCastPickingHelper::vtkPVRayCastPickingHelper()
{
  this->Selection = NULL;
  this->Input = NULL;
  this->PointA[0] = this->PointA[1] = this->PointA[2] = 0.0;
  this->PointB[0] = this->PointB[1] = this->PointB[2] = 0.0;
}

//----------------------------------------------------------------------------
vtkPVRayCastPickingHelper::~vtkPVRayCastPickingHelper()
{
  this->SetSelection(NULL);
  this->SetInput(NULL);
}

//----------------------------------------------------------------------------
void vtkPVRayCastPickingHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
  os << indent << "PointA: "
     << this->PointA[0] << ", " << this->PointA[1] << ", " << this->PointA[2]
     << endl;
  os << indent << "PointB: "
     << this->PointB[0] << ", " << this->PointB[1] << ", " << this->PointB[2]
     << endl;
  os << indent << "Last Intersection: "
     << this->Intersection[0] << ", " << this->Intersection[1] << ", "
     << this->Intersection[2] << endl;
  os << indent << "Input: "
     << (this->Input ? this->Input->GetClassName() : "NULL") << endl;
  os << indent << "Selection: "
     << (this->Selection ? this->Selection->GetClassName() : "NULL") << endl;
}

//----------------------------------------------------------------------------
void vtkPVRayCastPickingHelper::ComputeIntersection()
{
  assert("Need valid input" && this->Input && this->Selection);
  assert("Need valid ray" && vtkMath::Distance2BetweenPoints(this->PointA, this->PointB));

  // Reset the intersection value
  this->Intersection[0] = this->Intersection[1] = this->Intersection[2] = 0.0;

  double tolerance = 0.1;
  double t;
  int subId;
  double pcoord[3];

  vtkNew<vtkPVExtractSelection> extractSelectionFilter;
  extractSelectionFilter->SetInputConnection(0,this->Input->GetOutputPort(0));
  extractSelectionFilter->SetInputConnection(1,this->Selection->GetOutputPort(0));
  extractSelectionFilter->Update();
  vtkDataSet* ds = vtkDataSet::SafeDownCast(extractSelectionFilter->GetOutput());
  vtkCompositeDataSet* cds =
      vtkCompositeDataSet::SafeDownCast(extractSelectionFilter->GetOutput());
  if(ds && ds->GetNumberOfCells() > 0)
    {
    if(ds->GetCell(0)->IntersectWithLine(
         this->PointA, this->PointB, tolerance, t, this->Intersection,
         pcoord, subId) == 0 && t == VTK_DOUBLE_MAX)
      {
      vtkErrorMacro("The intersection was not properly found");
      }
    }
  else if(cds)
    {
    vtkSmartPointer<vtkCompositeDataIterator> dsIter;
    dsIter.TakeReference(cds->NewIterator());
    for(dsIter->GoToFirstItem();!dsIter->IsDoneWithTraversal();dsIter->GoToNextItem())
      {
      ds = vtkDataSet::SafeDownCast(dsIter->GetCurrentDataObject());
      if(ds && ds->GetNumberOfCells() > 0)
        {
        if(ds->GetCell(0)->IntersectWithLine(
             this->PointA, this->PointB, tolerance, t, this->Intersection,
             pcoord, subId) == 0 && t == VTK_DOUBLE_MAX)
          {
          vtkErrorMacro("The intersection was not properly found");
          }
        else
          {
          break;
          }
        }
      }
    }

  // Now do the global reduction so every node will get the right localtion
  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();
  if(controller->GetNumberOfProcesses() > 1)
    {
    controller->AllReduce(this->Intersection, this->Intersection, 3, vtkCommunicator::SUM_OP);
    }
}
