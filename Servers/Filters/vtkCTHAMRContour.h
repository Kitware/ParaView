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
// .NAME vtkCTHAMRContour - Contour a cell attribute.
//
// .SECTION Description
// vtkCTHAMRContour can use ghost levels or its own internal algorithm 
// to eliminate seams between levels.

#ifndef __vtkCTHAMRContour_h
#define __vtkCTHAMRContour_h

#include "vtkCTHDataToPolyDataFilter.h"

class vtkCTHData;
class vtkStringList;
class vtkAppendPolyData;
class vtkContourFilter;
class vtkImageData;
class vtkPolyData;
class vtkFloatArray;
class vtkDataArray;
class vtkIdList;
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
  // Ignore ghost levels forces this filter to look at neighboring blocks.
  vtkSetMacro(IgnoreGhostLevels,int);
  vtkGetMacro(IgnoreGhostLevels,int);
  
  // Description:
  // Look at clip plane to compute MTime.
  unsigned long GetMTime();    

protected:
  vtkCTHAMRContour();
  ~vtkCTHAMRContour();

  virtual void Execute();

  void ExecuteBlock(vtkImageData* block, vtkPolyData* appendCache);
  void ExecutePart(const char* arrayName, vtkImageData* block, 
                   vtkPolyData* appendCache);
  void ExecuteCellDataToPointData(vtkDataArray *cellVolumeFraction, 
                                  vtkFloatArray *pointVolumeFraction, int *dims);

  void ExecuteCellDataToPointData2(vtkDataArray *cellVolumeFraction, 
                            vtkFloatArray *pointVolumeFraction, vtkCTHData* data);
  void FindBlockNeighbors(vtkCTHData* self, int blockId, vtkIdList* blockList);
  float ComputeSharedPoint(int blockId, vtkIdList* blockList, int x, int y, int z, 
                           double* pCell, float* pPoint, vtkCTHData* output);

  void CreateInternalPipeline();
  void DeleteInternalPipeline();

  // Pipeline to extract a part from a block.
  vtkImageData* Image;
  vtkPolyData* PolyData;
  vtkContourFilter* Contour;
  vtkAppendPolyData* FinalAppend;
  vtkIdList* IdList;


  // ----
  vtkContourValues *ContourValues;
  char *InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

  int IgnoreGhostLevels;

private:
  void InternalImageDataCopy(vtkCTHAMRContour *src);

  vtkCTHAMRContour(const vtkCTHAMRContour&);  // Not implemented.
  void operator=(const vtkCTHAMRContour&);  // Not implemented.
};


#endif



