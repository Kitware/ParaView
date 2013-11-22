/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef UnstructuredFieldTopologyMap_h
#define UnstructuredFieldTopologyMap_h

#include "FieldTopologyMapData.h"

#include <map> // for map

class IdBlock;
class FieldLine;
class vtkDataSet;
class vtkFloatArray;
class vtkCellArray;
class vtkUnsignedCharArray;
class vtkIdTypeArray;
class vtkSQCellGenerator;

/// Interface to the topology map.
/**
Abstract collection of datastructures needed to build the topology map.
The details of building the map change drastically depending on the input
data type. Concrete classes deal with these specifics.
*/
class UnstructuredFieldTopologyMap : public FieldTopologyMapData
{
public:
  UnstructuredFieldTopologyMap()
        :
    SourceGen(0),
    SourcePts(0),
    SourceCells(0),
    SourceTypes(0),
    OutPts(0),
    OutCells(0),
    OutTypes(0),
    OutLocs(0)
      {  }

  virtual ~UnstructuredFieldTopologyMap();

  // Description:
  // Set the dataset to be used as the seed source. Use either
  // a dataset or a cell generator. The dataset explicitly contains
  // all geometry that will be accessed.
  virtual void SetSource(vtkDataSet *s);

  // Description:
  // Set the cell generator to be used as the seed source. Use either
  // a dataset or a cell generator. The cell generator implicitly contains
  // all geometry that will be accessed, generating only what is needed
  // on demand.
  virtual void SetSource(vtkSQCellGenerator *s);

  // Description:
  // Copy the IntersectColor and SourceId array into the output.
  virtual void SetOutput(vtkDataSet *o);

  // Description:
  // Convert a list of seed cells (sourceIds) to FieldLine
  // structures and build the output (if any).
  virtual vtkIdType InsertCells(IdBlock *SourceIds);

private:
  void ClearSource();
  void ClearOut();
  vtkIdType InsertCellsFromGenerator(IdBlock *SourceIds);
  vtkIdType InsertCellsFromDataset(IdBlock *SourceIds);

private:
  typedef std::pair<std::map<vtkIdType,vtkIdType>::iterator,bool> MapInsert;
  typedef std::pair<vtkIdType,vtkIdType> MapElement;
  std::map<vtkIdType,vtkIdType> IdMap;

  vtkSQCellGenerator *SourceGen;

  vtkFloatArray *SourcePts;
  vtkCellArray *SourceCells;
  vtkUnsignedCharArray *SourceTypes;

  vtkFloatArray *OutPts;
  vtkCellArray *OutCells;
  vtkUnsignedCharArray *OutTypes;
  vtkIdTypeArray *OutLocs;
};

#endif

// VTK-HeaderTest-Exclude: UnstructuredFieldTopologyMap.h
