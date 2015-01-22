/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPGenericIOReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkPGenericIOReader.h -- Read GenericIO formatted data
//
// .SECTION Description
//  Creates a vtkUnstructuredGrid instance from a GenericIO file.

#ifndef VTKPGENERICIOREADER_H_
#define VTKPGENERICIOREADER_H_

// VTK includes
#include "vtkUnstructuredGridAlgorithm.h" // Base class
#include "vtkPVVTKExtensionsCosmoToolsModule.h" // For export macro

// Forward Declarations
class vtkCallbackCommand;
class vtkDataArray;
class vtkDataArraySelection;
class vtkGenericIOMetaData;
class vtkIdList;
class vtkInformation;
class vtkInformationVector;
class vtkMultiProcessController;
class vtkStdString;
class vtkStringArray;
class vtkUnstructuredGrid;

// GenericIO Forward Declarations
namespace gio {
  class GenericIOReader;
}

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPGenericIOReader :
  public vtkUnstructuredGridAlgorithm
{
public:

enum IOType {
  IOTYPEMPI,
  IOTYPEPOSIX
};

enum BlockAssignment {
  ROUND_ROBIN,
  RCB
};


  static vtkPGenericIOReader *New();
  vtkTypeMacro(vtkPGenericIOReader,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the name of the cosmology particle binary file to read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Set/Get the variable name to be used as the x-axis for plotting particles.
  vtkSetStringMacro(XAxisVariableName);
  vtkGetStringMacro(XAxisVariableName);

  // Description:
  // Set/Get the variable name to be used as the x-axis for plotting particles.
  vtkSetStringMacro(YAxisVariableName);
  vtkGetStringMacro(YAxisVariableName);

  // Description:
  // Set/Get the variable name to be used as the x-axis for plotting particles.
  vtkSetStringMacro(ZAxisVariableName);
  vtkGetStringMacro(ZAxisVariableName);

  // Description:
  // Set/Get the underlying IO method the reader will employ, i.e., MPI or POSIX.
  vtkSetMacro(GenericIOType,int);
  vtkGetMacro(GenericIOType,int);

  // Description:
  // Set/Get the underlying block-assignment strategy to use, i.e., ROUND_ROBIN,
  // or RCB.
  vtkSetMacro(BlockAssignment,int);
  vtkGetMacro(BlockAssignment,int);

  // Description:
  // Set/Get the RankInQuery. Used in combination with SetQueryRankNeighbors(1)
  // tells the reader to render only the data of the RankInQuery and its
  // neighbors.
  vtkSetMacro(RankInQuery,int);
  vtkGetMacro(RankInQuery,int);

  // Description:
  // Set/Get whether the reader should read/render only the data of the
  // user-supplied rank, via SetRankInQuery(),
  vtkSetMacro(QueryRankNeighbors,int);
  vtkGetMacro(QueryRankNeighbors,int);

  // Description:
  // Set/Get whether the reader should append the coordinates of the block each
  // point was read from as a point data array.  Defaults to false (Off).
  vtkSetMacro(AppendBlockCoordinates,bool);
  vtkBooleanMacro(AppendBlockCoordinates,bool);
  vtkGetMacro(AppendBlockCoordinates,bool);

  // Description:
  // Returns the list of arrays used to select the variables to be used
  // for the x,y and z axis.
  vtkGetObjectMacro(ArrayList,vtkStringArray);

  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(PointDataArraySelection,vtkDataArraySelection);

  // Description:
  // Set/Get a multiprocess-controller for reading in parallel.
  // By default this parameter is set to NULL by the constructor.
  vtkSetMacro(Controller,vtkMultiProcessController*);
  vtkGetMacro(Controller,vtkMultiProcessController*);

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
  // Sets the status of the array named.  If the status is 1, the array
  // will be read in on the resulting dataset.
  void SetPointArrayStatus(const char* name, int status);

  // Description:
  // Gets/Sets the variable name for the halo id of the particle.
  // This is used by the requested halo selector to select only the
  // points in the desired halos.
  vtkSetStringMacro(HaloIdVariableName);
  vtkGetStringMacro(HaloIdVariableName);

  // Description:
  // Gets the ith requested halo id.
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  vtkIdType GetRequestedHaloId(vtkIdType i);

  // Description:
  // Gets the number of requested halo ids.
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  vtkIdType GetNumberOfRequestedHaloIds();

  // Description:
  // Sets the number of requested halo ids.
  // Use SetRequestedHaloId() to se the ids after this is called
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  void SetNumberOfRequestedHaloIds(vtkIdType numIds);

  // Description:
  // Adds the given halo id to the list of halo ids to request.
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  void AddRequestedHaloId(vtkIdType haloId);

  // Description:
  // Clears the list of requested halo ids.
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  void ClearRequestedHaloIds();

  // Description:
  // Sets the ith requested halo id to the given haloId.
  // If the number of requested halo ids is
  // greater than 0, only points with those halo ids will be read in.
  // Otherwise all points will be read in.
  void SetRequestedHaloId(vtkIdType i, vtkIdType haloId);

protected:
  vtkPGenericIOReader();
  virtual ~vtkPGenericIOReader();

  // Pipeline methods
  virtual int RequestInformation(
      vtkInformation*,vtkInformationVector**,vtkInformationVector*);
  virtual int RequestData(
      vtkInformation*,vtkInformationVector**,vtkInformationVector*);

  // Description:
  // Loads the GenericIO metadata from the file.
  void LoadMetaData();

  // Description:
  // This method checks if the internal reader parameters have changed.
  // Namely, if the I/O method or filename have changed, the method returns
  // true.
  bool ReaderParametersChanged();

  // Description:
  // Returns the internal reader instance according to IOType.
  gio::GenericIOReader* GetInternalReader();


  // Description:
  // Return the point from the raw data.
  void GetPointFromRawData(
          int xType, void* xBuffer,
          int yType, void* yBuffer,
          int zType, void* zBuffer,
          vtkIdType idx,
          double pnt[3]);

  // Description:
  // Loads the variable with the given name
  void LoadRawVariableData(std::string varName);

  // Description:
  // Loads the Raw data
  void LoadRawData();

  // Description:
  // Loads the particle coordinates
  void LoadCoordinates(vtkUnstructuredGrid *grid);

  // Description:
  // Loads the particle data arrays
  void LoadData(vtkUnstructuredGrid *grid);

  // Description:
  // Finds the neighbors of the user-supplied rank
  void FindRankNeighbors();

  // Descriptions
  // Call-back registered with the SelectionObserver.
  static void SelectionModifiedCallback(
    vtkObject *caller,unsigned long eid,
    void *clientdata,void *calldata );

  char *XAxisVariableName;
  char *YAxisVariableName;
  char *ZAxisVariableName;
  char *HaloIdVariableName;

  char *FileName;
  int GenericIOType;
  int BlockAssignment;

  int QueryRankNeighbors;
  int RankInQuery;

  bool BuildMetaData;
  bool AppendBlockCoordinates;


  vtkMultiProcessController* Controller;

  vtkStringArray* ArrayList;
  vtkIdList* HaloList;
  vtkDataArraySelection* PointDataArraySelection;
  vtkCallbackCommand* SelectionObserver;

// BTX
  gio::GenericIOReader* Reader;
  vtkGenericIOMetaData* MetaData;
// ETX

  int RequestInfoCounter;
  int RequestDataCounter;
private:
  vtkPGenericIOReader(const vtkPGenericIOReader&); // Not implemented.
  void operator=(const vtkPGenericIOReader&); // Not implemented.
};

#endif /* VTKPGENERICIOREADER_H_ */
