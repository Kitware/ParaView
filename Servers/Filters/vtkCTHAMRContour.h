/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHAMRContour.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHAMRContour - A source to test the new CTH AMR data object.
//
// .SECTION Description
// vtkCTHAMRContour is a collection of image datas. All have the same 
// dimensions. Each block has a different origin and spacing. It uses mandelbrot
// to create cell data. I scale the fractal array to look like a volume 
// fraction. I may also add block id and level as extra cell arrays.

#ifndef __vtkCTHAMRContour_h
#define __vtkCTHAMRContour_h

#include "vtkCTHDataToPolyDataFilter.h"
#include "vtkToolkits.h" // I need the VTK_USE_PATENTED flag 

class vtkCTHData;
class vtkPlane;
class vtkAppendPolyData;
class vtkSynchronizedTemplates3D;
class vtkContourFilter;
class vtkDataSetSurfaceFilter;
class vtkCutter;
class vtkImageData;
class vtkPolyData;
class vtkFloatArray;
class vtkDataArray;
class vtkContourValues;

class VTK_EXPORT vtkCTHAMRContour : public vtkCTHDataToPolyDataFilter
{
public:
  static vtkCTHAMRContour *New();

  vtkTypeRevisionMacro(vtkCTHAMRContour,vtkCTHDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Methods to set / get contour values.
  void SetValue(int i, double value);
  double GetValue(int i);
  double *GetValues();
  void GetValues(double *contourValues);
  void SetNumberOfContours(int number);
  int GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);

  // Description:
  // If you want to contour by an arbitrary array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  virtual void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}

  // Description:
  // Use a different algorithm that ignores ghost levels.
  vtkSetMacro(IgnoreGhostLevels, int);
  vtkGetMacro(IgnoreGhostLevels, int);

  // Description:
  // Look at contours to compute MTime.
  unsigned long GetMTime();    

protected:
  vtkCTHAMRContour();
  ~vtkCTHAMRContour();

  virtual void Execute();

  void ExecuteBlock(vtkImageData* block, vtkAppendPolyData* append, double startProgress);
  void ExecutePart(const char* arrayName, vtkImageData* block, 
                   vtkAppendPolyData* append);


  // ----
  vtkContourValues *ContourValues;
  char *InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

  int IgnoreGhostLevels;

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

private:
  void InternalImageDataCopy(vtkCTHAMRContour *src);

  vtkCTHAMRContour(const vtkCTHAMRContour&);  // Not implemented.
  void operator=(const vtkCTHAMRContour&);  // Not implemented.
};


#endif



