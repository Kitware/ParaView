/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPickFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPickFilter - Find nearest point and cell.
// .SECTION Description
// This filter is for picking points and cell in paraview.
// It executes in parallel on distributed data sets.  
// It assumes MPI we have an MPI controller.  The data remains distributed.
// The user sets a point, and the filter finds the nearest
// input point or cell.  Layers of cells can be grown through 
// point connectivity.  The output is the input point/cell/region.  
// I will have an interface that allows paraview to
// get the attributes of the picked point or cell.

#ifndef __vtkPickFilter_h
#define __vtkPickFilter_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class vtkMultiProcessController;
class vtkIdList;
class vtkIntArray;
class vtkPoints;

class VTK_EXPORT vtkPickFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  static vtkPickFilter *New();
  vtkTypeRevisionMacro(vtkPickFilter,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set your picking point here.
  vtkSetVector3Macro(WorldPoint,double);
  vtkGetVector3Macro(WorldPoint,double);

  // Description:
  // Select whether you are picking point or cells.
  // The default value of this flag is off (picking points).
  vtkSetMacro(PickCell,int);
  vtkGetMacro(PickCell,int);
  vtkBooleanMacro(PickCell,int);
  
  // Description:
  // This value specifies how many layers to grow 
  // around the picked point or cell. Default value is 0.
  vtkSetMacro(NumberOfLayers, int);  
  vtkGetMacro(NumberOfLayers, int);  

  // Description:
  // By default this is on.  
  // When this is on and NumberOfLevels larger than zero, 
  // this filter generates point and cell arrays containg the level.
  vtkSetMacro(GenerateLayerAttribute, int);  
  vtkGetMacro(GenerateLayerAttribute, int);  
  vtkBooleanMacro(GenerateLayerAttribute, int);  

  // Description:
  // This is set by default (if compiled with MPI).
  // User can override this default.
  void SetController(vtkMultiProcessController* controller);
  
protected:
  vtkPickFilter();
  ~vtkPickFilter();

  void Execute();
  void PointExecute();
  void CellExecute();
  int CompareProcesses(double bestDist2);

  // For the Onion peel (growing layers)
  void Grow(int level, vtkIdList* regionCellIds, vtkIntArray* regionCellLevels);
  void GetInputLayerPoints(vtkPoints* pts, int level, vtkDataSet* input);
  void GatherPoints(vtkPoints* pts);
  void CreateOutput(vtkIdList* regionCellIds, vtkIntArray* regionCellLevels);

  // Flag that toggles between picking cells or picking points.
  int PickCell;
  int NumberOfLayers;
  int GenerateLayerAttribute;

  // Input pick point.
  double WorldPoint[3];

  vtkMultiProcessController* Controller;

  // Index is the input id, value is the output id.
  vtkIdList* PointMap;
  // Index is the output id, value is the input id.
  vtkIdList* RegionPointIds;
  // Index is the output id, value is the level/layer.
  vtkIntArray* RegionPointLevels;

  // Returns outputId.
  vtkIdType InsertIdInPointMap(vtkIdType inId, int level);
  void InitializePointMap(vtkIdType numerOfInputPoints);
  void DeletePointMap();
  int ListContainsId(vtkIdList* ids, vtkIdType id);

  // Locator did no do what I wanted.
  vtkIdType FindPointId(double pt[3], vtkDataSet* input);

private:
  vtkPickFilter(const vtkPickFilter&);  // Not implemented.
  void operator=(const vtkPickFilter&);  // Not implemented.
};

#endif


