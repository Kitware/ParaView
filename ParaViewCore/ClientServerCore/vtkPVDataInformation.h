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
// .NAME vtkPVDataInformation - Light object for holding data information.
// .SECTION Description
// This object is a light weight object.  It has no user interface and
// does not necessarily last a long time.  It is meant to help collect
// information about data object and collections of data objects.  It
// has a PV in the class name because it should never be moved into
// VTK.
// 
// .SECTION Caveats
// Get polygons only works for poly data and it does not work propelry for the
// triangle strips.

#ifndef __vtkPVDataInformation_h
#define __vtkPVDataInformation_h

#include "vtkPVInformation.h"

class vtkCollection;
class vtkCompositeDataSet;
class vtkDataObject;
class vtkDataSet;
class vtkGenericDataSet;
class vtkGraph;
class vtkPVArrayInformation;
class vtkPVCompositeDataInformation;
class vtkPVDataSetAttributesInformation;
class vtkSelection;
class vtkTable;

class VTK_EXPORT vtkPVDataInformation : public vtkPVInformation
{
public:
  static vtkPVDataInformation* New();
  vtkTypeMacro(vtkPVDataInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Port number controls which output port the information is gathered from.
  // This is the only parameter that can be set on  the client-side before
  // gathering the information.
  vtkSetMacro(PortNumber, int);
  vtkGetMacro(PortNumber, int);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object. Calls AddInformation(info, 0).
  virtual void AddInformation(vtkPVInformation* info);

  // Description:
  // Merge another information object. If adding information of
  // 1 part across processors, set addingParts to false. If
  // adding information of parts, set addingParts to true.
  virtual void AddInformation(vtkPVInformation*, int addingParts);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // Serialize/Deserialize the parameters that control how/what information is
  // gathered. This are different from the ivars that constitute the gathered
  // information itself. For example, PortNumber on vtkPVDataInformation
  // controls what output port the data-information is gathered from.
  virtual void CopyParametersToStream(vtkMultiProcessStream&);
  virtual void CopyParametersFromStream(vtkMultiProcessStream&);

  // Description:
  // Remove all information.  The next add will be like a copy.
  // I might want to put this in the PVInformation superclass.
  void Initialize();

  // Description:
  // Access to information.
  vtkGetMacro(DataSetType, int);
  vtkGetMacro(CompositeDataSetType, int);
  const char *GetDataSetTypeAsString();
  int DataSetTypeIsA(const char* type);
  vtkGetMacro(NumberOfPoints, vtkTypeInt64);
  vtkGetMacro(NumberOfCells, vtkTypeInt64);
  vtkGetMacro(NumberOfRows, vtkTypeInt64);
  vtkGetMacro(MemorySize, int);
  vtkGetMacro(PolygonCount, int);
  vtkGetMacro(NumberOfDataSets, int);
  vtkGetVector6Macro(Bounds, double);
  
  // Description:
  // Returns a string describing the datatype that can be directly
  // shown in a user interface.
  const char* GetPrettyDataTypeString();

  // Description:
  // Of course Extent is only valid for structured data sets.
  // Extent is the largest extent that contains all the parts.
  vtkGetVector6Macro(Extent, int);

  // Description:
  // Access to information about points. Only valid for subclasses
  // of vtkPointSet.
  vtkGetObjectMacro(PointArrayInformation,vtkPVArrayInformation);

  // Description:
  // Access to information about point/cell/vertex/edge/row data.
  vtkGetObjectMacro(PointDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(CellDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(VertexDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(EdgeDataInformation, vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(RowDataInformation, vtkPVDataSetAttributesInformation);

  // Description:
  // Accesse to information about field data, if any.
  vtkGetObjectMacro(FieldDataInformation,vtkPVDataSetAttributesInformation);

  // Description:
  // Method to access vtkPVDataSetAttributesInformation using field association
  // type.
  // \c fieldAssociation can be vtkDataObject::FIELD_ASSOCIATION_POINTS,
  // vtkDataObject::FIELD_ASSOCIATION_CELLS etc.
  // (use vtkDataObject::FIELD_ASSOCIATION_NONE for field data).
  vtkPVDataSetAttributesInformation* GetAttributeInformation(int fieldAssociation);

  // Description:
  // If data is composite, this provides information specific to
  // composite datasets.
  vtkGetObjectMacro(CompositeDataInformation,vtkPVCompositeDataInformation);

  // Description:
  // Given the flat-index for a node in a composite dataset, this method returns
  // the data information for the node, it available.
  vtkPVDataInformation* GetDataInformationForCompositeIndex(int index);

  // Description:
  // This is same as GetDataInformationForCompositeIndex() however note that the
  // index will get modified in this method.
  vtkPVDataInformation* GetDataInformationForCompositeIndex(int* index);

  // Description:
  // ClassName of the data represented by information object.
  vtkGetStringMacro(DataClassName);

  // Description:
  // The least common class name of composite dataset blocks
  vtkGetStringMacro(CompositeDataClassName);

  vtkGetVector2Macro(TimeSpan, double);

  // Description:
  // Returns if the Time is set.
  vtkGetMacro(HasTime, int);

  // Description:
  // Returns the data time if, GetHasTime() return true.
  vtkGetMacro(Time, double);

  // Description:
  // Returns if the data type is structured.
  int IsDataStructured();

protected:
  vtkPVDataInformation();
  ~vtkPVDataInformation();

  void DeepCopy(vtkPVDataInformation *dataInfo, bool copyCompositeInformation=true);

  void AddFromMultiPieceDataSet(vtkCompositeDataSet* data);
  void CopyFromCompositeDataSet(vtkCompositeDataSet* data);
  virtual void CopyFromDataSet(vtkDataSet* data);
  void CopyFromGenericDataSet(vtkGenericDataSet *data);
  void CopyFromGraph(vtkGraph* graph);
  void CopyFromTable(vtkTable* table);
  void CopyFromSelection(vtkSelection* selection);
  void CopyCommonMetaData(vtkDataObject*);

  // Data information collected from remote processes.
  int            DataSetType;
  int            CompositeDataSetType;
  int            NumberOfDataSets;
  vtkTypeInt64   NumberOfPoints;
  vtkTypeInt64   NumberOfCells;
  vtkTypeInt64   NumberOfRows;
  int            MemorySize;
  vtkIdType      PolygonCount;
  double         Bounds[6];
  int            Extent[6];
  double         TimeSpan[2];
  double         Time;
  int            HasTime;

  char*          DataClassName;
  vtkSetStringMacro(DataClassName);

  char*          CompositeDataClassName;
  vtkSetStringMacro(CompositeDataClassName);

  vtkPVDataSetAttributesInformation* PointDataInformation;
  vtkPVDataSetAttributesInformation* CellDataInformation;
  vtkPVDataSetAttributesInformation* FieldDataInformation;
  vtkPVDataSetAttributesInformation* VertexDataInformation;
  vtkPVDataSetAttributesInformation* EdgeDataInformation;
  vtkPVDataSetAttributesInformation* RowDataInformation;

  vtkPVCompositeDataInformation* CompositeDataInformation;

  vtkPVArrayInformation* PointArrayInformation;

private:
  vtkPVDataInformation(const vtkPVDataInformation&); // Not implemented
  void operator=(const vtkPVDataInformation&); // Not implemented

  int PortNumber;
};

#endif
