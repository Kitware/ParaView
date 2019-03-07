// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGMVReader.h

  Copyright (c) 2009-2012 Sven Buijssen, Jens Acker, TU Dortmund
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// .NAME vtkGMVReader - reads a dataset in GeneralMeshViewer "GMV" format
// .SECTION Description
// vtkGMVReader creates a multi-block dataset and reads unstructured grids,
// structured regular brick meshes, logically rectangular brick meshes, tracer
// and polygonal data from ASCII and binary files stored in GMV file format,
// with optional data stored at the nodes or at the cells of the model.
//
// vtkGMVReader was originally created by Jens F. Acker (TU Dortmund) using
// vtkAVSucdReader as a development template to read unstructured grids with
// point and cell data. It has since been extended by Sven H.M. Buijssen (TU
// Dortmund) to support a larger subset of the GMV file format and maintained
// to keep up with VTK API changes.
//
// Frank Ortega (Applied Physics Division of Los Alamos National Laboratory)
// used to provide an I/O library called GMVREAD to read files in the GMV
// format. It is used by vtkGMVReader to avoid replicating his work and to
// insure against possible future format changes (assuming that GMVREAD would
// get updated accordingly then).
//
// .SECTION Caveats
// Face based data and less commonly used keywords are yet unsupported.
//
// .SECTION Thanks
// Thanks to Jean Favre for developing vtkAVSucdReader.
// Thanks to Yvan Fournier for providing the code to support nfaced elements
// for class vtkEnSightGoldReader which helped to add support for GMV cell
// type generic.

#ifndef vtkGMVReader_h
#define vtkGMVReader_h

#include "vtkGMVReaderModule.h" // for export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

#include <map>    // for TimeStepValuesMap & NumberOfPolygonsMap
#include <string> // for TimeStepValuesMap & NumberOfPolygonsMap

class vtkCallbackCommand;
class vtkDataArraySelection;
class vtkDataSet;
class vtkFieldData;
class vtkIntArray;
class vtkMultiProcessController;
class vtkPolyData;
class vtkStringArray;

class VTKGMVREADER_EXPORT vtkGMVReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkGMVReader* New();
  vtkTypeMacro(vtkGMVReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Description:
  // Specify file name of GMV datafile to read
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Is the given file name a GMV file?
  int CanReadFile(const char* filename);

  // Description:
  // Return the file at the given index, the indexing is 0 based.
  // const char* GetNthFileName(int index);

  // Description:
  // Is the file to be read written in binary format (as opposed to ascii).
  vtkSetMacro(BinaryFile, int);
  vtkGetMacro(BinaryFile, int);
  vtkBooleanMacro(BinaryFile, int);

  // Description:
  // Get the total number of nodes.
  vtkGetMacro(NumberOfNodes, unsigned long);

  // Description:
  // Get the total number of cells.
  vtkGetMacro(NumberOfCells, unsigned long);

  // Description:
  // Get the total number of tracers.
  vtkSetMacro(NumberOfTracers, unsigned long);
  vtkGetMacro(NumberOfTracers, unsigned long);

  // Description:
  // Get the total number of polygons.
  vtkSetMacro(NumberOfPolygons, unsigned long);
  vtkGetMacro(NumberOfPolygons, unsigned long);

  // Description:
  // Get the number of data fields at the nodes.
  vtkGetMacro(NumberOfNodeFields, int);

  // Description:
  // Get the number of data fields at the cell centers.
  vtkGetMacro(NumberOfCellFields, int);

  // Description:
  // Get the number of data fields for the model. Unused because VTK
  // has no methods for it.
  vtkGetMacro(NumberOfFields, int);

  // Description:
  // Get the number of data components at the nodes and cells.
  vtkGetMacro(NumberOfNodeComponents, int);
  vtkGetMacro(NumberOfCellComponents, int);

  // Description:
  // If true, then import tracers. True by default.
  vtkGetMacro(ImportTracers, int);
  vtkSetMacro(ImportTracers, int);
  vtkBooleanMacro(ImportTracers, int);

  // Description:
  // If true, then import polygons. False by default.
  vtkGetMacro(ImportPolygons, int);
  vtkSetMacro(ImportPolygons, int);
  vtkBooleanMacro(ImportPolygons, int);

  // Description:
  // Get an array that contains all the file names.
  vtkGetObjectMacro(FileNames, vtkStringArray);

  // Description:
  // Set/Get the endian-ness of the binary file.
  void SetByteOrderToBigEndian();
  void SetByteOrderToLittleEndian();
  const char* GetByteOrderAsString();

  vtkSetMacro(ByteOrder, int);
  vtkGetMacro(ByteOrder, int);

  // Description:
  // The following methods allow selective reading of solutions fields. By
  // default, ALL data fields (point data, cell data, field data) are read,
  // but this can be modified (e.g. from the ParaView GUI).
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  int GetNumberOfFieldArrays();
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);
  const char* GetFieldArrayName(int index);
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  int GetFieldArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void SetCellArrayStatus(const char* name, int status);
  void SetFieldArrayStatus(const char* name, int status);

  void DisableAllPointArrays();
  void EnableAllPointArrays();
  void DisableAllCellArrays();
  void EnableAllCellArrays();
  void DisableAllFieldArrays();
  void EnableAllFieldArrays();

  int GetHasTracers();
  int GetHasPolygons();
  int GetHasProbtimeKeyword();

  // Description:
  // Set the controller use in compositing (set to
  // the global controller by default)
  // If not using the default, this must be called before any
  // other methods.
  virtual void SetController(vtkMultiProcessController* controller);

