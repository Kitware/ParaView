/*=========================================================================

  Program:   ParaView
  Module:    vtkAttributeDataToTableFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkAttributeDataToTableFilter - this filter produces a
// vtkTable from the chosen attribute in the input data object.
// .SECTION Description
// vtkAttributeDataToTableFilter is a filter that produces a vtkTable from the
// chosen attribute in the input dataobject. This filter can accept composite
// datasets. If the input is a composite dataset, the output is a multiblock
// with vtkTable leaves.

#ifndef __vtkAttributeDataToTableFilter_h
#define __vtkAttributeDataToTableFilter_h

#include "vtkTableAlgorithm.h"

class vtkFieldData;

class VTK_EXPORT vtkAttributeDataToTableFilter : public vtkTableAlgorithm
{
public:
  static vtkAttributeDataToTableFilter* New();
  vtkTypeMacro(vtkAttributeDataToTableFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Select the attribute type. Accepted values are 
  // \li vtkDataObject::FIELD_ASSOCIATION_POINTS,
  // \li vtkDataObject::FIELD_ASSOCIATION_CELLS,
  // \li vtkDataObject::FIELD_ASSOCIATION_NONE,
  // \li vtkDataObject::FIELD_ASSOCIATION_VERTICES,
  // \li vtkDataObject::FIELD_ASSOCIATION_EDGES,
  // \li vtkDataObject::FIELD_ASSOCIATION_ROWS
  // If value is vtkDataObject::FIELD_ASSOCIATION_NONE, then FieldData
  // associated with the input dataobject is extracted.
  vtkSetMacro(FieldAssociation, int);
  vtkGetMacro(FieldAssociation, int);

  // Description:
  // It is possible for this filter to add additional meta-data to the field
  // data such as point coordinates (when point attributes are selected and 
  // input is pointset) or structured coordinates etc. To enable this addition
  // of extra information, turn this flag on. Off by default.
  vtkSetMacro(AddMetaData, bool);
  vtkGetMacro(AddMetaData, bool);
  vtkBooleanMacro(AddMetaData, bool);

  // Description:
  // When set (default) the vtkOriginalIndices array will be added to the
  // output. Can be overridden by setting this flag to 0.
  // This is only respected when AddMetaData is true.
  vtkSetMacro(GenerateOriginalIds, bool);
  vtkGetMacro(GenerateOriginalIds, bool);

//BTX
protected:
  vtkAttributeDataToTableFilter();
  ~vtkAttributeDataToTableFilter();
 
  // Overridden to indicate to the executive that we accept non-composite
  // datasets. We let the executive manage the looping over the composite
  // dataset leaves. 
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // Perform the data processing 
  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*);

  // Description:
  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  // Description:
  // Internal method to return the chosen field from the input. May return 0 is
  // the chosen field is not applicable for the current data object or not
  // present.
  vtkFieldData* GetSelectedField(vtkDataObject* input);

  // Description:
  // Called when AddMetaData is true. Adds meta-data to the output.
  void Decorate(vtkTable* output, vtkDataObject* input);

  void PassFieldData(vtkFieldData* output, vtkFieldData* input);

  int FieldAssociation;
  bool AddMetaData;
  bool GenerateOriginalIds;
private:
  vtkAttributeDataToTableFilter(const vtkAttributeDataToTableFilter&); // Not implemented
  void operator=(const vtkAttributeDataToTableFilter&); // Not implemented
//ETX
};

#endif

