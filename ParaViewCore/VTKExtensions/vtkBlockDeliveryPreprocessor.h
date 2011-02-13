/*=========================================================================

  Program:   ParaView
  Module:    vtkBlockDeliveryPreprocessor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkBlockDeliveryPreprocessor - filter used by block-delivery
// representation for pre-processing data.
// .SECTION Description
// vtkBlockDeliveryPreprocessor is a filter used by block-delivery
// representation for pre-processing data.
// It internally uses vtkAttributeDataToTableFilter.

#ifndef __vtkBlockDeliveryPreprocessor_h
#define __vtkBlockDeliveryPreprocessor_h

#include "vtkDataObjectAlgorithm.h"

class VTK_EXPORT vtkBlockDeliveryPreprocessor : public vtkDataObjectAlgorithm
{
public:
  static vtkBlockDeliveryPreprocessor* New();
  vtkTypeMacro(vtkBlockDeliveryPreprocessor, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // In case of Composite datasets, set the flat index of the subtree to pass.
  // Default is 0 which results in passing the entire composite tree.
  vtkSetMacro(CompositeDataSetIndex, unsigned int);
  vtkGetMacro(CompositeDataSetIndex, unsigned int);
  void SetCompositeDataSetIndex() { this->SetCompositeDataSetIndex(0); }

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
  // Flatten the table, i.e. split any multicomponent columns into separate
  // components, internally the vtkSplitColumnComponents filter is used.
  vtkSetMacro(FlattenTable, int);
  vtkGetMacro(FlattenTable, int);

  // Description:
  // When set (default) the vtkOriginalIndices array will be added to the
  // output. Can be overridden by setting this flag to 0.
  // This is only respected when AddMetaData is true.
  vtkSetMacro(GenerateOriginalIds, bool);
  vtkGetMacro(GenerateOriginalIds, bool);

//BTX
protected:
  vtkBlockDeliveryPreprocessor();
  ~vtkBlockDeliveryPreprocessor();

  virtual vtkExecutive* CreateDefaultExecutive();

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestDataObject(vtkInformation*,
                                vtkInformationVector**,
                                vtkInformationVector*);

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);

  int FieldAssociation;
  unsigned int CompositeDataSetIndex;
  int FlattenTable;
  bool GenerateOriginalIds;
private:
  vtkBlockDeliveryPreprocessor(const vtkBlockDeliveryPreprocessor&); // Not implemented
  void operator=(const vtkBlockDeliveryPreprocessor&); // Not implemented
//ETX
};

#endif

