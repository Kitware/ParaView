/*=========================================================================

  Program:   ParaView
  Module:    vtkQuerySelectionSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuerySelectionSource - a selection source that uses a "query" to
// generate the selection.
// .SECTION Description
// vtkQuerySelectionSource is a selection source that uses a "query" to generate
// the vtkSelection object.
// A query has the following form: "TERM OPERATOR VALUE(s)"
// eg. "GLOBALID is_in_range (0, 10)" here GLOBALID is the TERM, is_in_range is
// the operator and (0,10) are the values. A query can have additional
// qualifiers such as the process id, block id, amr level, amr block.

#ifndef __vtkQuerySelectionSource_h
#define __vtkQuerySelectionSource_h

#include "vtkSelectionAlgorithm.h"

class vtkAbstractArray;

class VTK_EXPORT vtkQuerySelectionSource : public vtkSelectionAlgorithm
{
public:
  static vtkQuerySelectionSource* New();
  vtkTypeMacro(vtkQuerySelectionSource, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum
    {
    NONE = 0
    };

  enum TermModes
    {
    ID=1,
    GLOBALID,
    ARRAY,
    LOCATION,
    BLOCK
    };

  enum OperatorTypes
    {
    IS_ONE_OF=1,
    IS_BETWEEN,
    IS_GE,
    IS_LE
    };
  //ETX

  // Description:
  // Get/Set the query term mode.
  vtkSetMacro(TermMode, int);
  vtkGetMacro(TermMode, int);

  // Description:
  // Set the array name if TermMode == ARRAY.
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);

  // Description:
  // Set the component number if TermMode == ARRAY.
  // -1 implies Magnitude in case of multicomponent arrays. Default is 0.
  vtkSetMacro(ArrayComponent, int);
  vtkGetMacro(ArrayComponent, int);

  // Description:
  // Get/Set the operator.
  vtkSetMacro(Operator, int);
  vtkGetMacro(Operator,  int);

  void SetNumberOfDoubleValues(unsigned int);
  void SetNumberOfIdTypeValues(unsigned int);
  void SetDoubleValues(double* values);
  void SetIdTypeValues(vtkIdType* values);

  // Description:
  vtkSetMacro(CompositeIndex, int);
  vtkGetMacro(CompositeIndex, int);

  vtkSetMacro(HierarchicalLevel, int);
  vtkGetMacro(HierarchicalLevel, int);

  vtkSetMacro(HierarchicalIndex, int);
  vtkGetMacro(HierarchicalIndex, int);

  vtkSetMacro(ProcessID, int);
  vtkGetMacro(ProcessID, int);

  // Possible values are as defined by
  // vtkSelectionNode::SelectionField.
  vtkSetMacro(FieldType, int);
  vtkGetMacro(FieldType, int);
 
  // Description:
  // Useful only when FieldType=POINT. If true, it results in selecting the
  // cells that contain the selected points.
  vtkSetMacro(ContainingCells, int);
  vtkGetMacro(ContainingCells, int);

  // Description:
  // Invert the selection.
  vtkSetMacro(Inverse, int);
  vtkGetMacro(Inverse, int);

  // Description:
  // This merely reconstructs the query as a user friendly text eg. "IDs >= 12".
  // ( Makes you want to wonder if we should support parsing input query text as
  // well ;) )
  const char* GetUserFriendlyText();

//BTX
protected:
  vtkQuerySelectionSource();
  ~vtkQuerySelectionSource();

  virtual int RequestInformation(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Returns a new instance for a vtkAbstractArray on success.
  vtkAbstractArray* BuildSelectionList();

  int TermMode;
  int Operator;
  int FieldType;
  int Inverse;

  char* ArrayName;
  int ArrayComponent;

  int CompositeIndex;
  int HierarchicalIndex;
  int HierarchicalLevel;
  int ProcessID;

  int ContainingCells;

  char* UserFriendlyText;
private:
  vtkQuerySelectionSource(const vtkQuerySelectionSource&); // Not implemented
  void operator=(const vtkQuerySelectionSource&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif

