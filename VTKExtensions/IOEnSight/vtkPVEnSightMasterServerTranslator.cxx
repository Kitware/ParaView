// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVEnSightMasterServerTranslator.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVEnSightMasterServerTranslator);

//----------------------------------------------------------------------------
vtkPVEnSightMasterServerTranslator::vtkPVEnSightMasterServerTranslator()
{
  this->ProcessId = 0;
}

//----------------------------------------------------------------------------
vtkPVEnSightMasterServerTranslator::~vtkPVEnSightMasterServerTranslator() = default;

//----------------------------------------------------------------------------
void vtkPVEnSightMasterServerTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ProcessId: " << this->ProcessId << "\n";
}

//----------------------------------------------------------------------------
int vtkPVEnSightMasterServerTranslator::PieceToExtentThreadSafe(int vtkNotUsed(piece),
  int vtkNotUsed(numPieces), int vtkNotUsed(ghostLevel), int* wholeExtent, int* resultExtent,
  int splitMode, int byPoints)
{
  if (this->Piece == this->ProcessId)
  {
    // Return whole extent.
    return this->Superclass::PieceToExtentThreadSafe(
      0, 1, 0, wholeExtent, resultExtent, splitMode, byPoints);
  }
  else
  {
    // Return empty extent.
    int extent[6] = { 0, -1, 0, -1, 0, -1 };
    return this->Superclass::PieceToExtentThreadSafe(
      0, 1, 0, extent, resultExtent, splitMode, byPoints);
  }
}
