/*=========================================================================

  Program:   ParaView
  Module:    vtkExtractCTHPart2.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractCTHPart2.h"

#include "vtkToolkits.h"
#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkClipPolyData.h"
#include "vtkContourFilter.h"
#include "vtkCutter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkFloatArray.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStringList.h"
#include "vtkCharArray.h"
#include "vtkFloatArray.h"
#include "vtkTimerLog.h"

#ifdef VTK_USE_PATENTED
#include "vtkKitwareContourFilter.h"
#include "vtkKitwareCutter.h"
#endif

#include <math.h>

vtkCxxRevisionMacro(vtkExtractCTHPart2, "1.2");
vtkStandardNewMacro(vtkExtractCTHPart2);
vtkCxxSetObjectMacro(vtkExtractCTHPart2,ClipPlane,vtkPlane);

//------------------------------------------------------------------------------
vtkExtractCTHPart2::vtkExtractCTHPart2()
{
  this->VolumeArrayNames = vtkStringList::New();
  this->Clipping = 0;
  this->ClipPlane = vtkPlane::New();
  // For consistent references.
  this->ClipPlane->Register(this);
  this->ClipPlane->Delete();
}

//------------------------------------------------------------------------------
vtkExtractCTHPart2::~vtkExtractCTHPart2()
{
  this->VolumeArrayNames->Delete();
  this->VolumeArrayNames = NULL;
  this->SetClipPlane(NULL);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If clip plane is modified,
// then this object is modified as well.
unsigned long vtkExtractCTHPart2::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if (this->ClipPlane)
    {
    time = this->ClipPlane->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}



//--------------------------------------------------------------------------
void vtkExtractCTHPart2::RemoveAllVolumeArrayNames()
{
  int num, idx;

  num = this->GetNumberOfOutputs();
  for (idx = 0; idx < num; ++idx)
    {
    this->SetOutput(idx, NULL);
    }

  this->VolumeArrayNames->RemoveAllItems();  
}

//--------------------------------------------------------------------------
void vtkExtractCTHPart2::AddVolumeArrayName(char* arrayName)
{
  vtkPolyData* d = vtkPolyData::New();
  int num = this->GetNumberOfOutputs();
  this->VolumeArrayNames->AddString(arrayName);
  this->SetOutput(num, d);
  d->Delete();
  d = NULL;
}

//--------------------------------------------------------------------------
int vtkExtractCTHPart2::GetNumberOfVolumeArrayNames()
{
  return this->VolumeArrayNames->GetNumberOfStrings();
}

//--------------------------------------------------------------------------
const char* vtkExtractCTHPart2::GetVolumeArrayName(int idx)
{
  return this->VolumeArrayNames->GetString(idx);
}

//--------------------------------------------------------------------------
void vtkExtractCTHPart2::SetOutput(int idx, vtkPolyData* d)
{
  this->vtkSource::SetNthOutput(idx, d);  
}

//----------------------------------------------------------------------------
vtkPolyData* vtkExtractCTHPart2::GetOutput(int idx)
{
  return (vtkPolyData *) this->vtkSource::GetOutput(idx); 
}

//----------------------------------------------------------------------------
int vtkExtractCTHPart2::GetNumberOfOutputs()
{
  return this->VolumeArrayNames->GetNumberOfStrings();
}


//--------------------------------------------------------------------------
void vtkExtractCTHPart2::ComputeInputUpdateExtents(vtkDataObject *output)
{
  vtkDataSet *input = this->GetInput();
  int piece = output->GetUpdatePiece();
  int numPieces = output->GetUpdateNumberOfPieces();
  int ghostLevel = output->GetUpdateGhostLevel();

  if (input == NULL)
    {
    return;
    }

  input->SetUpdateExtent(piece, numPieces, ghostLevel+1);
  // Force input to clip.
  // We could deal with larger extents if we create a 
  // rectilinear grid synchronized templates.
  input->RequestExactExtentOn();
}


//------------------------------------------------------------------------------
void vtkExtractCTHPart2::Execute()
{
  int idx, num;
  int idx2, numPts;
  const char* arrayName;
  vtkPolyData* output;


  num = this->VolumeArrayNames->GetNumberOfStrings();
  for (idx = 0; idx < num; ++idx)
    {
    arrayName = this->VolumeArrayNames->GetString(idx);
    output = this->GetOutput(idx);
    this->ExecutePart(arrayName, output);

    // In the future we might be able to select the rgb color here.
    if (num > 1)
      {
      // Add scalars to color this part.
      numPts = output->GetNumberOfPoints();
      vtkFloatArray *partArray = vtkFloatArray::New();
      partArray->SetName("Part Index");
      float *p = partArray->WritePointer(0, numPts);
      for (idx2 = 0; idx2 < numPts; ++idx2)
        {
        p[idx2] = (float)(idx);
        }
      output->GetPointData()->SetScalars(partArray);
      partArray->Delete();
      }
    } 
}



//------------------------------------------------------------------------------
void vtkExtractCTHPart2::ExecutePart(const char* arrayName,
                                     vtkRectilinear .........
                                     vtkPolyData* output)
{
  vtkRectilinearGrid* input = this->GetInput();
  vtkPolyData* tmp;
  vtkDataArray* cellVolumeFraction;
  vtkFloatArray* pointVolumeFraction;
  vtkClipPolyData *clip0;
  vtkDataSetSurfaceFilter *surface;
  vtkAppendPolyData *append1;
  vtkAppendPolyData *append2 = NULL;
  int* dims;

  vtkTimerLog::MarkStartEvent("Execute Part");

  data->GetCellData()->PassData(input->GetCellData());
  // Only convert single volume fraction array to point data.
  // Other attributes will have to be viewed as cell data.
  cellVolumeFraction = input->GetCellData()->GetArray(arrayName);
  if (cellVolumeFraction == NULL)
    {
    vtkErrorMacro("Could not find cell array " << arrayName);
    data->Delete();
    return;
    }
  if (cellVolumeFraction->GetDataType() != VTK_FLOAT)
    {
    vtkErrorMacro("Expecting volume fraction to be of type float.");
    data->Delete();
    return;
    }
  pointVolumeFraction = vtkFloatArray::New();
  dims = input->GetDimensions();
  pointVolumeFraction->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
  this->ExecuteCellDataToPointData(cellVolumeFraction, 
                                   pointVolumeFraction, dims);
  data->GetPointData()->SetScalars(pointVolumeFraction);

  // Create the contour surface.
#ifdef VTK_USE_PATENTED
  //vtkContourFilter *contour = vtkContourFilter::New();
  vtkContourFilter *contour = vtkKitwareContourFilter::New();
  // vtkDataSetSurfaceFilter does not generate normals, so they will be lost.
  contour->ComputeNormalsOff();
#else
  vtkContourFilter *contour = vtkContourFilter::New();
#endif
  contour->SetInput(data);
  contour->SetValue(0, 0.5);

  vtkTimerLog::MarkStartEvent("CTH Contour");
  contour->Update();
  vtkTimerLog::MarkEndEvent("CTH Contour");

  // Create the capping surface for the contour and append.
  append1 = vtkAppendPolyData::New();
  append1->AddInput(contour->GetOutput());
  surface = vtkDataSetSurfaceFilter::New();
  surface->SetInput(data);
  tmp = surface->GetOutput();

  vtkTimerLog::MarkStartEvent("Surface");
  tmp->Update();
  vtkTimerLog::MarkEndEvent("surface");

  // Clip surface less than volume fraction 0.5.
  clip0 = vtkClipPolyData::New();
  clip0->SetInput(surface->GetOutput());
  clip0->SetValue(0.5);
  tmp = clip0->GetOutput();
  vtkTimerLog::MarkStartEvent("Clip Surface");
  tmp->Update();
  vtkTimerLog::MarkEndEvent("Clip Surface");
  append1->AddInput(clip0->GetOutput());

  vtkTimerLog::MarkStartEvent("Append");
  append1->Update();
  vtkTimerLog::MarkEndEvent("Append");

  tmp = append1->GetOutput();
  
  if (this->Clipping && this->ClipPlane)
    {
    vtkClipPolyData *clip1, *clip2;
    // We need to append iso and capped surfaces.
    append2 = vtkAppendPolyData::New();
    // Clip the volume fraction iso surface.
    clip1 = vtkClipPolyData::New();
    clip1->SetInput(tmp);
    clip1->SetClipFunction(this->ClipPlane);
    append2->AddInput(clip1->GetOutput());
    // We need to create a capping surface.
#ifdef VTK_USE_PATENTED
    vtkCutter *cut = vtkKitwareCutter::New();
#else
    vtkCutter *cut = vtkCutter::New();
#endif
    cut->SetInput(data);
    cut->SetCutFunction(this->ClipPlane);
    cut->SetValue(0, 0.0);
    clip2 = vtkClipPolyData::New();
    clip2->SetInput(cut->GetOutput());
    clip2->SetValue(0.5);
    append2->AddInput(clip2->GetOutput());
    append2->Update();
    tmp = append2->GetOutput();
    clip1->Delete();
    clip1 = NULL;
    cut->Delete();
    cut = NULL;
    clip2->Delete();
    clip2 = NULL;
    }

  output->CopyStructure(tmp);
  output->GetCellData()->PassData(tmp->GetCellData());

  // Get rid of extra ghost levels.
  output->RemoveGhostCells(output->GetUpdateGhostLevel()+1);

  data->Delete();
  contour->Delete();
  surface->Delete();
  clip0->Delete();
  append1->Delete();
  if (append2)
    {
    append2->Delete();
    }
  pointVolumeFraction->Delete();

  // Add a name for this part.
  vtkCharArray *nameArray = vtkCharArray::New();
  nameArray->SetName("Name");
  char *str = nameArray->WritePointer(0, (int)(strlen(arrayName))+1);
  sprintf(str, "%s", arrayName);
  output->GetFieldData()->AddArray(nameArray);
  nameArray->Delete();
  vtkTimerLog::MarkEndEvent("Execute Part");
}


//------------------------------------------------------------------------------
void vtkExtractCTHPart2::ComputeDual(vtkRectilinearGrid* input,
                                     vtkRectilinearGrid* dual)
{
  vtkFloatArray* inCoords;
  vtkFloatArray* dualCoords;
  vtkIdType idx, num;
  float val0, val1;
  int ext[6];
  
  // Compute and set the dual extent.
  input->GetExtent(ext);
  ++ext[1];
  ++ext[3];
  ++ext[5];
  dual->SetExtent(ext);
  
  // Compute and set the coordinate arrays.
  // X
  inCoords = input->GetXCoordinates();
  dualCoords = vtkFloatArray::New();
  this->ComputeDualCoordinates(inCoors, dualCoords);
  dualCoords->SetXCoordinates(dualCoords);
  dualCoords->Delete();
  dualCoords = NULL;
  // Y
  inCoords = input->GetYCoordinates();
  dualCoords = vtkFloatArray::New();
  this->ComputeDualCoordinates(inCoors, dualCoords);
  dualCoords->SetYCoordinates(dualCoords);
  dualCoords->Delete();
  dualCoords = NULL;
  // Z
  inCoords = input->GetZCoordinates();
  dualCoords = vtkFloatArray::New();
  this->ComputeDualCoordinates(inCoors, dualCoords);
  dualCoords->SetZCoordinates(dualCoords);
  dualCoords->Delete();
  dualCoords = NULL;
  
  // Now we need to convert all of the cell data into point data.
  // The duplication of the first and last complicate the code.
  num = (ext[1]-ext[0]+1)*(ext[3]-ext[2]+1)*(ext[5]-ext[4]+1);
  vtkDataSetAttributes* inData = input->GetCellData();
  vtkDataSetAttributes* dualData = input->GetPointData();
  dualData->CopyAllocate(inputData, num);
  
  vtkIdType dualId;
  vtkIdType inX, inY, inZ;
  int x, y, z;
  // Which index to duplcate (at end)
  vtkIdType xEndDup, yEndDup, zEndDup;
  xEndDup = ext[1]-1;
  yEndDup = ext[3]-1;
  zEndDup = ext[5]-1;
  
  inZ = 0;
  for (z = ext[4]; z <= ext[5]; ++z)
    {
    inY = inZ;
    for (y = ext[2]; y <= ext[3]; ++y)
      {
      inX = inY;
      for (x = ext[0]; x <= ext[1]; ++x)
        {
        dualData->CopyData(inData, inX, dualId);
        ++dualId;
        // First and last are duplicated.
        if (x != ext[0] && x != xEndDup)
          {
          ++inX;
          }
        }
      // First and last are duplicated.
      if (y != ext[2] && y != yEndDup)
        {
        ++inY;
        } 
      }
    // First and last are duplicated.
    if (z != ext[4] && z != zEndDup)
      {
      ++inZ;
      }    
    }
}

//------------------------------------------------------------------------------
void vtkExtractCTHPart2::ComputeDualCoordinates(vtkFloatArray* inCoords,
                                                vtkFloatArray* dualCoords)
{
  vtkIdType idx, num;
  float val0, val1;
  
  num = inCoords->GetNumberOfTuples();
  val0 = inCoords->GetValue(0);
  // First and last values are special. (To keep the same bounds).
  dualCoords->InsertNextValue(val0);
  for (idx = 1; idx < num; ++idx)
    {
    val1 = inCoords->GetValue(idx);
    dualCoords->InsertNextValue(0.5*(val0+val1));
    val0 = val1;
    }
  // First and last values are special. (To keep the same bounds).
  dualCoords->InsertNextValue(val0);
}




//------------------------------------------------------------------------------
void vtkExtractCTHPart2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VolumeArrayNames: \n";
  vtkIndent i2 = indent.GetNextIndent();
  this->VolumeArrayNames->PrintSelf(os, i2);
  if (this->Clipping)
    {
    os << indent << "Clipping: On\n";
    if (this->ClipPlane)
      {
      os << indent << "ClipPlane:\n";
      this->ClipPlane->PrintSelf(os, indent.GetNextIndent());
      }
    else  
      {
      os << indent << "ClipPlane: NULL\n";
      }
    }
  else  
    {
    os << indent << "Clipping: Off\n";
    }
}

