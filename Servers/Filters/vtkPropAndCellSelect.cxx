/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropAndCellSelect.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPropAndCellSelect.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkMultiGroupDataInformation.h"
#include "vtkCompositeDataSet.h"

#include "vtkSelectDataSets.h"
#include "vtkFrustumExtractor.h"

#include "vtkUnstructuredGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkDataSet.h"
#include "vtkDataObject.h"

#include "vtkCompositeDataPipeline.h"

vtkCxxRevisionMacro(vtkPropAndCellSelect, "1.1");
vtkStandardNewMacro(vtkPropAndCellSelect);

//----------------------------------------------------------------------------
vtkPropAndCellSelect::vtkPropAndCellSelect()
{
  //we are a source, and don't take any inputs in the pipeline
  this->SetNumberOfInputPorts(0);
  //we just use one output port...
  //...however that output will hold multiple datasets in a CompositeDataSet
  this->SetNumberOfOutputPorts(1);

  this->CompositeDataPipeline1 = vtkCompositeDataPipeline::New();
  this->PropPicker = vtkSelectDataSets::New();
  this->PropPicker->SetExecutive(this->CompositeDataPipeline1);
  this->AtomExtractor = vtkFrustumExtractor::New();

  this->SelectionType = 1;
}

//----------------------------------------------------------------------------
vtkPropAndCellSelect::~vtkPropAndCellSelect()
{
  this->AtomExtractor->Delete();
  this->PropPicker->Delete();
  this->CompositeDataPipeline1->Delete();
}

//----------------------------------------------------------------------------
int vtkPropAndCellSelect::FillOutputPortInformation(int p, vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_TYPE_NAME(), 
            "vtkMultiBlockDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPropAndCellSelect::RequestInformation(
  vtkInformation *i, 
  vtkInformationVector **iv, 
  vtkInformationVector *ov)
{
  vtkInformation* info = ov->GetInformationObject(0);

  //we are not set up for streaming
  info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), 1);
  
  //we produce composite output
  vtkMultiGroupDataInformation *compInfo = vtkMultiGroupDataInformation::New();

  int numBlocks = 0;
  int showBounds = this->AtomExtractor->GetShowBounds();
  if (showBounds)
    {
    //a single block for the selection area outline
    numBlocks = 1;
    }
  else
    {
    //one block for each dataset the proppicker found
    numBlocks = this->GetNumProps();
    }

  compInfo->SetNumberOfGroups(numBlocks);

  //each block contains one data set
  for (int i=0; i<numBlocks; i++)
    {
    compInfo->SetNumberOfDataSets(i, 1);
    }
  
  info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION(),compInfo);
  //cleanup
  compInfo->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int vtkPropAndCellSelect::RequestData(
  vtkInformation *r,
  vtkInformationVector **iv,
  vtkInformationVector *ov)
{ 
  int showBounds = this->AtomExtractor->GetShowBounds();

  // get a hold of my output dataobject to put the results into
  vtkInformation* outInfo = ov->GetInformationObject(0);
  vtkDataObject* dObjOut = 
    outInfo->Get(vtkCompositeDataSet::COMPOSITE_DATA_SET());
  vtkMultiBlockDataSet* output = 
    vtkMultiBlockDataSet::SafeDownCast(dObjOut);
  if (!output)
    {
    return 0;
    }
  
  //execute the dataset picker
  this->PropPicker->Update();
  vtkMultiBlockDataSet *propsSelected = this->PropPicker->GetOutput();

  if (this->GetShowBounds())
    {
    output->SetNumberOfBlocks(1);

    this->AtomExtractor->SetInput(propsSelected);
    this->AtomExtractor->Update();
    vtkDataSet* outline = this->AtomExtractor->GetOutput();
    vtkDataSet* copy = outline->NewInstance();
    copy->ShallowCopy(outline);
    output->SetDataSet(0, 0, copy);    
    copy->Delete();
    return 1;
    }

  //run the cell/point extractor on each selected dataset
  unsigned int numBlocks = propsSelected->GetNumberOfBlocks();
  output->SetNumberOfBlocks(numBlocks);

  for (unsigned int i = 0; i < numBlocks; i++)
    {
    unsigned int numDataSets = propsSelected->GetNumberOfDataSets(i);
    output->SetNumberOfDataSets(i, numDataSets);
    for (unsigned int j=0; j<numDataSets; j++)
      {
      vtkDataSet* dObj = vtkDataSet::SafeDownCast(
        propsSelected->GetDataSet(i, j));
      if (dObj)
        {
        if (this->SelectionType == 0)
          {
          output->SetDataSet(i, j, dObj);
          }
        else
          {
          this->AtomExtractor->SetInput(dObj);
          this->AtomExtractor->Update();
          vtkDataSet* ijOutput = this->AtomExtractor->GetOutput();
          vtkDataSet* copy = ijOutput->NewInstance();
          copy->ShallowCopy(ijOutput);
          output->SetDataSet(i, j, copy);
          copy->Delete();
          }
        }
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPropAndCellSelect::Initialize()
{
  this->PropPicker->Initialize();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPropAndCellSelect::AddDataSet(vtkObject *obj)
{
  this->PropPicker->AddDataSet(obj);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPropAndCellSelect::GetNumProps()
{
  return this->PropPicker->GetNumProps();
}

//----------------------------------------------------------------------------
void vtkPropAndCellSelect::CreateFrustum(double vertices[32])
{
  this->AtomExtractor->CreateFrustum(vertices);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPropAndCellSelect::SetShowBounds(int sb)
{ 
  this->AtomExtractor->SetShowBounds(sb);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPropAndCellSelect::GetShowBounds()
{
  return this->AtomExtractor->GetShowBounds();
}

//----------------------------------------------------------------------------
void vtkPropAndCellSelect::SetExactTest(int et)
{
  this->AtomExtractor->SetExactTest(et);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPropAndCellSelect::GetExactTest()
{
  return this->AtomExtractor->GetExactTest();
}

//----------------------------------------------------------------------------
void vtkPropAndCellSelect::SetPassThrough(int pt)
{
  this->AtomExtractor->SetPassThrough(pt);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPropAndCellSelect::GetPassThrough()
{
  return this->AtomExtractor->GetPassThrough();
}

//----------------------------------------------------------------------------
void vtkPropAndCellSelect::SetSelectionType(int st)
{ 
  if (st < 0) st = 0;
  if (st > 3) st = 3;
  this->SelectionType = st;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkPropAndCellSelect::GetSelectionType()
{
  return this->SelectionType;
}

//----------------------------------------------------------------------------
void vtkPropAndCellSelect::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PropPicker: " << this->PropPicker << endl;
  os << indent << "AtomExtractor: " << this->AtomExtractor << endl;
}

