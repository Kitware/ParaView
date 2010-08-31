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

#include "vtkDataObjectAlgorithm.h"
class vtkCallbackCommand;
class vtkDataSet;
class vtkDataSetSurfaceFilter;
class vtkGenericDataSet;
class vtkGenericGeometryFilter;
class vtkHyperOctree;
class vtkImageData;
class vtkInformationIntegerVectorKey;
class vtkInformationVector;
class vtkMultiProcessController;
class vtkOutlineSource;
class vtkPolyData;
class vtkPVRecoverGeometryWireframe;
class vtkRectilinearGrid;
class vtkStructuredGrid;
class vtkUnstructuredGrid;
class vtkUnstructuredGridGeometryFilter;

class VTK_EXPORT vtkPVGeometryFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkPVGeometryFilter *New();
  vtkTypeMacro(vtkPVGeometryFilter,vtkDataObjectAlgorithm);
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
  void SetUseStrips(int);
  vtkGetMacro(UseStrips, int);
  vtkBooleanMacro(UseStrips, int);

  // Desctiption:
  // Makes set use strips call modified after it changes the setting.
  void SetForceUseStrips(int);
  vtkGetMacro(ForceUseStrips, int);
  vtkBooleanMacro(ForceUseStrips, int);

  // Description:
  // Whether to generate cell normals.  Cell normals should speed up
  // rendering when point normals are not available.  They can only be used
  // for poly cells now.  This option does nothing if the output
  // contains lines, verts, or strips.
  vtkSetMacro(GenerateCellNormals, int);
  vtkGetMacro(GenerateCellNormals, int);
  vtkBooleanMacro(GenerateCellNormals, int);

  // Description:
  // Nonlinear faces are approximated with flat polygons.  This parameter
  // controls how many times to subdivide nonlinear surface cells.  Higher
  // subdivisions generate closer approximations but take more memory and
  // rendering time.  Subdivision is recursive, so the number of output polygons
  // can grow exponentially with this parameter.
  virtual void SetNonlinearSubdivisionLevel(int);
  vtkGetMacro(NonlinearSubdivisionLevel, int);

  // Description:
  // Set and get the controller.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // If on, the output polygonal dataset will have a celldata array that
  // holds the cell index of the original 3D cell that produced each output
  // cell. This is useful for picking. The default is off to conserve
  // memory.
  void SetPassThroughCellIds(int);
  vtkGetMacro(PassThroughCellIds,int);
  vtkBooleanMacro(PassThroughCellIds,int);

  // Description:
  // If on, the output polygonal dataset will have a pointdata array that
  // holds the point index of the original vertex that produced each output
  // vertex. This is useful for picking. The default is off to conserve
  // memory.
  void SetPassThroughPointIds(int);
  vtkGetMacro(PassThroughPointIds,int);
  vtkBooleanMacro(PassThroughPointIds,int);

  // Description:
  // If off, which is the default, extracts the surface of the data fed
  // into the geometry filter. If on, it produces a bounding box for the
  // input to the filter that is producing that data instead.
  vtkSetMacro(MakeOutlineOfInput,int);
  vtkGetMacro(MakeOutlineOfInput,int);
  vtkBooleanMacro(MakeOutlineOfInput,int);

  // Description:
  // These keys are put in the output composite-data metadata for multipieces
  // since this filter merges multipieces together.
  static vtkInformationIntegerVectorKey* POINT_OFFSETS();
  static vtkInformationIntegerVectorKey* VERTS_OFFSETS();
  static vtkInformationIntegerVectorKey* LINES_OFFSETS();
  static vtkInformationIntegerVectorKey* POLYS_OFFSETS();
  static vtkInformationIntegerVectorKey* STRIPS_OFFSETS();
//BTX
protected:
  vtkPVGeometryFilter();
  ~vtkPVGeometryFilter();

  // Description:
  // Overridden to create vtkMultiBlockDataSet when input is a
  // composite-dataset and vtkPolyData when input is a vtkDataSet.
  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector**,
                                vtkInformationVector*);
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);
  virtual int RequestCompositeData(vtkInformation* request,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  void ExecuteBlock(vtkDataObject* input,
                    vtkPolyData* output,
                    int doCommunicate);

  void DataSetExecute(vtkDataSet* input, vtkPolyData* output,
                      int doCommunicate);
  void GenericDataSetExecute(vtkGenericDataSet* input, vtkPolyData* output,
                             int doCommunicate);
  void ImageDataExecute(vtkImageData* input,
                        vtkPolyData* output,
                        int doCommunicate);
  void StructuredGridExecute(vtkStructuredGrid* input, vtkPolyData* output);
  void RectilinearGridExecute(vtkRectilinearGrid* input, vtkPolyData* output);
  void UnstructuredGridExecute(
    vtkUnstructuredGrid* input, vtkPolyData* output, int doCommunicate);
  void PolyDataExecute(
    vtkPolyData* input, vtkPolyData* output, int doCommunicate);
  void OctreeExecute(
    vtkHyperOctree* input, vtkPolyData* output, int doCommunicate);
  void ExecuteCellNormals(vtkPolyData* output, int doCommunicate);
  void ChangeUseStripsInternal(int val, int force);

  int OutlineFlag;
  int UseOutline;
  int UseStrips;
  int GenerateCellNormals;
  int NonlinearSubdivisionLevel;

  vtkMultiProcessController* Controller;
  vtkOutlineSource *OutlineSource;
  vtkDataSetSurfaceFilter* DataSetSurfaceFilter;
  vtkGenericGeometryFilter *GenericGeometryFilter;
  vtkUnstructuredGridGeometryFilter *UnstructuredGridGeometryFilter;
  vtkPVRecoverGeometryWireframe *RecoverWireframeFilter;

  // Description:
  // Call CheckAttributes on the \c input which ensures that all attribute
  // arrays have valid lengths.
  int CheckAttributes(vtkDataObject* input);

  // Callback registered with the InternalProgressObserver.
  static void InternalProgressCallbackFunction(vtkObject*, unsigned long,
                                               void* clientdata, void*);
  void InternalProgressCallback(vtkAlgorithm *algorithm);
  // The observer to report progress from the internal readers.
  vtkCallbackCommand* InternalProgressObserver;

  virtual int FillInputPortInformation(int, vtkInformation*);

  virtual void ReportReferences(vtkGarbageCollector*);

  // Description:
  // Overridden to request ghost-cells for vtkUnstructuredGrid inputs so that we
  // don't generate internal surfaces.
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);


  // Convenience method to purge ghost cells.
  void RemoveGhostCells(vtkPolyData*);

  int PassThroughCellIds;
  int PassThroughPointIds;
  int ForceUseStrips;
  vtkTimeStamp     StripSettingMTime;
  int StripModFirstPass;
  int MakeOutlineOfInput;

private:
  vtkPVGeometryFilter(const vtkPVGeometryFilter&); // Not implemented
  void operator=(const vtkPVGeometryFilter&); // Not implemented

  void AddCompositeIndex(vtkPolyData* pd, unsigned int index);
  void AddHierarchicalIndex(vtkPolyData* pd, unsigned int level, unsigned int index);
  class BoundsReductionOperation;
//ETX
};

#endif
