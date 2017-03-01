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
/**
 * @class   vtkBlockDeliveryPreprocessor
 * @brief   filter used by block-delivery
 * representation for pre-processing data.
 *
 * vtkBlockDeliveryPreprocessor is a filter used by block-delivery
 * representation for pre-processing data.
 * It internally uses vtkAttributeDataToTableFilter.
*/

#ifndef vtkBlockDeliveryPreprocessor_h
#define vtkBlockDeliveryPreprocessor_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkBlockDeliveryPreprocessor
  : public vtkDataObjectAlgorithm
{
public:
  static vtkBlockDeliveryPreprocessor* New();
  vtkTypeMacro(vtkBlockDeliveryPreprocessor, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * In case of Composite datasets, set the flat index of the subtree to pass.
   * Default or empty results in passing the entire composite tree.
   */
  void AddCompositeDataSetIndex(unsigned int index);
  void RemoveAllCompositeDataSetIndices();
  //@}

  //@{
  /**
   * Allow user to enable/disable cell connectivity generation in the datamodel
   */
  vtkSetMacro(GenerateCellConnectivity, bool);
  vtkGetMacro(GenerateCellConnectivity, bool);
  //@}

  //@{
  /**
   * Select the attribute type. Accepted values are
   * \li vtkDataObject::FIELD_ASSOCIATION_POINTS,
   * \li vtkDataObject::FIELD_ASSOCIATION_CELLS,
   * \li vtkDataObject::FIELD_ASSOCIATION_NONE,
   * \li vtkDataObject::FIELD_ASSOCIATION_VERTICES,
   * \li vtkDataObject::FIELD_ASSOCIATION_EDGES,
   * \li vtkDataObject::FIELD_ASSOCIATION_ROWS
   * If value is vtkDataObject::FIELD_ASSOCIATION_NONE, then FieldData
   * associated with the input dataobject is extracted.
   */
  vtkSetMacro(FieldAssociation, int);
  vtkGetMacro(FieldAssociation, int);
  //@}

  //@{
  /**
   * Flatten the table, i.e. split any multicomponent columns into separate
   * components, internally the vtkSplitColumnComponents filter is used.
   */
  vtkSetMacro(FlattenTable, int);
  vtkGetMacro(FlattenTable, int);
  //@}

  //@{
  /**
   * When set (default) the vtkOriginalIndices array will be added to the
   * output. Can be overridden by setting this flag to 0.
   * This is only respected when AddMetaData is true.
   */
  vtkSetMacro(GenerateOriginalIds, bool);
  vtkGetMacro(GenerateOriginalIds, bool);
  //@}

protected:
  vtkBlockDeliveryPreprocessor();
  ~vtkBlockDeliveryPreprocessor();

  virtual vtkExecutive* CreateDefaultExecutive() VTK_OVERRIDE;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  int FieldAssociation;
  int FlattenTable;
  bool GenerateOriginalIds;
  bool GenerateCellConnectivity;

private:
  vtkBlockDeliveryPreprocessor(const vtkBlockDeliveryPreprocessor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBlockDeliveryPreprocessor&) VTK_DELETE_FUNCTION;

  class CompositeDataSetIndicesType;
  CompositeDataSetIndicesType* CompositeDataSetIndices;
};

#endif
