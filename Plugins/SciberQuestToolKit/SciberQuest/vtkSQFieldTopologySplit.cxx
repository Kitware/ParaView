/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "vtkSQFieldTopologySplit.h"
#include "TopologicalClassSelector.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQFieldTopologySplit);

//-----------------------------------------------------------------------------
vtkSQFieldTopologySplit::vtkSQFieldTopologySplit()
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQFieldTopologySplit::vtkSQFieldTopologySplit" << std::endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(5);
}

//-----------------------------------------------------------------------------
vtkSQFieldTopologySplit::~vtkSQFieldTopologySplit()
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQFieldTopologySplit::~vtkSQFieldTopologySplit" << std::endl;
  #endif
}

//----------------------------------------------------------------------------
int vtkSQFieldTopologySplit::FillInputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQFieldTopologySplit::FillInputPortInformation" << std::endl;
  #endif

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),0);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQFieldTopologySplit::FillOutputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  #if SQTK_DEBUG>1
  pCerr() << "=====vtkSQFieldTopologySplit::FillOutputPortInformation" << std::endl;
  #endif

  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQFieldTopologySplit::RequestInformation(
    vtkInformation * /*req*/,
    vtkInformationVector ** /*inInfos*/,
    vtkInformationVector *vtkNotUsed(outInfos))
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQFieldTopologySplit::RequestInformation" << std::endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQFieldTopologySplit::RequestData(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector)
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQFieldTopologySplit::RequestData" << std::endl;
  #endif

  vtkInformation *inInfo=inputVector[0]->GetInformationObject(0);

  vtkDataSet *input
    = dynamic_cast<vtkDataSet*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (input==0)
    {
    vtkErrorMacro("Empty input.");
    return 1;
    }

  if ( (dynamic_cast<vtkPolyData*>(input)==0)
    && (dynamic_cast<vtkUnstructuredGrid*>(input)==0))
    {
    vtkErrorMacro("Input type " << input->GetClassName() << " is unsupported.");
    return 1;
    }

  vtkInformation *outInfo;
  vtkUnstructuredGrid *output;
  int pieceNo;
  int nPieces;
  int portNo=0;
  double progInc=0.2;

  //----------------------------------------
  //  class       value     definition
  //--------------------------------------
  //  solar wind  0         d-d
  //              3         0-d
  //              4         i-d

  // get the info object
  outInfo=outputVector->GetInformationObject(portNo);
  ++portNo;

  // get the ouptut
  output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (pieceNo>=nPieces)
    {
    // sanity - the requst cannot be fullfilled
    output->Initialize();
    }
  else
    {
    // create this class
    TopologicalClassSelector sel;
    sel.SetInput(input);
    sel.AppendRange(-0.5,0.5);
    sel.AppendRange(2.5,4.5);
    output->ShallowCopy(sel.GetOutput());
    }
  this->UpdateProgress(portNo*progInc);

  //--------------------------------------
  //  magnetos-   5         n-n
  //  phere       6         s-n
  //              9         s-s

  // get the info object
  outInfo=outputVector->GetInformationObject(portNo);
  ++portNo;

  // get the ouptut
  output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (pieceNo>=nPieces)
    {
    // sanity - the requst cannot be fullfilled
    output->Initialize();
    }
  else
    {
    // create this class
    TopologicalClassSelector sel;
    sel.SetInput(input);
    sel.AppendRange(4.5,6.5);
    sel.AppendRange(8.5,9.5);
    output->ShallowCopy(sel.GetOutput());
    }
  this->UpdateProgress(portNo*progInc);

  //--------------------------------------
  //  north       1         n-d
  //  connected   7         0-n
  //              8         i-n

  // get the info object
  outInfo=outputVector->GetInformationObject(portNo);
  ++portNo;

  // get the ouptut
  output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (pieceNo>=nPieces)
    {
    // sanity - the requst cannot be fullfilled
    output->Initialize();
    }
  else
    {
    // create this class
    TopologicalClassSelector sel;
    sel.SetInput(input);
    sel.AppendRange(0.5,1.5);
    sel.AppendRange(6.5,8.5);
    output->ShallowCopy(sel.GetOutput());
    }
  this->UpdateProgress(portNo*progInc);

  //--------------------------------------
  //  south       2         s-d
  //  connected   10        0-s
  //              11        i-s

  // get the info object
  outInfo=outputVector->GetInformationObject(portNo);
  ++portNo;

  // get the ouptut
  output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (pieceNo>=nPieces)
    {
    // sanity - the requst cannot be fullfilled
    output->Initialize();
    }
  else
    {
    // create this class
    TopologicalClassSelector sel;
    sel.SetInput(input);
    sel.AppendRange(1.5,2.5);
    sel.AppendRange(9.5,11.5);
    output->ShallowCopy(sel.GetOutput());
    }
  this->UpdateProgress(portNo*progInc);

  //-------------------------------------
  //  null/short  12        0-0
  //  integration 13        i-0
  //              14        i-i
  //---------------------------------------

  // get the info object
  outInfo=outputVector->GetInformationObject(portNo);
  ++portNo;

  // get the ouptut
  output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (pieceNo>=nPieces)
    {
    // sanity - the requst cannot be fullfilled
    output->Initialize();
    }
  else
    {
    // create this class
    TopologicalClassSelector sel;
    sel.SetInput(input);
    sel.AppendRange(11.5,14.5);
    output->ShallowCopy(sel.GetOutput());
    }
  this->UpdateProgress(portNo*progInc);

  return 1;
}


//-----------------------------------------------------------------------------
void vtkSQFieldTopologySplit::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
