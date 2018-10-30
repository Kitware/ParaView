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
 * @class   vtkPVDataInformation
 * @brief   Light object for holding data information.
 *
 * This object is a light weight object.  It has no user interface and
 * does not necessarily last a long time.  It is meant to help collect
 * information about data object and collections of data objects.  It
 * has a PV in the class name because it should never be moved into
 * VTK.
 *
 * @warning
 * Get polygons only works for poly data and it does not work propelry for the
 * triangle strips.
*/

#ifndef vtkPVDataInformation_h
#define vtkPVDataInformation_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkPVInformation.h"

class vtkCollection;
class vtkCompositeDataSet;
class vtkDataObject;
class vtkDataSet;
class vtkGenericDataSet;
class vtkGraph;
class vtkHyperTreeGrid;
class vtkInformation;
class vtkPVArrayInformation;
class vtkPVCompositeDataInformation;
class vtkPVDataSetAttributesInformation;
class vtkPVDataInformationHelper;
class vtkSelection;
class vtkTable;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPVDataInformation : public vtkPVInformation
{
public:
  static vtkPVDataInformation* New();
  vtkTypeMacro(vtkPVDataInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Method to find and return attribute array information for a particular
   * array for the given attribute type if one exists.
   * Returns NULL if none is found.
   * \c fieldAssociation can be vtkDataObject::FIELD_ASSOCIATION_POINTS,
   * vtkDataObject::FIELD_ASSOCIATION_CELLS etc.
   * (use vtkDataObject::FIELD_ASSOCIATION_NONE for field data) (or
   * vtkDataObject::POINT, vtkDataObject::CELL, vtkDataObject::FIELD).
   */
  vtkPVArrayInformation* GetArrayInformation(const char* arrayname, int fieldAssociation);

  //@{
  /**
   * Port number controls which output port the information is gathered from.
   * This is the only parameter that can be set on  the client-side before
   * gathering the information.
   */
  vtkSetMacro(PortNumber, int);
  vtkGetMacro(PortNumber, int);
  //@}

  /**
   * Transfer information about a single object into this object.
   */
  void CopyFromObject(vtkObject*) override;

  /**
   * Merge another information object. Calls AddInformation(info, 0).
   */
  void AddInformation(vtkPVInformation* info) override;

  /**
   * Merge another information object. If adding information of
   * 1 part across processors, set addingParts to false. If
   * adding information of parts, set addingParts to true.
   */
  virtual void AddInformation(vtkPVInformation*, int addingParts);

  //@{
  /**
   * Manage a serialized version of the information.
   */
  void CopyToStream(vtkClientServerStream*) override;
  void CopyFromStream(const vtkClientServerStream*) override;
  //@}

  //@{
  /**
   * Serialize/Deserialize the parameters that control how/what information is
   * gathered. This are different from the ivars that constitute the gathered
   * information itself. For example, PortNumber on vtkPVDataInformation
   * controls what output port the data-information is gathered from.
   */
  void CopyParametersToStream(vtkMultiProcessStream&) override;
  void CopyParametersFromStream(vtkMultiProcessStream&) override;
  //@}

  /**
   * Remove all information.  The next add will be like a copy.
   * I might want to put this in the PVInformation superclass.
   */
  void Initialize();

  //@{
  /**
   * Access to information.
   */
  vtkGetMacro(DataSetType, int);
  vtkGetMacro(CompositeDataSetType, int);
  const char* GetDataSetTypeAsString();
  bool DataSetTypeIsA(const char* type);
  vtkGetMacro(NumberOfPoints, vtkTypeInt64);
  vtkGetMacro(NumberOfCells, vtkTypeInt64);
  vtkGetMacro(NumberOfRows, vtkTypeInt64);
  vtkGetMacro(NumberOfTrees, vtkTypeInt64);
  vtkGetMacro(NumberOfVertices, vtkTypeInt64);
  vtkGetMacro(NumberOfEdges, vtkTypeInt64);
  vtkGetMacro(NumberOfLeaves, vtkTypeInt64);
  vtkGetMacro(MemorySize, int);
  vtkGetMacro(PolygonCount, int);
  vtkGetMacro(NumberOfDataSets, int);
  vtkGetVector6Macro(Bounds, double);
  //@}

  /**
   * Returns the number of elements of the given type where type can
   * vtkDataObject::POINT, vtkDataObject::CELL, ... etc.
   */
  vtkTypeInt64 GetNumberOfElements(int type);

  /**
   * Returns a string describing the datatype that can be directly
   * shown in a user interface.
   */
  const char* GetPrettyDataTypeString();

  //@{
  /**
   * Of course Extent is only valid for structured data sets.
   * Extent is the largest extent that contains all the parts.
   */
  vtkGetVector6Macro(Extent, int);
  //@}

  //@{
  /**
   * Access to information about points. Only valid for subclasses
   * of vtkPointSet.
   */
  vtkGetObjectMacro(PointArrayInformation, vtkPVArrayInformation);
  //@}

  //@{
  /**
   * Access to information about point/cell/vertex/edge/row data.
   */
  vtkGetObjectMacro(PointDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(CellDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(VertexDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(EdgeDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(RowDataInformation, vtkPVDataSetAttributesInformation);
  //@}

  //@{
  /**
   * Accesse to information about field data, if any.
   */
  vtkGetObjectMacro(FieldDataInformation, vtkPVDataSetAttributesInformation);
  //@}

  /**
   * Method to access vtkPVDataSetAttributesInformation using field association
   * type.
   * \c fieldAssociation can be vtkDataObject::FIELD_ASSOCIATION_POINTS,
   * vtkDataObject::FIELD_ASSOCIATION_CELLS etc.
   * (use vtkDataObject::FIELD_ASSOCIATION_NONE for field data).
   */
  vtkPVDataSetAttributesInformation* GetAttributeInformation(int fieldAssociation);

  //@{
  /**
   * If data is composite, this provides information specific to
   * composite datasets.
   */
  vtkGetObjectMacro(CompositeDataInformation, vtkPVCompositeDataInformation);
  //@}

  /**
   * Given the flat-index for a node in a composite dataset, this method returns
   * the data information for the node, it available.
   */
  vtkPVDataInformation* GetDataInformationForCompositeIndex(int index);

  /**
   * Compute the number of block leaf from this information
   * multipieces are counted as single block.
   * The boolean skipEmpty parameter allows to choose to count empty dataset are not
   * Calling this method with skipEmpty to false will correspond to the vtkBlockColors array
   * in a multiblock.
   */
  unsigned int GetNumberOfBlockLeafs(bool skipEmpty);

  /**
   * This is same as GetDataInformationForCompositeIndex() however note that the
   * index will get modified in this method.
   */
  vtkPVDataInformation* GetDataInformationForCompositeIndex(int* index);

  //@{
  /**
   * ClassName of the data represented by information object.
   */
  vtkGetStringMacro(DataClassName);
  //@}

  //@{
  /**
   * The least common class name of composite dataset blocks
   */
  vtkGetStringMacro(CompositeDataClassName);
  //@}

  vtkGetVector2Macro(TimeSpan, double);

  //@{
  /**
   * Returns if the Time is set.
   */
  vtkGetMacro(HasTime, int);
  //@}

  //@{
  /**
   * Returns the data time if, GetHasTime() return true.
   */
  vtkGetMacro(Time, double);
  //@}

  //@{
  /**
   * Returns the number of time steps.
   */
  vtkGetMacro(NumberOfTimeSteps, int);
  //@}

  //@{
  /**
   * Returns the label that should be used instead of "Time" if any.
   */
  vtkGetStringMacro(TimeLabel);
  //@}

  /**
   * Returns if the data type is structured.
   */
  bool IsDataStructured();

  /**
   * Returns true if provided fieldAssociation is valid for this dataset, false otherwise.
   * Always returns true for composite datasets.
   * eg, FIELD_ASSOCIATION_EDGES will return false for a vtkPolyData, true for a vtkGraph.
   */
  bool IsAttributeValid(int fieldAssociation);

  //@{
  /**
   * If this instance of vtkPVDataInformation summarizes a node in a
   * composite-dataset, and if that node has been given a label in that
   * composite dataset (using vtkCompositeDataSet::NAME meta-data), then this
   * will return that name. Returns NULL if this instance doesn't represent a
   * node in a composite dataset or doesn't have a label/name associated with
   * it.
   */
  vtkGetStringMacro(CompositeDataSetName);
  //@}

  /**
   * Allows run time addition of information getters for new classes
   */
  static void RegisterHelper(const char* classname, const char* helperclassname);

protected:
  vtkPVDataInformation();
  ~vtkPVDataInformation() override;

  void DeepCopy(vtkPVDataInformation* dataInfo, bool copyCompositeInformation = true);

  void AddFromMultiPieceDataSet(vtkCompositeDataSet* data);
  void CopyFromCompositeDataSet(vtkCompositeDataSet* data);
  void CopyFromCompositeDataSetInitialize(vtkCompositeDataSet* data);
  void CopyFromCompositeDataSetFinalize(vtkCompositeDataSet* data);
  virtual void CopyFromDataSet(vtkDataSet* data);
  void CopyFromGenericDataSet(vtkGenericDataSet* data);
  void CopyFromGraph(vtkGraph* graph);
  void CopyFromTable(vtkTable* table);
  void CopyFromHyperTreeGrid(vtkHyperTreeGrid* data);
  void CopyFromSelection(vtkSelection* selection);
  void CopyCommonMetaData(vtkDataObject*, vtkInformation*);

  static vtkPVDataInformationHelper* FindHelper(const char* classname);

  // Data information collected from remote processes.
  int DataSetType = -1;
  int CompositeDataSetType = -1;
  int NumberOfDataSets = 0;
  vtkTypeInt64 NumberOfPoints = 0; // data sets
  vtkTypeInt64 NumberOfCells = 0;
  vtkTypeInt64 NumberOfRows = 0;  // tables
  vtkTypeInt64 NumberOfTrees = 0; // hypertreegrids
  vtkTypeInt64 NumberOfVertices = 0;
  vtkTypeInt64 NumberOfEdges = 0; // graphs
  vtkTypeInt64 NumberOfLeaves = 0;
  int MemorySize = 0;
  vtkIdType PolygonCount = 0;
  double Bounds[6] = { VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX, VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX,
    VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX };
  int Extent[6] = { VTK_INT_MAX, -VTK_INT_MAX, VTK_INT_MAX, -VTK_INT_MAX, VTK_INT_MAX,
    -VTK_INT_MAX };
  double TimeSpan[2] = { VTK_DOUBLE_MAX, -VTK_DOUBLE_MAX };
  double Time = 0.0;
  int HasTime = 0;
  int NumberOfTimeSteps = 0;

  char* DataClassName = nullptr;
  vtkSetStringMacro(DataClassName);

  char* TimeLabel = nullptr;
  vtkSetStringMacro(TimeLabel);

  char* CompositeDataClassName = nullptr;
  vtkSetStringMacro(CompositeDataClassName);

  char* CompositeDataSetName = nullptr;
  vtkSetStringMacro(CompositeDataSetName);

  vtkPVDataSetAttributesInformation* PointDataInformation;
  vtkPVDataSetAttributesInformation* CellDataInformation;
  vtkPVDataSetAttributesInformation* FieldDataInformation;
  vtkPVDataSetAttributesInformation* VertexDataInformation;
  vtkPVDataSetAttributesInformation* EdgeDataInformation;
  vtkPVDataSetAttributesInformation* RowDataInformation;

  vtkPVCompositeDataInformation* CompositeDataInformation;

  vtkPVArrayInformation* PointArrayInformation;

  friend class vtkPVDataInformationHelper;
  friend class vtkPVCompositeDataInformation;

private:
  vtkPVDataInformation(const vtkPVDataInformation&) = delete;
  void operator=(const vtkPVDataInformation&) = delete;

  int PortNumber = -1;
};

#endif
