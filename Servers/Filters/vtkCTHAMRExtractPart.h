/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHAMRExtractPart.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHAMRExtractPart - A source to test the new CTH AMR data object.
//
// .SECTION Description
// vtkCTHAMRExtractPart is a collection of image datas. All have the same 
// dimensions. Each block has a different origin and spacing. It uses mandelbrot
// to create cell data. I scale the fractal array to look like a volume 
// fraction. I may also add block id and level as extra cell arrays.

#ifndef __vtkCTHAMRExtractPart_h
#define __vtkCTHAMRExtractPart_h

#include "vtkCTHDataToPolyDataFilter.h"
#include "vtkToolkits.h" // I need the VTK_USE_PATENTED flag 

class vtkCTHData;
class vtkPlane;
class vtkStringList;
class vtkAppendPolyData;
class vtkSynchronizedTemplates3D;
class vtkContourFilter;
class vtkClipPolyData;
class vtkCutter;
class vtkImageData;
class vtkPolyData;
class vtkFloatArray;
class vtkDataArray;
class vtkIdList;

class VTK_EXPORT vtkCTHAMRExtractPart : public vtkCTHDataToPolyDataFilter
{
public:
  static vtkCTHAMRExtractPart *New();

  vtkTypeRevisionMacro(vtkCTHAMRExtractPart,vtkCTHDataToPolyDataFilter);
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
  // Set, get or maninpulate the implicit clipping plane.
  void SetClipPlane(vtkPlane *clipPlane);
  vtkGetObjectMacro(ClipPlane, vtkPlane);

  // Description:
  // Use a different algorithm that ignores ghost levels.
  vtkSetMacro(IgnoreGhostLevels, int);
  vtkGetMacro(IgnoreGhostLevels, int);

  // Description:
  // Look at clip plane to compute MTime.
  unsigned long GetMTime();    

protected:
  vtkCTHAMRExtractPart();
  ~vtkCTHAMRExtractPart();

  virtual void Execute();

  void ExecuteBlock(vtkImageData* block, vtkAppendPolyData** appends, double startProgress, double stopProgress);
  void ExecutePart(const char* arrayName, vtkImageData* block, 
                   vtkAppendPolyData* append);

  int IgnoreGhostLevels;

  vtkPlane* ClipPlane;
  vtkStringList *VolumeArrayNames;
  vtkIdList* IdList;

  void CreateInternalPipeline();
  void DeleteInternalPipeline();

  // Pipeline to extract a part from a block.
  vtkImageData* Image;
  // Polydata is not instantiated.  It just holds the output
  // from the internal pipeline.
  vtkPolyData* PolyData;
  // KitwareContourFilter took too long because of garbage collection
  // when input of internal filter was deleted.
  // Synchronized template super class is poly data source.
#ifdef VTK_USE_PATENTED  
  vtkSynchronizedTemplates3D* Contour;
#else
  vtkContourFilter* Contour;
#endif
  vtkAppendPolyData* Append1;
  vtkAppendPolyData* Append2;
  vtkClipPolyData* Clip1;
  vtkClipPolyData* Clip2;
  vtkCutter* Cut;

private:
  void InternalImageDataCopy(vtkCTHAMRExtractPart *src);

  vtkCTHAMRExtractPart(const vtkCTHAMRExtractPart&);  // Not implemented.
  void operator=(const vtkCTHAMRExtractPart&);  // Not implemented.
};


#endif



