/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPieceStreamingExtentTranslator.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPieceStreamingExtentTranslator);
//----------------------------------------------------------------------------
vtkPieceStreamingExtentTranslator::vtkPieceStreamingExtentTranslator()
{
  this->NumberOfPasses = 1;
}

//----------------------------------------------------------------------------
vtkPieceStreamingExtentTranslator::~vtkPieceStreamingExtentTranslator()
{
}

//----------------------------------------------------------------------------
int vtkPieceStreamingExtentTranslator::PassToRequest(
  int pass, vtkInformation* info)
{
  // convert pass request to piece request. We respect existing piece request,
  // if any.
  if (this->NumberOfPasses > 1 && pass >= 0 && pass < this->NumberOfPasses)
    {
    int piece = vtkStreamingDemandDrivenPipeline::GetUpdatePiece(info);
    int num_pieces =
      vtkStreamingDemandDrivenPipeline::GetUpdateNumberOfPieces(info);

    piece = piece * this->NumberOfPasses + pass;
    num_pieces = num_pieces * this->NumberOfPasses;

    vtkStreamingDemandDrivenPipeline::SetUpdatePiece(info, piece);
    vtkStreamingDemandDrivenPipeline::SetUpdateNumberOfPieces(info, num_pieces);
    cout << "Requesting Piece: " << piece << "/" << num_pieces << endl;
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPieceStreamingExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
