/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __PoincareMapData_h
#define __PoincareMapData_h

#include "FieldTraceData.h" // for FieldTraceData

class IdBlock;
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
class PoincareMapData : public FieldTraceData
{
public:
  PoincareMapData()
        :
    SourceGen(0),
    SourcePts(0),
    SourceCells(0),
    OutPts(0),
    OutCells(0),
    SourceId(0),
    SourceCellGid(0)
       {}

  virtual ~PoincareMapData();

  /**
  Set the dataset to be used as the seed source. Use either
  a dataset or a cell generator. The dataset explicitly contains
  all geometry that will be accessed.
  */
  virtual void SetSource(vtkDataSet *s);

  /**
  Set the cell generator to be used as the seed source. Use either
  a dataset or a cell generator. The cell generator implicitly contains
  all geometry that will be accessed, generating only what is needed
  on demand.
  */
  virtual void SetSource(vtkSQCellGenerator *s);

  /**
  Copy the IntersectColor and SourceId array into the output.
  */
  virtual void SetOutput(vtkDataSet *o);

  /**
  Convert a list of seed cells (sourceIds) to FieldLine
  structures and build the output (if any).
  */
  virtual vtkIdType InsertCells(IdBlock *SourceIds);

  /**
  Set the global id of cell 0 in this processes source cells.
  */
  void SetSourceCellGid(unsigned long gid){ this->SourceCellGid=gid; }
  unsigned long GetSourceCellGid(){ return this->SourceCellGid; }

  /**
  Scalars are updated during sync geometry.
  */
  virtual int SyncScalars(){ return 1; }

  /**
  Move poincare map geometry from the internal structure
  into the vtk output data.
  */
  virtual int SyncGeometry();

  /**
  No legend is used for poincare map.
  */
  virtual void PrintLegend(int){}


private:
  vtkIdType InsertCellsFromGenerator(IdBlock *SourceIds);
  vtkIdType InsertCellsFromDataset(IdBlock *SourceIds);

  void ClearSource();
  void ClearOut();

private:
  vtkSQCellGenerator *SourceGen;

  vtkFloatArray *SourcePts;
  vtkCellArray *SourceCells;

  vtkFloatArray *OutPts;
  vtkCellArray *OutCells;

  vtkIntArray *SourceId;
  unsigned long SourceCellGid;
};

#endif

// VTK-HeaderTest-Exclude: PoincareMapData.h
