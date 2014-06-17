/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
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
