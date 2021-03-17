/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDataInformation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPVDataInformation
 * @brief provides meta data about a vtkDataObject subclass.
 *
 * vtkPVDataInformation is a vtkPVInformation subclass intended to collect
 * information from vtkDataObject and subclasses. It designed to be used by
 * application in lieu of actual data to glean insight into the data e.g. data
 * type, number of points, number of cells, arrays, ranges etc.
 *
 */

#ifndef vtkPVDataInformation_h
#define vtkPVDataInformation_h

#include "vtkDataObject.h" // for vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES
#include "vtkLegacy.h"     // for VTK_LEGACY
#include "vtkNew.h"        // for vtkNew
#include "vtkPVInformation.h"
#include "vtkRemotingCoreModule.h" //needed for exports
#include "vtkSmartPointer.h"       // for vtkSmartPointer

#include <vector> // for std::vector

class vtkCollection;
class vtkCompositeDataSet;
class vtkDataAssembly;
class vtkDataObject;
class vtkDataSet;
class vtkGenericDataSet;
class vtkGraph;
class vtkHyperTreeGrid;
class vtkInformation;
class vtkPVArrayInformation;
class vtkPVCompositeDataInformation;
class vtkPVDataInformationHelper;
class vtkPVDataSetAttributesInformation;
class vtkSelection;
class vtkTable;

