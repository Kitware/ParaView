/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHAMRContour.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHAMRContour.h"
#include "vtkCTHData.h"
#include "vtkObjectFactory.h"

#include "vtkToolkits.h"
#include "vtkImageData.h"
#include "vtkCharArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkOutlineFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkPolyData.h"
#include "vtkCTHAMRCellToPointData.h"
#ifdef VTK_USE_PATENTED
#  include "vtkKitwareContourFilter.h"
#  include "vtkSynchronizedTemplates3D.h"
#  include "vtkSynchronizedTemplates2D.h"
#else
#  include "vtkContourFilter.h"
#endif
#include "vtkStringList.h"
#include "vtkPlane.h"
#include "vtkIdList.h"
#include "vtkTimerLog.h"
#include "vtkGarbageCollector.h"


vtkCxxRevisionMacro(vtkCTHAMRContour, "1.10");
vtkStandardNewMacro(vtkCTHAMRContour);

//----------------------------------------------------------------------------
vtkCTHAMRContour::vtkCTHAMRContour()
{
  this->ContourValues = vtkContourValues::New();
  this->InputScalarsSelection = 0;
  
  this->Image = 0;
  this->PolyData = 0;

  this->IgnoreGhostLevels = 0;
}

//----------------------------------------------------------------------------
vtkCTHAMRContour::~vtkCTHAMRContour()
{
  this->SetInputScalarsSelection(0);
  this->ContourValues->Delete();
  this->ContourValues = 0;

  // Not really necessary because exeucte cleans up.
  this->DeleteInternalPipeline();
}

//------------------------------------------------------------------------------
unsigned long vtkCTHAMRContour::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if (this->ContourValues)
    {
    time = this->ContourValues->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkCTHAMRContour::Execute()
{
  int blockId, numBlocks;
  vtkCTHData* input = this->GetInput();
  vtkCTHData* inputCopy = vtkCTHData::New();
  vtkPolyData* output;
  vtkImageData* block; 
  vtkAppendPolyData* tmp;

  vtkGarbageCollector::DeferredCollectionPush();

  this->CreateInternalPipeline();
  inputCopy->ShallowCopy(input);

  vtkTimerLog::MarkStartEvent("CellToPoint");
  // Convert cell data to point data.
  vtkCTHAMRCellToPointData* cellToPoint = vtkCTHAMRCellToPointData::New();
  cellToPoint->SetInput(inputCopy);
  cellToPoint->AddVolumeArrayName(this->InputScalarsSelection);
  cellToPoint->SetIgnoreGhostLevels(this->IgnoreGhostLevels);
  cellToPoint->Update();
  inputCopy->ShallowCopy(cellToPoint->GetOutput());
  cellToPoint->Delete();    
  vtkTimerLog::MarkEndEvent("CellToPoint");

  // Create an append for each part (one part per output).
  tmp = vtkAppendPolyData::New();

  this->UpdateProgress(.05);

  // Loops over all blocks.
  // It would be easier to loop over parts frist, but them we would
  // have to extract the blocks more than once. 
  numBlocks = inputCopy->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    double startProg = .05 + .9 * static_cast<double>(blockId)/static_cast<double>(numBlocks); 
    this->UpdateProgress(startProg);
    block = vtkImageData::New();
    inputCopy->GetBlock(blockId, block);
    this->ExecuteBlock(block, tmp);
    block->Initialize();
    block->Delete();
    block = NULL;
    }
  
  this->UpdateProgress(.95);
  output = this->GetOutput();
 
  if (tmp->GetNumberOfInputConnections(0) > 0)
    {
    vtkTimerLog::MarkStartEvent("BlockAppend");               
    tmp->Update();
    vtkTimerLog::MarkEndEvent("BlockAppend");                  
    output->CopyStructure(tmp->GetOutput());
    output->GetPointData()->PassData(tmp->GetOutput()->GetPointData());
    output->GetCellData()->PassData(tmp->GetOutput()->GetCellData());
    output->GetFieldData()->PassData(input->GetFieldData());
    }
  tmp->Delete();
  tmp = 0;

  // Add a name for this part.
  vtkCharArray *nameArray = vtkCharArray::New();
  nameArray->SetName("Name");
  const char* arrayName = this->InputScalarsSelection;
  char *str = nameArray->WritePointer(0, (int)(strlen(arrayName))+1);
  sprintf(str, "%s", arrayName);
  output->GetFieldData()->AddArray(nameArray);
  nameArray->Delete();

  inputCopy->Delete();
  inputCopy = NULL;
  this->DeleteInternalPipeline();
  vtkGarbageCollector::DeferredCollectionPop();  
}

//-----------------------------------------------------------------------------
void vtkCTHAMRContour::ExecuteBlock(vtkImageData* block, 
                                    vtkAppendPolyData* tmp)
{
  vtkFloatArray* pointVolumeFraction;
  const char* arrayName;

  arrayName = this->InputScalarsSelection;
  pointVolumeFraction = (vtkFloatArray*)(block->GetPointData()->GetArray(arrayName));
  if (pointVolumeFraction == NULL)
    {
    vtkErrorMacro("Could not find point array.");
    } 
  
  arrayName = this->InputScalarsSelection;
  this->ExecutePart(arrayName, block, tmp);
}


