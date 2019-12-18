/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPExtentTranslator.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPExtentTranslator.h"

#include "vtkImageData.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"

#include <cassert>
#include <vector>

class vtkPExtentTranslatorInternals
{
public:
  std::vector<int> AllProcessExtents;
};

vtkStandardNewMacro(vtkPExtentTranslator);
//----------------------------------------------------------------------------
vtkPExtentTranslator::vtkPExtentTranslator()
{
  this->Internals = new vtkPExtentTranslatorInternals;
}

//----------------------------------------------------------------------------
vtkPExtentTranslator::~vtkPExtentTranslator()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
int vtkPExtentTranslatorPieceToExtentThreadSafe(int* resultExtent, vtkDataSet* dataSet)
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
int vtkPExtentTranslator::PieceToExtentThreadSafe(int piece, int vtkNotUsed(numPieces),
  int vtkNotUsed(ghostLevel), int* vtkNotUsed(wholeExtent), int* resultExtent,
  int vtkNotUsed(splitMode), int vtkNotUsed(byPoints))
{
  assert(piece >= 0);

  assert(static_cast<int>(this->Internals->AllProcessExtents.size()) % 6 == 0);

  if (this->Internals->AllProcessExtents.size() >= 6)
  {
    if (static_cast<size_t>(piece * 6) >= this->Internals->AllProcessExtents.size())
    {
      vtkErrorMacro("Invalid piece.");
      return 0;
    }

    memcpy(resultExtent, &this->Internals->AllProcessExtents[piece * 6], sizeof(int) * 6);
    return 1;
  }

  vtkErrorMacro("GatherExtents must be called before this method");
  return 0;
}

//----------------------------------------------------------------------------
void vtkPExtentTranslator::GatherExtents(vtkDataSet* dataset)
{
  if (dataset == NULL)
  {
    this->Internals->AllProcessExtents.clear();
    return;
  }

  vtkMultiProcessController* controller = vtkMultiProcessController::GetGlobalController();

  int myExtent[6];
  vtkPExtentTranslatorPieceToExtentThreadSafe(myExtent, dataset);
  int myId = controller ? controller->GetLocalProcessId() : 0;
  int numProcs = controller ? controller->GetNumberOfProcesses() : 1;

  assert(numProcs >= 1);

  this->Internals->AllProcessExtents.resize(numProcs * 6);
  if (numProcs > 1)
  {
    controller->AllGather(myExtent, &(this->Internals->AllProcessExtents[0]), 6);
  }
  else
  {
    assert(myId == 0);
    (void)myId; // "use" myId in non-assert'ing builds.
    memcpy(&this->Internals->AllProcessExtents[0], myExtent, sizeof(int) * 6);
  }
}

//----------------------------------------------------------------------------
void vtkPExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Ranks: " << (this->Internals->AllProcessExtents.size() / 6) << endl;
  os << indent << "Extents: " << endl;
  for (size_t cc = 0; cc < this->Internals->AllProcessExtents.size(); cc += 6)
  {
    int* extents = &this->Internals->AllProcessExtents[cc];
    cout << indent.GetNextIndent() << "[" << extents[0] << ", " << extents[1] << ", " << extents[2]
         << ", " << extents[3] << ", " << extents[4] << ", " << extents[5] << "]" << endl;
  }
}
