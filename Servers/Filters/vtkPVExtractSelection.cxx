/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtractSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtractSelection.h"

#include "vtkAlgorithm.h"
#include "vtkCellData.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataSet.h"
#include "vtkExecutive.h"
#include "vtkExtractSelection.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProp.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSignedCharArray.h"
#include "vtkDoubleArray.h"
#include "vtkUnstructuredGrid.h"

#include "vtkSmartPointer.h"
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkPVExtractSelection, "1.1");
vtkStandardNewMacro(vtkPVExtractSelection);

vtkCxxSetObjectMacro(vtkPVExtractSelection, OutputSelection, vtkSelection);
vtkCxxSetObjectMacro(vtkPVExtractSelection, InputSelection, vtkSelection);

struct vtkPVExtractSelectionInternals
{
  vtkstd::vector<vtkSmartPointer<vtkProp> > Props;
  vtkstd::vector<vtkSmartPointer<vtkAlgorithm> > OriginalSources;
};

//----------------------------------------------------------------------------
vtkPVExtractSelection::vtkPVExtractSelection()
{
  this->selType = 0;
  this->AtomExtractor = vtkExtractSelection::New();

  // Make sure that the extractor filter has the right executive and
  // that it requests the right piece and number of pieces.
  vtkCompositeDataPipeline* cdp = vtkCompositeDataPipeline::New();
  //vtkDemandDrivenPipeline* cdp = vtkDemandDrivenPipeline::New();

  this->AtomExtractor->SetExecutive(cdp);
  vtkInformation* outInfo = 
    cdp->GetOutputInformation()->GetInformationObject(0);
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();
  cdp->SetUpdateNumberOfPieces(outInfo, processModule->GetNumberOfPartitions());
  cdp->SetUpdatePiece(outInfo, processModule->GetPartitionId());
  cdp->SetUpdateGhostLevel(outInfo, 0);
  cdp->Delete();
  this->Internal = new vtkPVExtractSelectionInternals;
  this->InputSelection = vtkSelection::New();
  this->OutputSelection = vtkSelection::New();

}

//----------------------------------------------------------------------------
vtkPVExtractSelection::~vtkPVExtractSelection()
{
  this->AtomExtractor->Delete();
  delete this->Internal;
  if (this->OutputSelection)
    {
    this->OutputSelection->Delete();
    }
  if (this->InputSelection)
    {
    this->InputSelection->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::Select()
{ 
  if (!this->InputSelection)
    {
    vtkErrorMacro("PV Extract Selection must be given parameters to select something.");
    return;
    }

  if (!this->OutputSelection)
    {
    this->OutputSelection = vtkSelection::New();
    }

  this->OutputSelection->Clear();
  this->OutputSelection->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::SELECTIONS);

  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();
  
  //run the cell/point extractor on each dataset
  unsigned int numDataSets = this->Internal->OriginalSources.size();
  for (unsigned int i = 0; i < numDataSets; i++)
    {
    vtkSelection* selection = NULL;
    vtkDataSet* ds = vtkDataSet::SafeDownCast(
      this->Internal->OriginalSources[i]->GetOutputDataObject(0));
    if (ds)
      {
      // We make a copy of the dataset so that we are not connected 
      // to the ParaView pipeline
      vtkDataSet* newDS = ds->NewInstance();
      newDS->ShallowCopy(ds);       
      this->AtomExtractor->SetInput(0,this->InputSelection);
      this->AtomExtractor->SetInput(1,newDS);
      newDS->Delete();
      this->AtomExtractor->Update();
      
      vtkDataSet* output = this->AtomExtractor->GetOutput();
      vtkIdTypeArray *origIds = vtkIdTypeArray::SafeDownCast(output->GetCellData()->GetArray("vtkOriginalCellIds"));
      if (origIds == NULL || origIds->GetNumberOfTuples() == 0)
        {
        this->AtomExtractor->SetInput((vtkDataSet*)0);
        continue;
        }
      
      // Create and add a selection node to our output
      selection = vtkSelection::New();
      this->OutputSelection->AddChild(selection);
        
      // For now, we only support cell ids
      selection->GetProperties()->Set(
        vtkSelection::CONTENT_TYPE(), vtkSelection::CELL_IDS);

      //copy over the ids we found into the new node
      selection->SetSelectionList(origIds);
        
      // Make sure that the input is released
      this->AtomExtractor->SetInput((vtkDataSet*)0);

      vtkClientServerID pid = processModule->GetIDFromObject(
        this->Internal->OriginalSources[i]);
      selection->GetProperties()->Set(
        vtkSelection::SOURCE_ID(), pid.ID);
      
      if (this->Internal->Props.size() > i)
        {
        vtkClientServerID pid = processModule->GetIDFromObject(
          this->Internal->Props[i]);
        selection->GetProperties()->Set(
          vtkSelection::PROP_ID(), pid.ID);
        }
      
      //Record the process id of the node that is running this code too
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
void vtkPVExtractSelection::ClearOriginalSources()
{
  this->Internal->OriginalSources.clear();
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::ClearProps()
{
  this->Internal->Props.clear();
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::Initialize()
{
  this->ClearProps();
  this->ClearOriginalSources();
  this->OutputSelection->Clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::AddOriginalSource(vtkAlgorithm* source)
{
  this->Internal->OriginalSources.push_back(source);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::AddProp(vtkProp* prop)
{
  this->Internal->Props.push_back(prop);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVExtractSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AtomExtractor: " << this->AtomExtractor << endl;
  os << indent << "InputSelection: ";
  if (this->InputSelection)
    {
    this->InputSelection->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "OutputSelection: ";
  if (this->OutputSelection)
    {
    this->OutputSelection->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

