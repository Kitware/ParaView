/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQFieldTopologySelect.h"
#include "TopologicalClassSelector.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkUnstructuredGrid.h"
#include "vtkPolyData.h"


#define SetClassSelectionMacro(CID)\
  void vtkSQFieldTopologySelect::SetSelect##CID(int v)\
    {\
    if (this->ClassSelection[CID]==v) return;\
    this->ClassSelection[CID]=v;\
    this->Modified();\
    }
#define GetClassSelectionMacro(CID)\
  int vtkSQFieldTopologySelect::GetSelect##CID(){ return this->ClassSelection[CID]; }

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQFieldTopologySelect);

//-----------------------------------------------------------------------------
vtkSQFieldTopologySelect::vtkSQFieldTopologySelect()
{
  #ifdef SQTK_DEBUG
  std::cerr << "===============================vtkSQFieldTopologySelect::vtkSQFieldTopologySelect" << std::endl;
  #endif

  // select all by default.
  for (int classId=0; classId<15; ++classId)
    {
    this->ClassSelection[classId]=1;
    }

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQFieldTopologySelect::~vtkSQFieldTopologySelect()
{
  #ifdef SQTK_DEBUG
  std::cerr << "===============================vtkSQFieldTopologySelect::~vtkSQFieldTopologySelect" << std::endl;
  #endif
}

//----------------------------------------------------------------------------
int vtkSQFieldTopologySelect::FillInputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  #ifdef SQTK_DEBUG
  std::cerr << "===============================vtkSQFieldTopologySelect::FillInputPortInformation" << std::endl;
  #endif

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),0);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQFieldTopologySelect::FillOutputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  #if SQTK_DEBUG>1
  pCerr() << "===============================vtkSQFieldTopologySelect::FillOutputPortInformation" << std::endl;
  #endif

  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQFieldTopologySelect::RequestInformation(
    vtkInformation * /*req*/,
    vtkInformationVector ** /*inInfos*/,
    vtkInformationVector *vtkNotUsed(outInfos))
{
  #ifdef SQTK_DEBUG
  std::cerr << "===============================vtkSQFieldTopologySelect::RequestInformation" << std::endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQFieldTopologySelect::RequestData(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector)
{
  #ifdef SQTK_DEBUG
  std::cerr << "===============================vtkSQFieldTopologySelect::RequestData" << std::endl;
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

  // get the info object
  vtkInformation *outInfo=outputVector->GetInformationObject(0);

  // get the ouptut
  vtkUnstructuredGrid *output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (pieceNo>=nPieces)
    {
    // sanity - the requst cannot be fullfilled
    output->Initialize();
    return 1;
    }


  double progInc=0.0667;
  double prog=0.0;

  TopologicalClassSelector sel;
  sel.SetInput(input);

  for (int classId=0; classId<15; ++classId)
    {
    if (this->ClassSelection[classId])
      {
      double lo=classId-0.5;
      double hi=classId+0.5;
      sel.AppendRange(lo,hi);
      }
    prog+=progInc;
    this->UpdateProgress(prog);
    }

  output->ShallowCopy(sel.GetOutput());

  return 1;
}

SetClassSelectionMacro(DD);
GetClassSelectionMacro(DD);
SetClassSelectionMacro(ND);
GetClassSelectionMacro(ND);
SetClassSelectionMacro(SD);
GetClassSelectionMacro(SD);
SetClassSelectionMacro(OD);
GetClassSelectionMacro(OD);
SetClassSelectionMacro(ID);
GetClassSelectionMacro(ID);
SetClassSelectionMacro(NN);
GetClassSelectionMacro(NN);
SetClassSelectionMacro(SN);
GetClassSelectionMacro(SN);
SetClassSelectionMacro(ON);
GetClassSelectionMacro(ON);
SetClassSelectionMacro(IN);
GetClassSelectionMacro(IN);
SetClassSelectionMacro(SS);
GetClassSelectionMacro(SS);
SetClassSelectionMacro(OS);
GetClassSelectionMacro(OS);
SetClassSelectionMacro(IS);
GetClassSelectionMacro(IS);
SetClassSelectionMacro(OO);
GetClassSelectionMacro(OO);
SetClassSelectionMacro(IO);
GetClassSelectionMacro(IO);
SetClassSelectionMacro(II);
GetClassSelectionMacro(II);

//-----------------------------------------------------------------------------
void vtkSQFieldTopologySelect::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
