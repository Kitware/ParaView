/*=========================================================================

  Program:   ParaView
  Module:    vtkDataTabulator.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkDataTabulator
 * @brief converts input data to a table (or composite-data of tables)
 *
 * vtkDataTabulator is intended for use by representations that primarily work
 * with tabular data. It converts input data to table. This is done by selecting
 * which type of data is to be extracted e.g. cell data, point data, field data
 * etc. The filter also supports adding meta-data to the output to encode
 * information in the input that'd potentially be lost by this conversion.
 */

#ifndef vtkDataTabulator_h
#define vtkDataTabulator_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkInformationIntegerKey.h"
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" // needed for export macro
#include "vtkSmartPointer.h"
#include "vtkSplitColumnComponents.h"

#include <set>

class vtkPartitionedDataSet;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkDataTabulator : public vtkDataObjectAlgorithm
{
public:
  static vtkDataTabulator* New();
  vtkTypeMacro(vtkDataTabulator, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  void AddSelector(const char* selector);
  void ClearSelectors();
  //@}

  //@{
  /**
   * Get/Set the name of the assembly you use together with the selectors to
   * subset. Default is Hierarchy.
   */
  vtkSetStringMacro(ActiveAssemblyForSelectors);
  vtkGetStringMacro(ActiveAssemblyForSelectors);
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
  vtkSetMacro(SplitComponents, int);
  vtkGetMacro(SplitComponents, int);
  //@}

  //@{
  /**
   * Corresponds to `vtkSplitColumnComponents::NamingMode`. Pick which mode to
   * use to name arrays when `SplitComponents` is true. Default is
   * `vtkSplitColumnComponents::NAMES_WITH_UNDERSCORES`.
   */
  vtkSetClampMacro(SplitComponentsNamingMode, int, vtkSplitColumnComponents::NUMBERS_WITH_PARENS,
    vtkSplitColumnComponents::NAMES_WITH_UNDERSCORES);
  vtkGetMacro(SplitComponentsNamingMode, int);
  //@}

  //@{
  /**
   * When set (default) the vtkOriginalIndices array will be added to the
   * output. Can be overridden by setting this flag to false.
   */
  vtkSetMacro(GenerateOriginalIds, bool);
  vtkGetMacro(GenerateOriginalIds, bool);
  //@}

  /**
   * Used to identify a node in composite datasets.
   */
  static vtkInformationIntegerKey* COMPOSITE_INDEX();

  //@{
  /**
   * Used to identify a dataset in a hiererchical box dataset.
   */
  static vtkInformationIntegerKey* HIERARCHICAL_LEVEL();
  static vtkInformationIntegerKey* HIERARCHICAL_INDEX();
  //@}

  /**
   * Helper to check if the vtkPartitionedDataSet produced by this filter
   * includes information about composite ids for the input. If not, the input
   * wasn't a composite dataset.
   */
  static bool HasInputCompositeIds(vtkPartitionedDataSet* ptd);

protected:
  vtkDataTabulator();
  ~vtkDataTabulator();

  int FillOutputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkSmartPointer<vtkDataObject> Transform(vtkDataObject* data);

private:
  vtkDataTabulator(const vtkDataTabulator&) = delete;
  void operator=(const vtkDataTabulator&) = delete;

  int FieldAssociation;
  bool GenerateCellConnectivity;
  bool GenerateOriginalIds;
  int SplitComponents;
  int SplitComponentsNamingMode;
  std::set<std::string> Selectors;
  char* ActiveAssemblyForSelectors;
};

#endif
