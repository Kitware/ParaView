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
#include "vtkRectilinearGrid.h"
#include "vtkCharArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkAppendPolyData.h"
#include "vtkPolyData.h"
#include "vtkContourFilter.h"
#include "vtkStringList.h"
#include "vtkTimerLog.h"
#include "vtkGarbageCollector.h"


vtkCxxRevisionMacro(vtkCTHAMRContour, "1.13");
vtkStandardNewMacro(vtkCTHAMRContour);

//----------------------------------------------------------------------------
vtkCTHAMRContour::vtkCTHAMRContour()
{
  this->ContourValues = vtkContourValues::New();

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkCTHAMRContour::~vtkCTHAMRContour()
{
  this->ContourValues->Delete();
  this->ContourValues = 0;
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
  vtkPolyData* output;
  vtkRectilinearGrid* block; 
  vtkAppendPolyData* append;
  vtkPolyData* tmp;
  vtkPolyData* contourOutput = 0;

  vtkGarbageCollector::DeferredCollectionPush();

  append = vtkAppendPolyData::New();
  block = vtkRectilinearGrid::New();
  if (contourOutput == 0)
    {
    vtkContourFilter* contour;
    contour = vtkContourFilter::New();  
    contour->SetInput(block);
    int numContours=this->ContourValues->GetNumberOfContours();
    double *values=this->ContourValues->GetValues();
    contour->SetNumberOfContours(numContours);
    for (int i=0; i < numContours; i++)
      {
      contour->SetValue(i,values[i]);
      }
    contour->SetInputArrayToProcess(0,this->GetInputArrayInformation(0));
    contourOutput = contour->GetOutput();
    contourOutput->Register(this);
    contour->Delete();
    contour = 0;
    }

  // Loops over all blocks.
  // It would be easier to loop over parts frist, but them we would
  // have to extract the blocks more than once. 
  numBlocks = input->GetNumberOfBlocks();
  for (blockId = 0; blockId < numBlocks; ++blockId)
    {
    double startProg = static_cast<double>(blockId)/static_cast<double>(numBlocks); 
    this->UpdateProgress(startProg);
    input->GetDualBlock(blockId, block);
    contourOutput->Update();
    tmp = vtkPolyData::New();
    tmp->ShallowCopy(contourOutput);
    append->AddInput(tmp);
    tmp->Delete();
    }

  output = this->GetOutput();
  if (append->GetNumberOfInputConnections(0) > 0)
    {
    append->Update();
    output->CopyStructure(append->GetOutput());
    output->GetPointData()->PassData(append->GetOutput()->GetPointData());
    output->GetCellData()->PassData(append->GetOutput()->GetCellData());
    output->GetFieldData()->PassData(input->GetFieldData());
    }
  append->Delete();
  append = 0;
  contourOutput->UnRegister(this );
  contourOutput = 0;
  block->Delete();
  block = 0;

  vtkGarbageCollector::DeferredCollectionPop();  
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
}




