/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGeometryFilter.h
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
// .NAME vtkPVGeometryFilter - Geometry filter that does outlines for volumes.
// .SECTION Description
// This filter defaults to using the outline filter unless the input
// is a structured volume.

#ifndef __vtkPVGeometryFilter_h
#define __vtkPVGeometryFilter_h

#include "vtkPolyDataSource.h"

class vtkDataObject;
class vtkDataSet;
class vtkDataSetSurfaceFilter;
#ifdef PARAVIEW_BUILD_DEVELOPMENT
class vtkHierarchicalBoxDataSet;
class vtkHierarchicalBoxOutlineFilter;
#endif
class vtkImageData;
class vtkStructuredGrid;
class vtkRectilinearGrid;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkPVGeometryFilter : public vtkPolyDataSource
{
public:
  static vtkPVGeometryFilter *New();
  vtkTypeRevisionMacro(vtkPVGeometryFilter,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This flag is set during the execute method.  It indicates
  // that the input was 3d and an outline representation was used.
  vtkGetMacro(OutlineFlag, int);

  // Description:
  // Set/get whether to produce outline (vs. surface).
  vtkSetMacro(UseOutline, int);
  vtkGetMacro(UseOutline, int);

  // Description:
  // When input is structured data, this flag will generate faces with
  // triangle strips.  This should render faster and use less memory, but no
  // cell data is copied.  By default, UseStrips is Off.
  vtkSetMacro(UseStrips, int);
  vtkGetMacro(UseStrips, int);
  vtkBooleanMacro(UseStrips, int);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkDataObject *input);
  vtkDataObject *GetInput();
  
protected:
  vtkPVGeometryFilter();
  ~vtkPVGeometryFilter();

  void Execute();
  void DataSetExecute(vtkDataSet *input);
  void ImageDataExecute(vtkImageData *input);
  void StructuredGridExecute(vtkStructuredGrid *input);
  void RectilinearGridExecute(vtkRectilinearGrid *input);
  void UnstructuredGridExecute(vtkUnstructuredGrid *input);
  void PolyDataExecute(vtkPolyData *input);
  void DataSetSurfaceExecute(vtkDataSet *input);
#ifdef PARAVIEW_BUILD_DEVELOPMENT
  void HierarchicalBoxExecute(vtkHierarchicalBoxDataSet *input);
#endif

  int OutlineFlag;
  int UseOutline;
  int UseStrips;

  vtkDataSetSurfaceFilter* DataSetSurfaceFilter;
#ifdef PARAVIEW_BUILD_DEVELOPMENT
  vtkHierarchicalBoxOutlineFilter* HierarchicalBoxOutline;
#endif

  int CheckAttributes(vtkDataObject* input);

private:
  vtkPVGeometryFilter(const vtkPVGeometryFilter&); // Not implemented
  void operator=(const vtkPVGeometryFilter&); // Not implemented
};

#endif


