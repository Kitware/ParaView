/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractCTHPart2.h
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
// .NAME vtkExtractCTHPart2 - Generates surface of an CTH volume fraction.
// .SECTION Description
// vtkExtractCTHPart2 is a filter that is specialized for creating 
// visualization of a CTH simulation.  
// This filter is an attempt to avoid the cell to point data conversion.
// We first create a dual grid which converts cell data to point data
// without the averaging step.

#ifndef __vtkExtractCTHPart2_h
#define __vtkExtractCTHPart2_h

#include "vtkRectilinearGridToPolyDataFilter.h"
class vtkPlane;
class vtkDataArray;
class vtkFloatArray;
class vtkStringList;

class VTK_EXPORT vtkExtractCTHPart2 : public vtkRectilinearGridToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkExtractCTHPart2,vtkRectilinearGridToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkExtractCTHPart2 *New();

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
  vtkExtractCTHPart2();
  ~vtkExtractCTHPart2();

  void ComputeInputUpdateExtents(vtkDataObject *output);
  void Execute();
  void ExecutePart(const char* arrayName, vtkPolyData* output);
  void ExecuteCellDataToPointData(vtkDataArray *cellVolumeFraction, 
                       vtkFloatArray *pointVolumeFraction, int *dims);

  vtkStringList *VolumeArrayNames;

  int Clipping;
  vtkPlane *ClipPlane;

private:
  vtkExtractCTHPart2(const vtkExtractCTHPart2&);  // Not implemented.
  void operator=(const vtkExtractCTHPart2&);  // Not implemented.
};

#endif

