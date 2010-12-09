/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVTrivialExtentTranslator.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTrivialExtentTranslator.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkGarbageCollector.h"

vtkStandardNewMacro(vtkPVTrivialExtentTranslator);
vtkCxxSetObjectMacro(vtkPVTrivialExtentTranslator, DataSet, vtkDataSet);

//----------------------------------------------------------------------------
vtkPVTrivialExtentTranslator::vtkPVTrivialExtentTranslator()
{
  this->DataSet = 0;
}

//----------------------------------------------------------------------------
vtkPVTrivialExtentTranslator::~vtkPVTrivialExtentTranslator()
{
  this->SetDataSet(0);
}

//----------------------------------------------------------------------------
template <class T>
int vtkPVTrivialExtentTranslatorPieceToExtentThreadSafe(
  int *resultExtent,
  T* dataSet)
{
  memcpy(resultExtent, dataSet->GetExtent(), sizeof(int)*6);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkPVTrivialExtentTranslator::PieceToExtentThreadSafe(
      int vtkNotUsed(piece), int vtkNotUsed(numPieces),
      int vtkNotUsed(ghostLevel), int *wholeExtent,
      int *resultExtent, int vtkNotUsed(splitMode),
      int vtkNotUsed(byPoints))
{
  if (vtkImageData* id = vtkImageData::SafeDownCast(this->DataSet))
    {
    return vtkPVTrivialExtentTranslatorPieceToExtentThreadSafe(
      resultExtent, id);
    }
  else if (vtkStructuredGrid* sd = vtkStructuredGrid::SafeDownCast(this->DataSet))
    {
    return vtkPVTrivialExtentTranslatorPieceToExtentThreadSafe(
      resultExtent, sd);
    }
  else if (vtkRectilinearGrid* rd = vtkRectilinearGrid::SafeDownCast(this->DataSet))
    {
    return vtkPVTrivialExtentTranslatorPieceToExtentThreadSafe(
      resultExtent, rd);
    }

  memcpy(resultExtent, wholeExtent, sizeof(int)*6);
  return 1;
}
//----------------------------------------------------------------------------
void vtkPVTrivialExtentTranslator::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->DataSet, "DataSet");
}

//----------------------------------------------------------------------------
void vtkPVTrivialExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
