/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHExtractAMRPart.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHExtractAMRPart - A source to test my new CTH AMR data object.
// .SECTION Description
// vtkCTHExtractAMRPart is a collection of image datas.  All have the same dimensions.
// Each block has a different origin and spacing.  It uses mandelbrot
// to create cell data.  I scale the fractal array to look like a volme fraction.
// I may also add block id and level as extra cell arrays.

#ifndef __vtkCTHExtractAMRPart_h
#define __vtkCTHExtractAMRPart_h

#include "vtkCTHDataToPolyDataFilter.h"

class vtkCTHData;
class vtkPlane;
class vtkStringList;
class vtkAppendPolyData;
class vtkImageData;
class vtkFloatArray;
class vtkDataArray;

class VTK_EXPORT vtkCTHExtractAMRPart : public vtkCTHDataToPolyDataFilter
{
public:
  static vtkCTHExtractAMRPart *New();

  vtkTypeRevisionMacro(vtkCTHExtractAMRPart,vtkCTHDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Names of cell volume fraction arrays to extract.
  void RemoveAllVolumeArrayNames();
  void AddVolumeArrayName(char* arrayName);
  int GetNumberOfVolumeArrayNames();
  const char* GetVolumeArrayName(int idx);

  // Description:
  int GetNumberOfOutputs();
  vtkPolyData* GetOutput(int idx);
  vtkPolyData* GetOutput() { return this->GetOutput(0); }
  void SetOutput(int idx, vtkPolyData* d);
  void SetOutput(vtkPolyData* d) { this->SetOutput(0, d); }

  // Description:
  // Turn clipping on or off.  It is off by default.
  vtkSetMacro(Clipping,int);
  vtkGetMacro(Clipping,int);
  vtkBooleanMacro(Clipping,int);
  
  // Description:
  // Set, get or maninpulate the implicit clipping plane.
  void SetClipPlane(vtkPlane *clipPlane);
  vtkGetObjectMacro(ClipPlane, vtkPlane);

  // Description:
  // Look at clip plane to compute MTime.
  unsigned long GetMTime();    

protected:
  vtkCTHExtractAMRPart();
  ~vtkCTHExtractAMRPart();

  virtual void Execute();
  void ExecuteBlock(vtkImageData* block, vtkAppendPolyData** appends);
  void ExecutePart(const char* arrayName, vtkImageData* block, 
                   vtkAppendPolyData* append);
  void ExecuteCellDataToPointData(vtkDataArray *cellVolumeFraction, 
                                  vtkFloatArray *pointVolumeFraction, int *dims);

  int Clipping;
  vtkPlane* ClipPlane;
  vtkStringList *VolumeArrayNames;

private:
  void InternalImageDataCopy(vtkCTHExtractAMRPart *src);

private:
  vtkCTHExtractAMRPart(const vtkCTHExtractAMRPart&);  // Not implemented.
  void operator=(const vtkCTHExtractAMRPart&);  // Not implemented.
};


#endif



