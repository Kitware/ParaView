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
class vtkDataSet;
class vtkPVArrayInformation;
class vtkPVCompositeDataInformation;
class vtkPVDataSetAttributesInformation;
class vtkGenericDataSet;

class VTK_EXPORT vtkPVDataInformation : public vtkPVInformation
{
public:
  static vtkPVDataInformation* New();
  vtkTypeRevisionMacro(vtkPVDataInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  vtkGetMacro(MemorySize, int);
  vtkGetMacro(PolygonCount, int);
  vtkGetMacro(NumberOfDataSets, int);
  vtkGetVector6Macro(Bounds, double);

  // Description:
  // Of course Extent is only valid for structured data sets.
  // Extent is the largest extent that contains all the parts.
  vtkGetVector6Macro(Extent, int);

  // Description:
  // Access to information about points. Only valid for subclasses
  // of vtkPointSet.
  vtkGetObjectMacro(PointArrayInformation,vtkPVArrayInformation);

  // Description:
  // Access to information about point and cell data.
  vtkGetObjectMacro(PointDataInformation,vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(CellDataInformation,vtkPVDataSetAttributesInformation);

  // Description:
  // If data is composite, this provides information specific to
  // composite datasets.
  vtkGetObjectMacro(CompositeDataInformation,vtkPVCompositeDataInformation);

  // Description:
  // Name stored in field data.
  const char* GetName();

  // Description:
  // We allow the name to be set so paraview can set a default value
  // if the data has no name.
  void SetName(const char* name);

  // Description:
  // ClassName of the data represented by information object.
  vtkGetStringMacro(DataClassName);

  // Description:
  // The least common class name of composite dataset blocks
  vtkGetStringMacro(CompositeDataClassName);

protected:
  vtkPVDataInformation();
  ~vtkPVDataInformation();

  void DeepCopy(vtkPVDataInformation *dataInfo);

//BTX
  friend class vtkPVCompositeDataInformation;
//ETX

  int AddFromCompositeDataSet(vtkCompositeDataSet* data);
  void CopyFromCompositeDataSet(vtkCompositeDataSet* data, 
                                int recurse=1);
  void CopyFromDataSet(vtkDataSet* data);
  void CopyFromGenericDataSet(vtkGenericDataSet *data);

  // Data information collected from remote processes.
  int            DataSetType;
  int            CompositeDataSetType;
  int            NumberOfDataSets;
  vtkTypeInt64   NumberOfPoints;
  vtkTypeInt64   NumberOfCells;
  int            MemorySize;
  vtkIdType      PolygonCount;
  double         Bounds[6];
  int            Extent[6];

  char*          Name;

  char*          DataClassName;
  vtkSetStringMacro(DataClassName);

  char*          CompositeDataClassName;
  vtkSetStringMacro(CompositeDataClassName);

  vtkPVDataSetAttributesInformation* PointDataInformation;
  vtkPVDataSetAttributesInformation* CellDataInformation;

  vtkPVCompositeDataInformation* CompositeDataInformation;

  vtkPVArrayInformation* PointArrayInformation;

private:
  int NameSetToDefault;

  vtkPVDataInformation(const vtkPVDataInformation&); // Not implemented
  void operator=(const vtkPVDataInformation&); // Not implemented
};

#endif