class VTKREMOTINGCORE_EXPORT vtkPVDataInformation : public vtkPVInformation
{
public:
  static vtkPVDataInformation* New();
  vtkTypeMacro(vtkPVDataInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Indicates which output port of an algorithm this data should information
   * should be / has been collected from.
   *
   * Default is 0.
   */
  vtkSetMacro(PortNumber, int);
  vtkGetMacro(PortNumber, int);
  //@}

  //@{
  /**
   * Path if non-null, used to limit blocks from a composite dataset.
   * For details on support form of selector expressions see
   * `vtkDataAssembly::SelectNodes`.
   */
  vtkGetStringMacro(SubsetSelector);
  vtkSetStringMacro(SubsetSelector);
  //@}

  //@{
  /**
   * Get/Set the assembly to use for subsetting. To use hierarchy,
   * set this to `vtkDataAssembly::HierarchyName()` or simply use
   * `SetSubsetAssemblyNameToHierarchy()`.
   */
  vtkSetStringMacro(SubsetAssemblyName);
  vtkGetStringMacro(SubsetAssemblyName);
  void SetSubsetAssemblyNameToHierarchy();
  //@}

  /**
   * Populate vtkPVDataInformation using `object`. The object can be a
   * `vtkDataObject`, `vtkAlgorithm` or `vtkAlgorithmOutput`.
   */
  void CopyFromObject(vtkObject* object) override;

  //@{
  /**
   * vtkPVInformation API implementation.
   */
  void AddInformation(vtkPVInformation* info) override;
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  void CopyParametersToStream(vtkMultiProcessStream&) override;
  void CopyParametersFromStream(vtkMultiProcessStream&) override;
  //@}

  /**
   * Initializes and clears all values updated in `CopyFromObject`.
   */
  void Initialize();

  /**
   * Simply copies from another vtkPVDataInformation.
   */
  void DeepCopy(vtkPVDataInformation* info);

  /**
   * Finds and returns array information associated with the chosen array.
   * `fieldAssociation` must be one of `vtkDataObject::FieldAssociations` or
   * `vtkDataObject::AttributeTypes`.
   */
  vtkPVArrayInformation* GetArrayInformation(const char* arrayname, int fieldAssociation) const;

  /**
   * Returns the data set type. Returned values are defined in `vtkType.h`.
   * For composite datasets, this returns the common superclass for all non-null
   * leaf nodes in the composite dataset.
   *
   * `-1` indicates that the information has not be collected yet,
   * was collected from nullptr data object, or from a composite dataset with
   * no non-null leaf nodes.
   *
   * @sa GetCompositeDataSetType
   */
  vtkGetMacro(DataSetType, int);

  /**
   * Returns the type flag for composite datasets. For non-composite datasets, this is set
   * to `-1`.
   */
  vtkGetMacro(CompositeDataSetType, int);

  /**
   * Returns true if the data information corresponds to a composite dataset.
   */
  bool IsCompositeDataSet() const { return (this->CompositeDataSetType != -1); }

  /**
   * Returns true if the data information is empty or invalid.
   */
  bool IsNull() const { return this->DataSetType == -1 && this->CompositeDataSetType == -1; }

  //@{
  /**
   * For a composite dataset, returns a list of unique data set types for all non-null
   * leaf nodes. May be empty if there are non-null leaf nodes are the dataset is not
   * a composite dataset.
   *
   * @note There is no significance to the order of the types.
   */
  const std::vector<int>& GetUniqueBlockTypes() const { return this->UniqueBlockTypes; }
  unsigned int GetNumberOfUniqueBlockTypes() const
  {
    return static_cast<unsigned int>(this->UniqueBlockTypes.size());
  }
  int GetUniqueBlockType(unsigned int index) const;
  //@}

  //@{
  /**
   * Returns a string for the given type. This directly returns the classname and hence
   * may not be user-friendly.
   *
   * @sa GetPrettyDataTypeString
   */
  static const char* GetDataSetTypeAsString(int type);
  const char* GetDataSetTypeAsString() const
  {
    return vtkPVDataInformation::GetDataSetTypeAsString(this->DataSetType);
  }
  const char* GetDataClassName() const
  {
    return this->DataSetType != -1 ? this->GetDataSetTypeAsString() : nullptr;
  }
  const char* GetCompositeDataClassName() const
  {
    return this->CompositeDataSetType != -1
      ? vtkPVDataInformation::GetDataSetTypeAsString(this->CompositeDataSetType)
      : nullptr;
  }
  //@}

  //@{
  /**
   * Returns a user-friendly string describing the datatype.
   */
  const char* GetPrettyDataTypeString() const;
  static const char* GetPrettyDataTypeString(int dtype);
  //@}

  //@{
  /**
   * Returns true if the type is of the requested `classname` (or `typeId`).
   *
   * If `this->CompositeDataSetType` is valid i.e. the information represents a
   * composite dataset, the composite dataset type is first checked. If not,
   * `this->DataSetType` is checked. Since `this->DataSetType` is set to the
   * most common data type among all the leaf nodes in the composite dataset,
   * true implies all datasets in the composite dataset match the type.
   *
   * @sa `HasDataSetType`
   */
  bool DataSetTypeIsA(const char* classname) const;
  bool DataSetTypeIsA(int typeId) const;
  //@}

  //@{
  /**
   * Similar to `DataSetTypeIsA` except, in case of composite datasets, returns
   * true if any of the leaf nodes match the requested type.
   *
   * For non-composite datasets, this is same as `DataSetTypeIsA`.
   */
  bool HasDataSetType(const char* classname) const;
  bool HasDataSetType(int typeId) const;
  //@}

  /**
   * Returns number of elements of the chosen type in the data. For a composite dataset,
   * this is a sum of the element count across all leaf nodes.
   *
   * `elementType` must be `vtkDataObject::AttributeTypes`. For `vtkDataObject::FIELD`,
   * and `vtkDataObject::POINT_THEN_CELL` returns 0.
   */
  vtkTypeInt64 GetNumberOfElements(int elementType) const;

  //@{
  /**
   * Convenience methods that simply call `GetNumberOfElements` with appropriate element type.
   */
  vtkTypeInt64 GetNumberOfPoints() const { return this->GetNumberOfElements(vtkDataObject::POINT); }
  vtkTypeInt64 GetNumberOfCells() const { return this->GetNumberOfElements(vtkDataObject::CELL); }
  vtkTypeInt64 GetNumberOfVertices() const
  {
    return this->GetNumberOfElements(vtkDataObject::VERTEX);
  }
  vtkTypeInt64 GetNumberOfEdges() const { return this->GetNumberOfElements(vtkDataObject::EDGE); }
  vtkTypeInt64 GetNumberOfRows() const { return this->GetNumberOfElements(vtkDataObject::ROW); }
  //@}

  //@{
  /**
   * vtkHyperTreeGrid / vtkUniformHyperTreeGrid specific properties. Will return 0 for all
   * other data types.
   */
  vtkGetMacro(NumberOfTrees, vtkTypeInt64);
  vtkGetMacro(NumberOfLeaves, vtkTypeInt64);
  //@}

  /**
   * For vtkUniformGridAMR and subclasses, this returns the number of
   * levels
   */
  vtkGetMacro(NumberOfAMRLevels, vtkTypeInt64);

  /**
   * This is count of non-null non-composite datasets.
   * A count of 0 may mean:
   * * the data is nullptr
   * * the data is a composite dataset with no non-null leaf nodes
   * A count of 1 may mean:
   * * the data is non-composite dataset
   * * the data is a composite dataset with exactly 1 non-null leaf node.
   * A count of > 1 may mean:
   * * the data is non-composite, but distributed across ranks.
   * * the data is composite with multiple non-null leaf nodes.
   */
  vtkGetMacro(NumberOfDataSets, vtkTypeInt64);

  /**
   * Returns the memory size as reported by `vtkDataObject::GetActualMemorySize`.
   * For composite datasets, this is simply a sum of the values returned by
   * `vtkDataObject::GetActualMemorySize` per non-null leaf node. For datasets that
   * share internal arrays, this will overestimate the size.
   */
  vtkGetMacro(MemorySize, vtkTypeInt64);

  /**
   * Returns bounds for the dataset. May return an invalid
   * bounding box for data types where bounds don't make sense. For composite datasets,
   * this is a bounding box of all valid bounding boxes.
   */
  vtkGetVector6Macro(Bounds, double);

  //@{
  /**
   * Returns structured extents for the data. These are only valid for structured data types
   * like vtkStructuredGrid, vtkImageData etc. For composite dataset, these represent
   * the combined extents for structured datasets in the collection.
   */
  vtkGetVector6Macro(Extent, int);
  //@}

  //@{
  /**
   * Get vtkPVDataSetAttributesInformation information for the given association.
   * `vtkPVDataSetAttributesInformation` can be used to determine available arrays and get meta-data
   * about each of the available arrays.
   *
   * `fieldAssociation` must be `vtkDataObject::FieldAssociations` (or
   * `vtkDataObject::AttributeTypes`). vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS (or
   * vtkDataObject::POINT_THEN_CELL) is not supported.
   */
  vtkPVDataSetAttributesInformation* GetAttributeInformation(int fieldAssociation) const;
  //@}

  //@{
  /**
   * Convenience methods to get vtkPVDataSetAttributesInformation for specific
   * field type. Same as calling `GetAttributeInformation` with appropriate type.
   */
  vtkPVDataSetAttributesInformation* GetPointDataInformation() const
  {
    return this->GetAttributeInformation(vtkDataObject::POINT);
  }
  vtkPVDataSetAttributesInformation* GetCellDataInformation() const
  {
    return this->GetAttributeInformation(vtkDataObject::CELL);
  }
  vtkPVDataSetAttributesInformation* GetVertexDataInformation() const
  {
    return this->GetAttributeInformation(vtkDataObject::VERTEX);
  }
  vtkPVDataSetAttributesInformation* GetEdgeDataInformation() const
  {
    return this->GetAttributeInformation(vtkDataObject::EDGE);
  }
  vtkPVDataSetAttributesInformation* GetRowDataInformation() const
  {
    return this->GetAttributeInformation(vtkDataObject::ROW);
  }
  vtkPVDataSetAttributesInformation* GetFieldDataInformation() const
  {
    return this->GetAttributeInformation(vtkDataObject::FIELD);
  }
  //@}

  /**
   * For a vtkPointSet and subclasses, this provides information about the `vtkPoints`
   * array. For composite datasets, this is the obtained by accumulating
   * information from all point-sets in the collection.
   */
  vtkGetObjectMacro(PointArrayInformation, vtkPVArrayInformation);

  //@{
  /**
   * Returns `DATA_TIME_STEP`, if any, provided by the information object associated
   * with data.
   */
  vtkGetMacro(HasTime, bool);
  vtkGetMacro(Time, double);
  //@}

  //@{
  /**
   * Strictly speaking, these are not data information since these cannot be obtained
   * from the data but from the pipeline producing the data. However, they are provided
   * as part data information for simplicity.
   */
  vtkGetVector2Macro(TimeRange, double);
  vtkGetMacro(NumberOfTimeSteps, vtkTypeInt64);
  vtkGetMacro(TimeLabel, std::string);
  //@}

  /**
   * Returns true if the data is structured i.e. supports i-j-k indexing. For composite
   * datasets, this returns true if all leaf nodes are structured.
   */
  bool IsDataStructured() const;

  /**
   * Returns true if the data has structured data. For composite datasets, this
   * returns true if *any* of the datasets are structured.
   */
  bool HasStructuredData() const;

  /**
   * Returns true if the data has unstructured data. For composite datasets, this returns
   * true if *any* of the datasets are unstructured.
   */
  bool HasUnstructuredData() const;

  /**
   * Returns true if provided fieldAssociation is valid for this dataset,
   * false otherwise. For composite datasets, this returns true if the attribute type if
   * valid for any of the datasets in the collection.
   */
  bool IsAttributeValid(int fieldAssociation) const;

  /**
   * Returns the extent type for a given data-object type.
   */
  static int GetExtentType(int dataType);

  //@{
  /**
   * For composite datasets, this provides access to the information about
   * the dataset hierarchy and any data assembly associated with it.
   *
   * Hierarchy represents the hierarchy of the datatype while assembly
   * provides access to logical grouping and organization, if any.
   */
  vtkDataAssembly* GetHierarchy() const;
  vtkDataAssembly* GetDataAssembly() const;
  vtkDataAssembly* GetDataAssembly(const char* assemblyName) const;
  //@}

  /**
   * Until multiblock dataset is deprecated, applications often want to locate
   * the composite-index for the first leaf node.
   *
   * Returns 0 if not applicable.
   */
  vtkGetMacro(FirstLeafCompositeIndex, vtkTypeUInt64);

  /**
   * Given a composite index, return the blockname, if any.
   * Note, this is not reliable if executed on partitioned-dataset collection
   * since partitioned-dataset collection may have different composite indices
   * across ranks. Thus, this must only be used in code that is currently only
   * slated for migration from using composite-ids to selectors and hence does
   * not encounter partitioned-dataset collections.
   *
   * This uses `Hierarchy` and hence it only supported when hierarchy is
   * defined/available.
   *
   * Returns empty string if no name available or cannot be determined.
   */
  std::string GetBlockName(vtkTypeUInt64 cid) const;

  //@{
  /**
   * Deprecated in ParaView 5.10
   */
  VTK_LEGACY(vtkTypeUInt64 GetPolygonCount());
  VTK_LEGACY(void* GetCompositeDataInformation());
  VTK_LEGACY(vtkPVDataInformation* GetDataInformationForCompositeIndex(int));
  VTK_LEGACY(unsigned int GetNumberOfBlockLeafs(bool skipEmpty));
  VTK_LEGACY(vtkPVDataInformation* GetDataInformationForCompositeIndex(int*));
  VTK_LEGACY(double* GetTimeSpan());
  VTK_LEGACY(void GetTimeSpan(double&, double&));
  VTK_LEGACY(void GetTimeSpan(double[2]));
  VTK_LEGACY(static void RegisterHelper(const char*, const char*));
  //@}

protected:
  vtkPVDataInformation();
  ~vtkPVDataInformation() override;

  /**
   * Populate information object with contents from producer pipeline's
   * output port information.
   */
  void CopyFromPipelineInformation(vtkInformation* pipelineInfo);
  void CopyFromDataObject(vtkDataObject* dobj);
  friend class vtkPVDataInformationHelper;

  /**
   * Extracts selected blocks. Use SubsetSelector and SubsetAssemblyName to extract
   * chosen part of the composite dataset and returns that.
   */
  vtkSmartPointer<vtkDataObject> GetSubset(vtkDataObject* dobj) const;

private:
  vtkPVDataInformation(const vtkPVDataInformation&) = delete;
  void operator=(const vtkPVDataInformation&) = delete;

  int PortNumber = -1;
  char* SubsetSelector = nullptr;
  char* SubsetAssemblyName = nullptr;

  int DataSetType = -1;
  int CompositeDataSetType = -1;
  vtkTypeUInt64 FirstLeafCompositeIndex = 0;
  vtkTypeInt64 NumberOfTrees = 0;
  vtkTypeInt64 NumberOfLeaves = 0;
  vtkTypeInt64 NumberOfAMRLevels = 0;
  vtkTypeInt64 NumberOfDataSets = 0;
  vtkTypeInt64 MemorySize = 0;
  double Bounds[6] = { VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX,
    VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX };
  int Extent[6] = { VTK_INT_MAX, -VTK_INT_MAX, VTK_INT_MAX, -VTK_INT_MAX, VTK_INT_MAX,
    -VTK_INT_MAX };
  ;
  bool HasTime = false;
  double Time = 0.0;
  double TimeRange[2] = { VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX };
  std::string TimeLabel;
  vtkTypeInt64 NumberOfTimeSteps = 0;

  std::vector<int> UniqueBlockTypes;
  vtkTypeInt64 NumberOfElements[vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES] = { 0, 0, 0, 0, 0, 0, 0 };
  vtkNew<vtkPVDataSetAttributesInformation>
    AttributeInformations[vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES];
  vtkNew<vtkPVArrayInformation> PointArrayInformation;

  vtkNew<vtkDataAssembly> Hierarchy;
  vtkNew<vtkDataAssembly> DataAssembly;

  friend class vtkPVDataInformationAccumulator;
};

#endif
