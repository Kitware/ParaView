/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataSet.cxx
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
#include "vtkCompositeDataSet.h"

#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataVisitor.h"
#include "vtkDataSet.h"

vtkCxxRevisionMacro(vtkCompositeDataSet, "1.1");

//----------------------------------------------------------------------------
vtkCompositeDataSet::vtkCompositeDataSet()
{
  this->Bounds[0] = VTK_LARGE_FLOAT;
  this->Bounds[1] = -VTK_LARGE_FLOAT;
  this->Bounds[2] = VTK_LARGE_FLOAT;
  this->Bounds[3] = -VTK_LARGE_FLOAT;
  this->Bounds[4] = VTK_LARGE_FLOAT;
  this->Bounds[5] = -VTK_LARGE_FLOAT;

  this->NumberOfPoints = 0;
  this->NumberOfCells  = 0;
}

//----------------------------------------------------------------------------
vtkCompositeDataSet::~vtkCompositeDataSet()
{
}

//----------------------------------------------------------------------------
vtkIdType vtkCompositeDataSet::GetNumberOfPoints()
{
  this->ComputeNumberOfPoints();
  return this->NumberOfPoints;
}

//----------------------------------------------------------------------------
vtkIdType vtkCompositeDataSet::GetNumberOfCells()
{
  this->ComputeNumberOfCells();
  return this->NumberOfCells;
}

//----------------------------------------------------------------------------
double* vtkCompositeDataSet::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

//----------------------------------------------------------------------------
// Compute the data bounding box from data points.
void vtkCompositeDataSet::ComputeBounds()
{
  int j;

  if ( this->GetMTime() > this->ComputeBoundsTime )
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] =  VTK_LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;

    vtkCompositeDataIterator* iter = this->NewIterator();
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
      {
      vtkDataObject* dobj = iter->GetCurrentDataObject();
      vtkDataSet* curDataSet = vtkDataSet::SafeDownCast(dobj);
      if (curDataSet)
        {
        float* bounds = curDataSet->GetBounds();
        for (j=0; j<3; j++)
          {
          if ( bounds[2*j] < this->Bounds[2*j] )
            {
            this->Bounds[2*j] = bounds[2*j];
            }
          if ( bounds[j] > this->Bounds[2*j+1] )
            {
            this->Bounds[2*j+1] = bounds[2*j+1];
            }
          }
        }
      else
        {
        vtkErrorMacro( << "Unexptected data object type: " << dobj->GetClassName() );
        }
      iter->GoToNextItem();
      }
    iter->Delete();

    this->ComputeBoundsTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::ComputeNumberOfPoints()
{
  if ( this->GetMTime() > this->ComputeNPtsTime )
    {
    this->NumberOfPoints = 0;
    vtkCompositeDataIterator* iter = this->NewIterator();
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
      {
      vtkDataObject* dobj = iter->GetCurrentDataObject();
      vtkDataSet* curDataSet = vtkDataSet::SafeDownCast(dobj);
      if (curDataSet)
        {
        this->NumberOfPoints += curDataSet->GetNumberOfPoints();
        }
      else
        {
        vtkErrorMacro( << "Unexptected data object type: " << dobj->GetClassName() );
        }
      iter->GoToNextItem();
      }
    iter->Delete();

    this->ComputeNPtsTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::ComputeNumberOfCells()
{
  if ( this->GetMTime() > this->ComputeNCellsTime )
    {
    this->NumberOfPoints = 0;
    vtkCompositeDataIterator* iter = this->NewIterator();
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
      {
      vtkDataObject* dobj = iter->GetCurrentDataObject();
      vtkDataSet* curDataSet = vtkDataSet::SafeDownCast(dobj);
      if (curDataSet)
        {
        this->NumberOfCells += curDataSet->GetNumberOfCells();
        }
      else
        {
        vtkErrorMacro( << "Unexptected data object type: " << dobj->GetClassName() );
        }
      iter->GoToNextItem();
      }
    iter->Delete();

    this->ComputeNCellsTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::Initialize()
{
  this->Superclass::Initialize();
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::SetUpdateExtent(int piece, int numPieces, int ghostLevel)
{
  this->UpdatePiece = piece;
  this->UpdateNumberOfPieces = numPieces;
  this->UpdateGhostLevel = ghostLevel;
  this->UpdateExtentInitialized = 1;
}

//----------------------------------------------------------------------------
void vtkCompositeDataSet::GetUpdateExtent(int &piece, int &numPieces, int &ghostLevel)
{
  piece = this->UpdatePiece;
  numPieces = this->UpdateNumberOfPieces;
  ghostLevel = this->UpdateGhostLevel;
}


//----------------------------------------------------------------------------
void vtkCompositeDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