protected:
  vtkGMVReader();
  ~vtkGMVReader() override;
  // int ProcessRequest( vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  // Setup the output with no data available.  Used in error cases.
  void SetupEmptyOutput();

  char* FileName;
  int BinaryFile;

  unsigned long NumberOfNodes;
  unsigned long NumberOfCells;

  unsigned long NumberOfTracers;
  int ImportTracers;

  unsigned long NumberOfPolygons;
  int ImportPolygons;

  unsigned int NumberOfNodeFields;
  unsigned int NumberOfNodeComponents;
  unsigned int NumberOfCellFields;
  unsigned int NumberOfCellComponents;
  unsigned int NumberOfFields;
  unsigned int NumberOfFieldComponents;

  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;
  vtkDataArraySelection* FieldDataArraySelection;

  // The observer to modify this object when the array selections are
  // modified.
  vtkCallbackCommand* SelectionObserver;

  // Callback registered with the SelectionObserver.
  static void SelectionModifiedCallback(
    vtkObject* caller, unsigned long eid, void* clientdata, void* calldata);

  vtkMultiProcessController* Controller;

private:
  vtkGMVReader(const vtkGMVReader&) = delete;
  void operator=(const vtkGMVReader&) = delete;

  vtkStringArray* FileNames; // VTK array of files
  bool ContainsProbtimeKeyword;

  vtkDataSet* Mesh;
  vtkFieldData* FieldDataTmp;
  vtkPolyData* Tracers;
  vtkPolyData* Polygons;

  // filename -> #polygons and filename -> #tracers mappings
  typedef std::map<std::string, unsigned long> stringToULongMap;
  stringToULongMap NumberOfPolygonsMap;
  stringToULongMap NumberOfTracersMap;

  // filename -> time step mapping
  std::map<std::string, double> TimeStepValuesMap;

  enum
  {
    FILE_BIG_ENDIAN = 0,
    FILE_LITTLE_ENDIAN = 1
  };
  enum GMVCell_type
  {
    LINE = 0,
    TRI = 1,
    QUAD = 2,
    TET = 3,
    HEX = 4,
    PHEX8 = 5,
    PHEX20 = 6,
    PRISM = 7,
    PYRAMID = 8,
    VFACE2D = 9,
    VFACE3D = 10
  };

  template <class C>
  struct DataInfo
  {
    int veclen; // number of components in the node or cell variable
    C min[3];   // pre-calculated data minima (max size 3 for vectors)
    C max[3];   // pre-calculated data maxima (max size 3 for vectors)
  };

  DataInfo<float>* NodeDataInfo; // type float because Get{Node,Cell}DataRange
  DataInfo<float>* CellDataInfo; // takes floats for min/max

  // Toggle whether GMV node numbers needs to decremented by 1 for VTK
  bool DecrementNodeIds;
  int ByteOrder;
  int GetLabel(char* string, int number, char* label);
};

#endif // vtkGMVReader_h
