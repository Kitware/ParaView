/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAttributeEditor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAttributeEditor
// .SECTION Description
//  This class edits an attribute value either within a region or at a point. 
//  It takes two inputs, one from a filter and one from the original 
//  source/reader (these could be the same). When these are different, the 
//  points and cells in the filter are used to narrow the region to edit and 
//  acts as a selector. The editable region is defined by an ImplicitFunction.
//  This class was modeled after vtkClipDataSet.

#ifndef __vtkAttributeEditor_h
#define __vtkAttributeEditor_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkImplicitFunction;
class vtkPointLocator;
class vtkMultiProcessController;
class vtkIdList;
class vtkIntArray;
class vtkPoints;
class vtkDataSet;
class vtkAppendFilter;
class vtkFloatArray;

#define VTK_ATTRIBUTE_MODE_DEFAULT         0
#define VTK_ATTRIBUTE_MODE_USE_POINT_DATA  1
#define VTK_ATTRIBUTE_MODE_USE_CELL_DATA   2

class VTK_EXPORT vtkAttributeEditor : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkAttributeEditor,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkAttributeEditor *New();

  // Description:
  // Get either input of this filter.
//BTX
//  vtkDataSet *GetInput(int idx);
//  vtkDataSet *GetInput() 
//    {return this->GetInput( 0 );}
//ETX

  // Description:
  // Specify the source object used when writing out data.
  void SetSource(vtkDataSet *source);
  vtkDataSet *GetSource();

  // Description:
  // Remove a dataset from the list of data to append.
  //void RemoveInput(vtkDataSet *in);

  //void AddInput(vtkDataObject *);
  //void AddInput(int, vtkDataObject*);

  // Description:
  // Access to the flag that toggles between a source view and a filter view
  vtkSetMacro(UnfilteredDataset,int);
  vtkGetMacro(UnfilteredDataset,int);
  vtkBooleanMacro(UnfilteredDataset,int);

  // Description:
  // Set the clipping value of the implicit function (if clipping with
  // implicit function) or point. The default value is 0.0. 
  vtkSetMacro(Value,double);
  vtkGetMacro(Value,double);

  // Description
  // Specify the implicit function with which to perform the
  // clipping. If you do not define an implicit function, 
  // then the selected input scalar data will be used for clipping.
  virtual void SetClipFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ClipFunction,vtkImplicitFunction);

  // Description:
  // Function could be a vtk3DWidget or vtkImplicitFunction
  void SetPickFunction(vtkObject *func);

  // Description:
  // Control how the filter works with scalar point data and cell attribute
  // data.  By default (AttributeModeToDefault), the filter will use point
  // data, and if no point data is available, then cell data is
  // used. Alternatively you can explicitly set the filter to use point data
  // (AttributeModeToUsePointData) or cell data (AttributeModeToUseCellData).
  vtkSetMacro(AttributeMode,int);
  vtkGetMacro(AttributeMode,int);
  void SetAttributeModeToDefault() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_DEFAULT);};
  void SetAttributeModeToUsePointData() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_POINT_DATA);};
  void SetAttributeModeToUseCellData() 
    {this->SetAttributeMode(VTK_ATTRIBUTE_MODE_USE_CELL_DATA);};
  const char *GetAttributeModeAsString();

  vtkSetMacro(AttributeValue,double);
  vtkGetMacro(AttributeValue,double);

  vtkSetMacro(EditMode,int);
  vtkGetMacro(EditMode,int);
  vtkBooleanMacro(EditMode,int);

  // When this flag is set, the next time the filter is executed, the arrays of edits will be deleted.
  // This is meant to be called just before a timestep change is about to occur.
  vtkSetMacro(ClearEdits,int);
  vtkGetMacro(ClearEdits,int);
  vtkBooleanMacro(ClearEdits,int);

  // Description:
  // Set the tolerance for merging clip intersection points that are near
  // the vertices of cells. This tolerance is used to prevent the generation
  // of degenerate primitives. Note that only 3D cells actually use this
  // instance variable.
  vtkSetClampMacro(MergeTolerance,double,0.0001,0.25);
  vtkGetMacro(MergeTolerance,double);

  // Description:
  // Return the mtime also considering the locator and clip function.
  unsigned long GetMTime();

  // Description:
  // Specify a spatial locator for merging points. By default, an
  // instance of vtkMergePoints is used.
  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. The 
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

  // Point picking stuff - borrowed from vtkPickFilter

  // Description:
  // Set your picking point here.
  vtkSetVector3Macro(WorldPoint,double);
  vtkGetVector3Macro(WorldPoint,double);

  // Description:
  // Select whether you are us9ing a world point to pick, or
  // a cell / point id.
  vtkSetMacro(PickCell,int);
  vtkGetMacro(PickCell,int);
  vtkBooleanMacro(PickCell,int);

  // Description:
  // This is set by default (if compiled with MPI).
  // User can override this default.
  void SetController(vtkMultiProcessController* controller);
  
protected:
  vtkAttributeEditor(vtkImplicitFunction *cf=NULL);
  ~vtkAttributeEditor();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  vtkImplicitFunction *ClipFunction;
  
  vtkPointLocator *Locator;
  double Value;

  double MergeTolerance;

  //helper functions
  void ClipVolume(vtkDataSet *input, vtkUnstructuredGrid *output);

  double AttributeValue;
  int AttributeMode;
  int EditMode;
  int IsPointPick;
  int UnfilteredDataset;
  int ClearEdits;

  // Point picking stuff:

  void RegionExecute(vtkDataSet *readerInput, vtkDataSet *filterInput, vtkDataSet *readerOutput, vtkDataSet *filterOutput);
  void PointExecute(vtkDataSet *readerInput, vtkDataSet *filterInput, vtkDataSet *readerOutput, vtkDataSet *filterOutput);
  void CellExecute(vtkDataSet *readerInput, vtkDataSet *filterInput, vtkDataSet *readerOutput, vtkDataSet *filterOutput);

  void CreateOutput(vtkIdList* regionCellIds);
  int CompareProcesses(double bestDist2);

  // Flag that toggles between picking cells or picking points.
  int PickCell;

  vtkMultiProcessController* Controller;

  // Input pick point.
  double WorldPoint[3];

  // Index is the input id, value is the output id.
  vtkIdList* PointMap;
  // Index is the output id, value is the input id.
  vtkIdList* RegionPointIds;

  // I need this because I am converting this filter
  // to have multiple inputs, and removing the layer feature
  // at the same time.  Maps can only be from one input.
  int BestInputIndex;

  // Returns outputId.
  vtkIdType InsertIdInPointMap(vtkIdType inId);
  void InitializePointMap(vtkIdType numerOfInputPoints);
  void DeletePointMap();
  int ListContainsId(vtkIdList* ids, vtkIdType id);

  // Locator did not do what I wanted.
  vtkIdType FindPointId(double pt[3], vtkDataSet* input);

  vtkFloatArray *FilterDataArray;
  vtkFloatArray *ReaderDataArray;

private:
  vtkAttributeEditor(const vtkAttributeEditor&);  // Not implemented.
  void operator=(const vtkAttributeEditor&);  // Not implemented.
};

#endif
