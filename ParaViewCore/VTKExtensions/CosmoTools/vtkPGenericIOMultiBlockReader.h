/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPGenericIOMultiBlockReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkPGenericIOMultiBlockReader.h -- Read GenericIO formatted data
//
// .SECTION Description
//  Creates a vtkMultiBlockDataSet from a GenericIO file

#include "vtkPVVTKExtensionsCosmoToolsModule.h"
#include "vtkMultiBlockDataSetAlgorithm.h" // parent class

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkMultiProcessController;
class vtkStringArray;
class vtkUnstructuredGrid;
class vtkInformationDoubleKey;

// GenericIO forward declarations
namespace gio
{
  class GenericIOReader;
}

enum IOType {
  IOTYPEMPI,
  IOTYPEPOSIX
};

enum BlockAssignment {
  ROUND_ROBIN,
  RCB
};

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPGenericIOMultiBlockReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkPGenericIOMultiBlockReader* New();
  vtkTypeMacro(vtkPGenericIOMultiBlockReader,vtkMultiBlockDataSetAlgorithm)
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Set/Get the variable name to be used as the x-axis for plotting particles.
  vtkSetStringMacro(XAxisVariableName)
  vtkGetStringMacro(XAxisVariableName)

  // Description:
  // Set/Get the variable name to be used as the y-axis for plotting particles.
  vtkSetStringMacro(YAxisVariableName)
  vtkGetStringMacro(YAxisVariableName)

  // Description:
  // Set/Get the variable name to be used as the z-axis for plotting particles.
  vtkSetStringMacro(ZAxisVariableName)
  vtkGetStringMacro(ZAxisVariableName)

  // Description:
  // Specify the name of the cosmology particle binary file to read
  vtkSetStringMacro(FileName)
  vtkGetStringMacro(FileName)

  // Description:
  // Set/Get the underlying IO method the reader will employ, i.e., MPI or POSIX.
  vtkSetMacro(GenericIOType,int)
  vtkGetMacro(GenericIOType,int)

  // Description:
  // Set/Get the underlying block-assignment strategy to use, i.e., ROUND_ROBIN,
  // or RCB.
  vtkSetMacro(BlockAssignment,int)
  vtkGetMacro(BlockAssignment,int)

  // Description:
  // Returns the list of arrays used to select the variables to be used
  // for the x,y and z axis.
  vtkGetObjectMacro(ArrayList,vtkStringArray)

  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(PointDataArraySelection,vtkDataArraySelection)

  // Description:
  // Set/Get a multiprocess-controller for reading in parallel.
  // By default this parameter is set to NULL by the constructor.
  vtkGetMacro(Controller,vtkMultiProcessController*)
  vtkSetMacro(Controller,vtkMultiProcessController*)

  // Description:
  // Returns the number of arrays in the file, i.e., the number of columns.
  int GetNumberOfPointArrays();

  // Description:
  // Returns the name of the ith array.
  const char* GetPointArrayName(int i);

  // Description:
  // Returns the status of the array corresponding to the given name.
  int GetPointArrayStatus(const char* name);

  // Description:
  // Sets the status of the array corresponding to the given name.
  void SetPointArrayStatus(const char* name, int status);
protected:
  vtkPGenericIOMultiBlockReader();
  ~vtkPGenericIOMultiBlockReader();

  char* XAxisVariableName;
  char* YAxisVariableName;
  char* ZAxisVariableName;

  char* FileName;
  int GenericIOType;
  int BlockAssignment;

  bool BuildMetaData;

  vtkMultiProcessController* Controller;

  vtkStringArray* ArrayList;
  vtkDataArraySelection* PointDataArraySelection;
  vtkCallbackCommand* SelectionObserver;

  gio::GenericIOReader* Reader;

  gio::GenericIOReader* GetInternalReader();

  bool ReaderParametersChanged();

  void LoadMetaData();

  void LoadRawVariableDataForBlock(const std::string& varName, int blockId);

  void LoadRawDataForBlock(int blockId);

  void GetPointFromRawData(
      int xType, void* xBuffer, int yType, void* yBuffer, int zType,
      void* zBuffer, vtkIdType id, double point[3]);

  void LoadCoordinatesForBlock(vtkUnstructuredGrid* grid, int blockId);

  void LoadDataArraysForBlock(vtkUnstructuredGrid* grid, int blockId);

  vtkUnstructuredGrid* LoadBlock(int blockId);

  // Descriptions
  // Call-back registered with the SelectionObserver.
  static void SelectionModifiedCallback(
    vtkObject *caller,unsigned long eid,
    void *clientdata,void *calldata );

  // Pipeline methods
  virtual int RequestInformation(
      vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  virtual int RequestData(
      vtkInformation*,vtkInformationVector**,vtkInformationVector*);


private:
  // not implemented
  vtkPGenericIOMultiBlockReader(const vtkPGenericIOMultiBlockReader& other);
  vtkPGenericIOMultiBlockReader& operator=(
      const vtkPGenericIOMultiBlockReader& other);
  // Internal struct -- must be here to avoid conflict with vtkACosmoReader's block_t
  struct block_t;
  // Internal helper class
  class vtkGenericIOMultiBlockMetaData;
  vtkGenericIOMultiBlockMetaData* MetaData;
};
