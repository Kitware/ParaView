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
#include "vtkSplitColumnComponents.h"          //  needed for enum

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkBlockDeliveryPreprocessor
  : public vtkDataObjectAlgorithm
{
public:
  static vtkBlockDeliveryPreprocessor* New();
  vtkTypeMacro(vtkBlockDeliveryPreprocessor, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
   * Corresponds to `vtkSplitColumnComponents::NamingMode`. Pick which mode to
   * use to name arrays when `FlattenTable` is true. Default is
   * `vtkSplitColumnComponents::NAMES_WITH_UNDERSCORES`.
   */
  vtkSetClampMacro(SplitComponentsNamingMode, int, vtkSplitColumnComponents::NUMBERS_WITH_PARENS,
    vtkSplitColumnComponents::NAMES_WITH_UNDERSCORES);
  vtkGetMacro(SplitComponentsNamingMode, int);
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
  ~vtkBlockDeliveryPreprocessor() override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FieldAssociation;
  int FlattenTable;
  bool GenerateOriginalIds;
  bool GenerateCellConnectivity;
  int SplitComponentsNamingMode;

private:
  vtkBlockDeliveryPreprocessor(const vtkBlockDeliveryPreprocessor&) = delete;
  void operator=(const vtkBlockDeliveryPreprocessor&) = delete;

  class CompositeDataSetIndicesType;
  CompositeDataSetIndicesType* CompositeDataSetIndices;
};

#endif
