/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtentTranslator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtentTranslator.h"

#include "vtkAlgorithm.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVExtentTranslator);

//vtkCxxSetObjectMacro(vtkPVExtentTranslator, OriginalSource, vtkDataSet);

//-----------------------------------------------------------------------------
vtkPVExtentTranslator::vtkPVExtentTranslator()
{
  this->OriginalSource = NULL;
  this->PortIndex = 0;
}

//-----------------------------------------------------------------------------
vtkPVExtentTranslator::~vtkPVExtentTranslator()
{
  this->SetOriginalSource(NULL);
}

//-----------------------------------------------------------------------------
void vtkPVExtentTranslator::SetOriginalSource(vtkAlgorithm *d)
{
  // No reference counting because of nasty loop. (TransmitPolyData ...)
  // Propagation should make it safe to ignore reference counting.
  this->OriginalSource = d;
} 


//-----------------------------------------------------------------------------
int vtkPVExtentTranslator::PieceToExtentThreadSafe(int piece, int numPieces, 
                                      int ghostLevel, int *wholeExtent, 
                                      int *resultExtent, int splitMode, 
                                      int byPoints)
{
  int ret;

  if (this->OriginalSource == NULL)
    {
    memcpy(resultExtent, wholeExtent, sizeof(int)*6);    
    }
  else
    {
    vtkInformation* info = 
      this->OriginalSource->GetExecutive()->GetOutputInformation(
      this->PortIndex);
    if (!info->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
      {
      memcpy(resultExtent, wholeExtent, sizeof(int)*6);    
      }
    else
      {
      info->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), resultExtent);
      }
    }

  if (byPoints)
    {
    ret = this->SplitExtentByPoints(piece, numPieces, resultExtent, splitMode);
    }
  else
    {
    ret = this->SplitExtent(piece, numPieces, resultExtent, splitMode);
    }
    
  if (ret == 0)
    {
    // Nothing in this piece.
    resultExtent[0] = resultExtent[2] = resultExtent[4] = 0;
    resultExtent[1] = resultExtent[3] = resultExtent[5] = -1;
    return 0;
    }

  if (ghostLevel > 0)
    {
    resultExtent[0] -= ghostLevel;
    resultExtent[1] += ghostLevel;
    resultExtent[2] -= ghostLevel;
    resultExtent[3] += ghostLevel;
    resultExtent[4] -= ghostLevel;
    resultExtent[5] += ghostLevel;
    }
    
  if (resultExtent[0] < wholeExtent[0])
    {
    resultExtent[0] = wholeExtent[0];
    }
  if (resultExtent[1] > wholeExtent[1])
    {
    resultExtent[1] = wholeExtent[1];
    }
  if (resultExtent[2] < wholeExtent[2])
    {
    resultExtent[2] = wholeExtent[2];
    }
  if (resultExtent[3] > wholeExtent[3])
    {
    resultExtent[3] = wholeExtent[3];
    }
  if (resultExtent[4] < wholeExtent[4])
    {
    resultExtent[4] = wholeExtent[4];
    }
  if (resultExtent[5] > wholeExtent[5])
    {
    resultExtent[5] = wholeExtent[5];
    }

  if (resultExtent[0] > resultExtent[1] ||
      resultExtent[2] > resultExtent[3] ||
      resultExtent[4] > resultExtent[5])
    {
    // Nothing in this piece.
    resultExtent[0] = resultExtent[2] = resultExtent[4] = 0;
    resultExtent[1] = resultExtent[3] = resultExtent[5] = -1;
    return 0;
    }
    
  return 1;
}



//-----------------------------------------------------------------------------
void vtkPVExtentTranslator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Original Source: " << this->OriginalSource << endl;
  os << indent << "PortIndex: " << this->PortIndex << endl;
}







