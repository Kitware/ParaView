/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHAMRCellToPointData.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHAMRCellToPointData - Convert one cell array to a point array.
//
// .SECTION Description
// vtkCTHAMRCellToPointData converts one cell data array to point data.
// It can use ghost cells, but they do not do the best job at eliminating
// seams between different levels.  It also has an algorithm to look
// at neighboring blocks.  This is better but it is not perfect.
// The last remaining issue is the interpolation. Marching cubes boundary
// does not match trilinear of higher level. 

#ifndef __vtkCTHAMRCellToPointData_h
#define __vtkCTHAMRCellToPointData_h

#include "vtkCTHDataToCTHDataFilter.h"

class vtkCTHData;
class vtkStringList;
class vtkImageData;
class vtkFloatArray;
class vtkDataArray;
class vtkStringList;
class vtkIdList;

class VTK_EXPORT vtkCTHAMRCellToPointData : public vtkCTHDataToCTHDataFilter
{
public:
  static vtkCTHAMRCellToPointData *New();

  vtkTypeRevisionMacro(vtkCTHAMRCellToPointData,vtkCTHDataToCTHDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // I am extending this filter to process multiple arrays at one time.
  void RemoveAllVolumeArrayNames();
  void AddVolumeArrayName(const char* arrayName);
  int GetNumberOfVolumeArrayNames();
  const char* GetVolumeArrayName(int idx);
        
  // Description:
  // Ignore ghost levels forces this filter to look at neighboring blocks.
  vtkSetMacro(IgnoreGhostLevels,int);
  vtkGetMacro(IgnoreGhostLevels,int);
  
protected:
  vtkCTHAMRCellToPointData();
  ~vtkCTHAMRCellToPointData();

  virtual void Execute();
  void CreateOutputGeometry(vtkCTHData* input, vtkCTHData* output);

  void ExecuteCellDataToPointData(
                         vtkCTHData* input, vtkDataArray *cellVolumeFraction, 
                         vtkCTHData* output, vtkFloatArray *pointVolumeFraction);

  void ExecuteCellDataToPointData2(vtkCTHData* input, vtkCTHData* output);
  void FindBlockNeighbors(vtkCTHData* self, int blockId, vtkIdList* blockList);
  float ComputeSharedPoint(int blockId, vtkIdList* blockList, int x, int y, int z, 
                           double* pCell, float* pPoint, vtkCTHData* input, vtkCTHData* output);
  void CopyCellData(vtkCTHData* input, vtkCTHData* output);
                           
  vtkStringList *VolumeArrayNames;
  vtkIdList* IdList;

  int IgnoreGhostLevels;

private:
  void InternalImageDataCopy(vtkCTHAMRCellToPointData *src);

  vtkCTHAMRCellToPointData(const vtkCTHAMRCellToPointData&);  // Not implemented.
  void operator=(const vtkCTHAMRCellToPointData&);  // Not implemented.
};


#endif



