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

#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"

#include <vector>

class vtkPVTrivialExtentTranslatorInternals
{
public:
  std::vector<int> AllProcessExtents;
};

vtkStandardNewMacro(vtkPVTrivialExtentTranslator);
vtkCxxSetObjectMacro(vtkPVTrivialExtentTranslator, DataSet, vtkDataSet);

//----------------------------------------------------------------------------
vtkPVTrivialExtentTranslator::vtkPVTrivialExtentTranslator()
{
  this->DataSet = 0;
  this->Internals = new vtkPVTrivialExtentTranslatorInternals;
}

//----------------------------------------------------------------------------
vtkPVTrivialExtentTranslator::~vtkPVTrivialExtentTranslator()
{
  this->SetDataSet(0);
  if(this->Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkPVTrivialExtentTranslatorPieceToExtentThreadSafe(
  int *resultExtent, vtkDataSet* dataSet)
{
  // this is really only meant for topologically structured grids
  if (vtkImageData* id = vtkImageData::SafeDownCast(dataSet))
    {
    id->GetExtent(resultExtent);
    }
  else if (vtkStructuredGrid* sd = vtkStructuredGrid::SafeDownCast(dataSet))
    {
    sd->GetExtent(resultExtent);
    }
  else if (vtkRectilinearGrid* rd = vtkRectilinearGrid::SafeDownCast(dataSet))
    {
    rd->GetExtent(resultExtent);
    }
  else
    {
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkPVTrivialExtentTranslator::PieceToExtentThreadSafe(
  int piece, int vtkNotUsed(numPieces), int vtkNotUsed(ghostLevel), int *wholeExtent,
  int *resultExtent, int vtkNotUsed(splitMode), int vtkNotUsed(byPoints))
{
  if(this->Internals->AllProcessExtents.size() > 6)
    {
    if(static_cast<size_t>(piece*6) >=
       this->Internals->AllProcessExtents.size())
      {
      vtkErrorMacro("Invalid piece.");
      return 0;
      }
    memcpy(resultExtent, &(this->Internals->AllProcessExtents[piece*6]),
           sizeof(int)*6);
    return 1;
    }
  if(vtkPVTrivialExtentTranslatorPieceToExtentThreadSafe(resultExtent, this->DataSet) == 0)
    {
    memcpy(resultExtent, wholeExtent, sizeof(int)*6);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVTrivialExtentTranslator::GatherExtents()
{
  if(this->DataSet == NULL)
    {
    this->Internals->AllProcessExtents.clear();
    return;
    }
  if(vtkMultiProcessController* controller =
       vtkMultiProcessController::GetGlobalController())
    {
    int numProcs = controller->GetNumberOfProcesses();
    if(numProcs > 1)
      {
      int myExtent[6];
      if(vtkPVTrivialExtentTranslatorPieceToExtentThreadSafe(
           myExtent, this->DataSet) == 0)
        {
        this->Internals->AllProcessExtents.clear();
        return;
        }
      this->Internals->AllProcessExtents.resize(numProcs*6);
      controller->AllGather(
        myExtent, &(this->Internals->AllProcessExtents[0]), 6);
      return;
      }
    }
  this->Internals->AllProcessExtents.clear();
  return;
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
  if(this->DataSet)
    {
    os << indent << "DataSet: " << this->DataSet << "\n";
    }
  else
    {
    os << indent << "DataSet: (NULL)\n";
    }
}
