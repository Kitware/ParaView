/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGeometryFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

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
class vtkHierarchicalBoxDataSet;
class vtkHierarchicalBoxOutlineFilter;
class vtkImageData;
class vtkStructuredGrid;
class vtkRectilinearGrid;
class vtkUnstructuredGrid;
class vtkOutlineSource;
class vtkMultiProcessController;
class vtkCallbackCommand;

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
  
  // Description:
  // Whether to generate cell normals.  Cell normals should speed up
  // rendering when point normals are not available.  They can only be used
  // for poly cells now.  This option does nothing if the output
  // contains lines, verts, or strips.
  vtkSetMacro(GenerateCellNormals, int);
  vtkGetMacro(GenerateCellNormals, int);
  vtkBooleanMacro(GenerateCellNormals, int);

  // Description:
  // Set and get the controller.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:
  vtkPVGeometryFilter();
  ~vtkPVGeometryFilter();

  virtual void Execute();
  virtual void ExecuteInformation();
  void DataSetExecute(vtkDataSet *input);
  void ImageDataExecute(vtkImageData *input);
  void StructuredGridExecute(vtkStructuredGrid *input);
  void RectilinearGridExecute(vtkRectilinearGrid *input);
  void UnstructuredGridExecute(vtkUnstructuredGrid *input);
  void PolyDataExecute(vtkPolyData *input);
  void DataSetSurfaceExecute(vtkDataSet *input);
  void ExecuteCellNormals(vtkPolyData *output);
  void HierarchicalBoxExecute(vtkHierarchicalBoxDataSet *input);

  int OutlineFlag;
  int UseOutline;
  int UseStrips;
  int GenerateCellNormals;

  vtkMultiProcessController* Controller;
  vtkOutlineSource *OutlineSource;
  vtkDataSetSurfaceFilter* DataSetSurfaceFilter;
  vtkHierarchicalBoxOutlineFilter* HierarchicalBoxOutline;

  int CheckAttributes(vtkDataObject* input);

  // Callback registered with the InternalProgressObserver.
  static void InternalProgressCallbackFunction(vtkObject*, unsigned long,
                                               void* clientdata, void*);
  void InternalProgressCallback();
  // The observer to report progress from the internal readers.
  vtkCallbackCommand* InternalProgressObserver;

  virtual int FillInputPortInformation(int, vtkInformation*);

  virtual void ReportReferences(vtkGarbageCollector*);
  virtual void RemoveReferences();
  private:
  vtkPVGeometryFilter(const vtkPVGeometryFilter&); // Not implemented
  void operator=(const vtkPVGeometryFilter&); // Not implemented
};

#endif


