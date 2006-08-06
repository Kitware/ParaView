/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeSelector.h"

#include "vtkAlgorithm.h"
#include "vtkCellData.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkFrustumExtractor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProp.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSignedCharArray.h"

#include "vtkSmartPointer.h"
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkVolumeSelector, "1.4");
vtkStandardNewMacro(vtkVolumeSelector);

vtkCxxSetObjectMacro(vtkVolumeSelector, Selection, vtkSelection);

struct vtkVolumeSelectorInternals
{
  vtkstd::vector<vtkSmartPointer<vtkDataObject> > DataSets;
  vtkstd::vector<vtkSmartPointer<vtkProp> > Props;
  vtkstd::vector<vtkSmartPointer<vtkAlgorithm> > OriginalSources;
};

//----------------------------------------------------------------------------
vtkVolumeSelector::vtkVolumeSelector()
{
  this->AtomExtractor = vtkFrustumExtractor::New();
  this->AtomExtractor->SetPassThrough(1);
  // Make sure that the extractor filter has the right executive and
  // that it requests the right piece and number of pieces.
  vtkCompositeDataPipeline* cdp = vtkCompositeDataPipeline::New();
  this->AtomExtractor->SetExecutive(cdp);
  vtkInformation* outInfo = 
    cdp->GetOutputInformation()->GetInformationObject(0);
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();
  cdp->SetUpdateNumberOfPieces(outInfo, processModule->GetNumberOfPartitions());
  cdp->SetUpdatePiece(outInfo, processModule->GetPartitionId());
  cdp->SetUpdateGhostLevel(outInfo, 0);
  cdp->Delete();
  this->Internal = new vtkVolumeSelectorInternals;
  this->Selection = vtkSelection::New();
}

//----------------------------------------------------------------------------
vtkVolumeSelector::~vtkVolumeSelector()
{
  this->AtomExtractor->Delete();
  delete this->Internal;
  if (this->Selection)
    {
    this->Selection->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkVolumeSelector::Select()
{ 
  if (!this->Selection)
    {
    this->Selection = vtkSelection::New();
    }

  this->Selection->Clear();
  this->Selection->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::SELECTIONS);

  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();

  //run the cell/point extractor on each selected dataset
  unsigned int numDataSets = this->Internal->DataSets.size();
  for (unsigned int i = 0; i < numDataSets; i++)
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(this->Internal->DataSets[i]);
    if (ds)
      {
      // First perform a selection with the dataset. This will use
      // the frustrum to mark selected cells.

      // We make a copy of the dataset so that we are not connected 
      // to the ParaView pipeline
      vtkDataSet* newDS = ds->NewInstance();
      newDS->ShallowCopy(ds);
      this->AtomExtractor->SetInput(newDS);
      newDS->Delete();
      this->AtomExtractor->Update();
      vtkDataSet* output = this->AtomExtractor->GetOutput();
      vtkSignedCharArray* insidedness = 
        vtkSignedCharArray::SafeDownCast(
          output->GetCellData()->GetArray("vtkInsidedness"));
      // If nothing is selected, no need to proceed
      if (!insidedness || insidedness->GetNumberOfTuples() == 0)
        {
        // Make sure that the input is released
        this->AtomExtractor->SetInput((vtkDataSet*)0);
        continue;
        }

      // Create and add a selection node
      vtkSelection* selection = vtkSelection::New();
      this->Selection->AddChild(selection);

      // For now, we only support cell ids
      selection->GetProperties()->Set(
        vtkSelection::CONTENT_TYPE(), vtkSelection::CELL_IDS);

      // Selected cell array
      vtkIdTypeArray* selectedCells = vtkIdTypeArray::New();
      vtkIdType numCells = insidedness->GetNumberOfTuples();
      for (vtkIdType ii=0; ii<numCells; ii++)
        {
        if (insidedness->GetValue(ii) == 1)
          {
          selectedCells->InsertNextValue(ii);
          }
        }
      selection->SetSelectionList(selectedCells);
      selectedCells->Delete();

      // Make sure that the input is released
      this->AtomExtractor->SetInput((vtkDataSet*)0);

      // Find the source of the geometry and add it's id as a
      // property
      vtkInformation* pInfo = ds->GetPipelineInformation();
      if (pInfo)
        {
        vtkExecutive* pExec = pInfo->GetExecutive(vtkExecutive::PRODUCER());
        if (pExec)
          {
          vtkAlgorithm* alg = pExec->GetAlgorithm();
          if (alg)
            {
            vtkClientServerID id = processModule->GetIDFromObject(alg);
            selection->GetProperties()->Set(
              vtkSelection::SOURCE_ID(), id.ID);
            }
          }
        }
      
      if (this->Internal->OriginalSources.size() > i)
        {
        vtkClientServerID pid = processModule->GetIDFromObject(
          this->Internal->OriginalSources[i]);
        selection->GetProperties()->Set(
          vtkSelectionSerializer::ORIGINAL_SOURCE_ID(), pid.ID);
        }

      if (this->Internal->Props.size() > i)
        {
        vtkClientServerID pid = processModule->GetIDFromObject(
          this->Internal->Props[i]);
        selection->GetProperties()->Set(
          vtkSelection::PROP_ID(), pid.ID);
        }

      // Add the process id as another property
      if (processModule->GetPartitionId() >= 0)
        {
        selection->GetProperties()->Set(
          vtkSelection::PROCESS_ID(), processModule->GetPartitionId());
        }

      selection->Delete();
      }
    }
}

//----------------------------------------------------------------------------
void vtkVolumeSelector::ClearOriginalSources()
{
  this->Internal->OriginalSources.clear();
}

//----------------------------------------------------------------------------
void vtkVolumeSelector::ClearDataSets()
{
  this->Internal->DataSets.clear();
}

//----------------------------------------------------------------------------
void vtkVolumeSelector::ClearProps()
{
  this->Internal->Props.clear();
}

//----------------------------------------------------------------------------
void vtkVolumeSelector::Initialize()
{
  this->ClearProps();
  this->ClearDataSets();
  this->ClearOriginalSources();
  this->Selection->Clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkVolumeSelector::AddOriginalSource(vtkAlgorithm* source)
{
  this->Internal->OriginalSources.push_back(source);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkVolumeSelector::AddProp(vtkProp* prop)
{
  this->Internal->Props.push_back(prop);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkVolumeSelector::AddDataSet(vtkAlgorithm* source)
{
  vtkDataObject *dObj = NULL;
  if (source)
    {
    dObj = source->GetOutputDataObject(0);
    if (!dObj)
      {
      vtkErrorMacro("Could not find algorithm's vtkDataObject output.");
      return;
      }
    }

  this->Internal->DataSets.push_back(dObj);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkVolumeSelector::CreateFrustum(double vertices[32])
{
  this->AtomExtractor->CreateFrustum(vertices);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkVolumeSelector::SetExactTest(int et)
{
  this->AtomExtractor->SetExactTest(et);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkVolumeSelector::GetExactTest()
{
  return this->AtomExtractor->GetExactTest();
}

//----------------------------------------------------------------------------
void vtkVolumeSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AtomExtractor: " << this->AtomExtractor << endl;
  os << indent << "Selection: ";
  if (this->Selection)
    {
    this->Selection->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