//------------------------------------------------------------------------------
void vtkCTHAMRContour::CreateInternalPipeline()
{
  vtkCTHData* input = this->GetInput();

  // Having inputs keeps us from having to set and remove inputs.
  // The garbage collecting associated with this is expensive.
  this->Image = vtkImageData::New();
  
  // Note: I had trouble with the kitware contour filter setting the input
  // of the internal synchronized templates filter (garbage collection took too long.)
  
  // Create the contour surface.
#ifdef VTK_USE_PATENTED
  if (input->GetDataDimension() == 3)
    {
    vtkSynchronizedTemplates3D* tmp = vtkSynchronizedTemplates3D::New();
    // vtkDataSetSurfaceFilter does not generate normals, so they will be lost.
    tmp->ComputeNormalsOff();
    tmp->SetInput(this->Image);
    // Copy the contour values to the internal filter.
    int numContours=this->ContourValues->GetNumberOfContours();
    double *values=this->ContourValues->GetValues();
    tmp->SetNumberOfContours(numContours);
    for (int i=0; i < numContours; i++)
      {
      tmp->SetValue(i,values[i]);
      }
    this->PolyData = tmp->GetOutput();
    this->PolyData->Register(this);
    tmp->Delete();
    }
  else
    {
    vtkSynchronizedTemplates2D* tmp = vtkSynchronizedTemplates2D::New();
    tmp->SetInput(this->Image);
    // Copy the contour values to the internal filter.
    int numContours=this->ContourValues->GetNumberOfContours();
    double *values=this->ContourValues->GetValues();
    tmp->SetNumberOfContours(numContours);
    for (int i=0; i < numContours; i++)
      {
      tmp->SetValue(i,values[i]);
      }
    this->PolyData = tmp->GetOutput();
    this->PolyData->Register(this);
    tmp->Delete();
    }
#else
  vtkContourFilter* tmp = vtkContourFilter::New();
  tmp->SetInput(this->Image);
  // Copy the contour values to the internal filter.
  int numContours=this->ContourValues->GetNumberOfContours();
  double *values=this->ContourValues->GetValues();
  tmp->SetNumberOfContours(numContours);
  for (int i=0; i < numContours; i++)
    {
    tmp->SetValue(i,values[i]);
    }
    this->PolyData = tmp->GetOutput();
    this->PolyData->Register(this);
    tmp->Delete();
#endif
  
}

//------------------------------------------------------------------------------
void vtkCTHAMRContour::DeleteInternalPipeline()
{
  if (this->Image)
    {
    this->Image->Delete();
    this->Image = 0;
    }

  if (this->PolyData)
    {
    this->PolyData->UnRegister(this);
    this->PolyData = 0;
    }
}

//------------------------------------------------------------------------------
void vtkCTHAMRContour::ExecutePart(const char* arrayName,
                                   vtkImageData* block, 
                                   vtkAppendPolyData* append)
{
  // See if we can skip this block.
  vtkDataArray* array = block->GetPointData()->GetArray(arrayName);
  if (array == 0)
    {
    return;
    }

  // Skip blocks that do not have contour surface in them.
  int idx;
  double *range = array->GetRange();
  int blockHasContourValue = 0;
  double *values=this->ContourValues->GetValues();
  int numContours = this->ContourValues->GetNumberOfContours();
  for (idx = 0; idx < numContours && ! blockHasContourValue; ++idx)
    {
    if (range[0] <= values[idx] || values[idx] <= range[1])
      {
      blockHasContourValue = 1;
      }
    }
  if ( ! blockHasContourValue)
    {
    return;
    }
  
  block->GetPointData()->SetActiveScalars(arrayName);

  this->Image->ShallowCopy(block);
  this->PolyData->Update();
  vtkPolyData* tmp = vtkPolyData::New();
  tmp->ShallowCopy(this->PolyData);
  append->AddInput(tmp);
  tmp->Delete();
}

//-----------------------------------------------------------------------------
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
void vtkCTHAMRContour::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i,value);
}

//-----------------------------------------------------------------------------
// Get the ith contour value.
double vtkCTHAMRContour::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

//-----------------------------------------------------------------------------
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
double *vtkCTHAMRContour::GetValues()
{
  return this->ContourValues->GetValues();
}

//-----------------------------------------------------------------------------
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
void vtkCTHAMRContour::GetValues(double *contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

//-----------------------------------------------------------------------------
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
void vtkCTHAMRContour::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

//-----------------------------------------------------------------------------
// Get the number of contours in the list of contour values.
int vtkCTHAMRContour::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

//-----------------------------------------------------------------------------
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void vtkCTHAMRContour::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

//-----------------------------------------------------------------------------
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
void vtkCTHAMRContour::GenerateValues(int numContours, double
                                             rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

//-----------------------------------------------------------------------------
void vtkCTHAMRContour::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  this->ContourValues->PrintSelf(os,indent.GetNextIndent());
  os << indent << "IgnoreGhostLevels: " << this->IgnoreGhostLevels << endl;

  if (this->InputScalarsSelection)
    {
    os << indent << "InputScalarsSelection: " 
       << this->InputScalarsSelection << endl;
    }
}




