/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef PolyDataFieldTopologyMap_h
#define PolyDataFieldTopologyMap_h

#include "FieldTraceData.h"

#include<vector>
using std::vector;

#include<map>
using std::map;
using std::pair;

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
class PolyDataFieldTopologyMap : public FieldTraceData
{
public:
  PolyDataFieldTopologyMap()
        :
    SourceGen(0),
    SourcePts(0),
    SourceCells(0),
    OutPts(0),
    OutCells(0),
    CellType(0)
      {  }

  virtual ~PolyDataFieldTopologyMap();

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
  virtual int InsertCells(IdBlock *SourceIds);


private:
  void ClearSource();
  void ClearOut();
  int InsertCellsFromGenerator(IdBlock *SourceIds);
  int InsertCellsFromDataset(IdBlock *SourceIds);

private:
  typedef pair<map<vtkIdType,vtkIdType>::iterator,bool> MapInsert;
  typedef pair<vtkIdType,vtkIdType> MapElement;
  map<vtkIdType,vtkIdType> IdMap;

  vtkSQCellGenerator *SourceGen;

  vtkFloatArray *SourcePts;
  vtkCellArray *SourceCells;

  vtkFloatArray *OutPts;
  vtkCellArray *OutCells;

  // enum {
  //   NONE=0,
  //   POLY=1,
  //   VERT=2,
  //   STRIP=3,
  //   LINE=4
  //   };
  int CellType;
};

#endif
