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

#ifndef __vtkPVDataInformation_h
#define __vtkPVDataInformation_h

#include "vtkPVInformation.h"
#include "vtkTypeFromNative.h" // This is required to get the 64 bit int definition.

class vtkCollection;
class vtkCompositeDataSet;
class vtkDataSet;
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
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation*);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*) const;
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // Remove all information.  The next add will be like a copy.
  // I might want to put this in the PVInformation superclass.
  void Initialize();

  // Description:
  // Access to information.
  vtkGetMacro(DataSetType, int);
  const char *GetDataSetTypeAsString();
  int DataSetTypeIsA(const char* type);
  vtkGetMacro(NumberOfPoints, vtkTypeInt64);
  vtkGetMacro(NumberOfCells, vtkTypeInt64);
  vtkGetMacro(MemorySize, int);
  vtkGetMacro(NumberOfDataSets, int);
  vtkGetVector6Macro(Bounds, double);

  // Description:
  // Of course Extent is only valid for structured data sets.
  // Extent is the largest extent that contains all the parts.
  vtkGetVector6Macro(Extent, int);

  // Description:
  // Access to information about point and cell data.
  vtkGetObjectMacro(PointDataInformation,vtkPVDataSetAttributesInformation);
  vtkGetObjectMacro(CellDataInformation,vtkPVDataSetAttributesInformation);

  // Description:
  // Name stored in field data.
  vtkGetStringMacro(Name);

  // Description:
  // We allow the name to be set so paraview can set a default value
  // if the data has no name.
  vtkSetStringMacro(Name);

  // Description:
  // ClassName of the data represented by information object.
  vtkGetStringMacro(DataClassName);

protected:
  vtkPVDataInformation();
  ~vtkPVDataInformation();

  void DeepCopy(vtkPVDataInformation *dataInfo);

  void CopyFromCompositeDataSet(vtkCompositeDataSet* data);
  void CopyFromDataSet(vtkDataSet* data);
  void CopyFromGenericDataSet(vtkGenericDataSet *data);
  
  // Data information collected from remote processes.
  int            DataSetType;
  int            NumberOfDataSets;
  vtkTypeInt64   NumberOfPoints;
  vtkTypeInt64   NumberOfCells;
  int            MemorySize;
  double         Bounds[6];
  int            Extent[6];

  char*          Name;

  char*          DataClassName;
  vtkSetStringMacro(DataClassName);

  vtkPVDataSetAttributesInformation* PointDataInformation;
  vtkPVDataSetAttributesInformation* CellDataInformation;

private:
  vtkPVDataInformation(const vtkPVDataInformation&); // Not implemented
  void operator=(const vtkPVDataInformation&); // Not implemented
};

#endif
