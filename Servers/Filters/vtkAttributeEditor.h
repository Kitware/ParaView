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
// .NAME vtkAttributeEditor - subclass of vtkClipDataSet
// .SECTION Description

// .SECTION See Also
// vtkClipDataSet

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

#define VTK_ATTRIBUTE_MODE_DEFAULT         0
#define VTK_ATTRIBUTE_MODE_USE_POINT_DATA  1
#define VTK_ATTRIBUTE_MODE_USE_CELL_DATA   2

class VTK_EXPORT vtkAttributeEditor : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkAttributeEditor,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function; InsideOut turned off;
  // value set to 0.0; and generate clip scalars turned off.
  static vtkAttributeEditor *New();

  // Description:
  // Get any input of this filter.
//BTX
  vtkDataSet *GetInput(int idx);
  vtkDataSet *GetInput() 
    {return this->GetInput( 0 );}
//ETX

  vtkSetMacro(UnfilteredDataset,int);
  vtkGetMacro(UnfilteredDataset,int);
  vtkBooleanMacro(UnfilteredDataset,int);

  // Description:
  // Remove a dataset from the list of data to append.
  void RemoveInput(vtkDataSet *in);

  // Description:
  // Set the clipping value of the implicit function (if clipping with
  // implicit function) or scalar value (if clipping with
  // scalars). The default value is 0.0. 
  vtkSetMacro(Value,double);
  vtkGetMacro(Value,double);

  // Description
  // Specify the implicit function with which to perform the
  // clipping. If you do not define an implicit function, 
  // then the selected input scalar data will be used for clipping.
  virtual void SetClipFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ClipFunction,vtkImplicitFunction);

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

  // Point picking stuff

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

  // Point picking stuff:

  void BoxExecute(vtkDataSet *, vtkDataSet *);
  void PointExecute(vtkDataSet *, vtkDataSet *);
  void CellExecute(vtkDataSet *, vtkDataSet *);

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

  // Locator did no do what I wanted.
  vtkIdType FindPointId(double pt[3], vtkDataSet* input);

private:
  vtkAttributeEditor(const vtkAttributeEditor&);  // Not implemented.
  void operator=(const vtkAttributeEditor&);  // Not implemented.
};

#endif
