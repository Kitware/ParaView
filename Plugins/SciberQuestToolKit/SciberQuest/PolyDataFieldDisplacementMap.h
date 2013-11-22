/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef PolyDataFieldDisplacementMap_h
#define PolyDataFieldDisplacementMap_h

#include "FieldDisplacementMapData.h"

#include <map> // for map

class IdBlock;
class FieldLine;
class vtkDataSet;
class vtkFloatArray;
class vtkCellArray;
class vtkUnsignedCharArray;
class vtkIdTypeArray;
class vtkSQCellGenerator;

/// Records the end point(displacement) of a stream line.
/**
The displacement map, maps the end point(s) of a streamline onto
the seed geometry. This map is used in FTLE computations.

PolyData implementation.
*/
class PolyDataFieldDisplacementMap : public FieldDisplacementMapData
{
public:
  PolyDataFieldDisplacementMap()
        :
    SourceGen(0),
    SourcePts(0),
    SourceCells(0),
    OutPts(0),
    OutCells(0),
    CellType(0)
  {}

  virtual ~PolyDataFieldDisplacementMap();

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

// VTK-HeaderTest-Exclude: PolyDataFieldDisplacementMap.h
