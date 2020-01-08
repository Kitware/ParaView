/*=========================================================================

  Program:   ParaView
  Module:    vtkCPInputDataDescription.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPInputDataDescription_h
#define vtkCPInputDataDescription_h

class vtkDataObject;
class vtkDataSet;
class vtkFieldData;

#include "vtkObject.h"
#include "vtkPVCatalystModule.h" // For windows import/export of shared libraries

/// @ingroup CoProcessing
/// This class provides the data description for each input for the coprocessor
/// pipelines.
class VTKPVCATALYST_EXPORT vtkCPInputDataDescription : public vtkObject
{
public:
  static vtkCPInputDataDescription* New();
  vtkTypeMacro(vtkCPInputDataDescription, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Reset the names of the fields that are needed.
  void Reset();

  // Description:
  // Add in a name of an array with name *fieldName* of type *type*
  // where the values comes from vtkDataObject::AttributeTypes.
  void AddField(const char* fieldName, int type);

  // Description:
  // Get the number of fields currently specified in this object.
  unsigned int GetNumberOfFields();

  // Description:
  // Get the name of the field given its current index.
  const char* GetFieldName(unsigned int fieldIndex);

  // Description:
  // Get the type of field given its current index. The types are defined
  // in vtkDataObject::AttributeTypes. A return value of -1 indicates an invalid
  // fieldIndex.
  int GetFieldType(unsigned int fieldIndex);

  // Description:
  // Return true if a field with fieldName is needed.
  bool IsFieldNeeded(const char* fieldName, int type);

  // Description:
  // When set to true, all fields are requested. Off by default.
  // Note that calling Reset() resets this flag to Off as well.
  vtkSetMacro(AllFields, bool);
  vtkGetMacro(AllFields, bool);
  vtkBooleanMacro(AllFields, bool);

  // Description:
  // Use this to enable the mesh. Off by default. Note that calling Reset()
  // resets this flag to Off as well.
  vtkSetMacro(GenerateMesh, bool);
  vtkGetMacro(GenerateMesh, bool);
  vtkBooleanMacro(GenerateMesh, bool);

  // Description:
  // Set the grid input for coprocessing.  The grid should have all of
  // the point data and cell data properly set.
  void SetGrid(vtkDataObject* grid);

  // Description:
  // Get the grid for coprocessing.
  vtkGetObjectMacro(Grid, vtkDataObject);

  // Description:
  // Returns true if the grid is necessary..
  bool GetIfGridIsNecessary();

  // Description:
  // Set/get the extents for a partitioned topologically regular grid
  // (i.e. vtkUniformGrid, vtkImageData, vtkRectilinearGrid, and
  // vtkStructuredGrid).
  vtkSetVector6Macro(WholeExtent, int);
  vtkGetVector6Macro(WholeExtent, int);

  // Description:
  // Shallow copy.
  void ShallowCopy(vtkCPInputDataDescription*);

protected:
  vtkCPInputDataDescription();
  ~vtkCPInputDataDescription() override;

  // Description:
  // On when all fields must be requested for the coprocessing pipeline.
  bool AllFields;

  // Description:
  // On when the mesh should be generated.
  bool GenerateMesh;

  // Description:
  // The grid for coprocessing. The grid is not owned by the object.
  vtkDataObject* Grid;

private:
  vtkCPInputDataDescription(const vtkCPInputDataDescription&) = delete;
  void operator=(const vtkCPInputDataDescription&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
  int WholeExtent[6];
};

#endif
