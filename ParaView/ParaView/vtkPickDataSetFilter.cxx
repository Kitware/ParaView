/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPickDataSetFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPickDataSetFilter.h"

#include "vtkTransform.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkUnsignedCharArray.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"

vtkCxxRevisionMacro(vtkPickDataSetFilter, "1.1");
vtkStandardNewMacro(vtkPickDataSetFilter);


vtkPickDataSetFilter::vtkPickDataSetFilter()
{
  this->SearchCells = 0;

  this->SamplePoint[0] = 0;
  this->SamplePoint[1] = 0;
  this->SamplePoint[2] = 0;
}

vtkPickDataSetFilter::~vtkPickDataSetFilter()
{
}

void vtkPickDataSetFilter::Execute()
{
  vtkIdType inPtId, inClId;
  vtkPoints *newPts=0;
  vtkTransform *trans = vtkTransform::New();
  vtkIdList *pts;
  vtkUnstructuredGrid *output = this->GetOutput();
  vtkDataSet *input = this->GetInput();

  vtkDebugMacro(<<"Generating glyphs");

  pts = vtkIdList::New();
  pts->Allocate(VTK_CELL_SIZE);

  if (!input)
    {
    vtkErrorMacro(<<"No input");
    return;
    }

  if ( input->GetNumberOfPoints() <= 0 )
    {
    vtkErrorMacro(<<"No points to probe");
    return;
    }

  output->Allocate(1, 10);

  float mindistance = VTK_FLOAT_MAX;
  float* spoint = this->GetSamplePoint();
  if ( this->SearchCells )
    {
    vtkCell *foundcell = 0;
    vtkIdType cellIdx = -1;
    for ( inClId=0; inClId < input->GetNumberOfCells(); inClId++)
      {
      vtkCell* ccell = input->GetCell(inClId);
      float distance = VTK_FLOAT_MAX;
      int subId = 0;
      float pcoord[3] = { 0, 0, 0 };
      float cpoint[3] = { 0, 0, 0 };
      float weights[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
      ccell->EvaluatePosition(spoint, cpoint, subId, pcoord, distance, 
                              weights);
      if ( distance < mindistance )
        {
        mindistance = distance;
        if ( foundcell )
          {
          foundcell->Delete();
          }
        foundcell = ccell->NewInstance();
        foundcell->DeepCopy(ccell);
        cellIdx = inClId;
        }
      }
    if ( foundcell )
      {
      newPts = vtkPoints::New();
      vtkIdType *points = new vtkIdType[foundcell->GetNumberOfPoints()];
      int cc;
      for ( cc = 0; cc < foundcell->GetNumberOfPoints(); cc ++ )
        {
        float* point = input->GetPoint(foundcell->GetPointId(cc));
        vtkIdType id = newPts->InsertNextPoint(point);
        points[cc] = id;
        }
      
      output->SetPoints(newPts);
      output->InsertNextCell( foundcell->GetCellType(),
                              foundcell->GetNumberOfPoints(),
                              points);
      delete [] points;
      foundcell->Delete();    
      newPts->Delete();
      }
    }
  else
    {
    // Find point that is the closest to the sample point
    vtkIdType pointIdx = -1;
    float foundpoint[3] = { 0, 0, 0 };
    int cc;
    for ( inPtId=0; inPtId < input->GetNumberOfPoints(); inPtId++)
      {
      float *point = input->GetPoint(inPtId);
      float distance = vtkMath::Distance2BetweenPoints(spoint, point);
      if ( distance < mindistance )
        {
        mindistance = distance;
        pointIdx = inPtId;
        for ( cc =0; cc < 3; cc ++ )
          {
          foundpoint[cc] = point[cc];
          }
        }
      }
    if ( pointIdx >= 0 )
      {
      newPts = vtkPoints::New();
      vtkIdType id = newPts->InsertNextPoint(foundpoint);
      
      // Update ourselves and release memory
      //
      output->SetPoints(newPts);
      output->InsertNextCell( VTK_VERTEX, 1, &id);

      newPts->Delete();
      }
    }

  output->Squeeze();
  trans->Delete();
  pts->Delete();
}

void vtkPickDataSetFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

