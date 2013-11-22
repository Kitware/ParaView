/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef FieldTracerData_h
#define FieldTracerData_h

#include "FieldLine.h"

#include "vtkIntArray.h" // for vtkIntArray
#include <vector> // for vector

class IdBlock;
class FieldLine;
class vtkDataSet;
class vtkIntArray;
class TerminationCondition;
class vtkSQCellGenerator;

/// Interface to the topology map.
/**
Abstract collection of datastructures needed to build the topology map.
The details of building the map change drastically depending on the input
data type. Concrete classes deal with these specifics.
*/
class FieldTraceData
{
public:
  FieldTraceData();
  virtual ~FieldTraceData();

  /**
  Set the dataset to be used as the seed source. Use either
  a dataset or a cell generator. The dataset explicitly contains
  all geometry that will be accessed.
  */
  virtual void SetSource(vtkDataSet *s)=0;

  /**
  Set the cell generator to be used as the seed source. Use either
  a dataset or a cell generator. The cell generator implicitly contains
  all geometry that will be accessed, generating only what is needed
  on demand.
  */
  virtual void SetSource(vtkSQCellGenerator *s)=0;

  /**
  Copy the IntersectColor and SourceId array into the output.
  */
  virtual void SetOutput(vtkDataSet *o)=0;

  /**
  compute seed points (centred on cells of input). Copy the cells
  on which we operate into the output.
  */
  virtual vtkIdType InsertCells(IdBlock *SourceIds)=0;

  /**
  Get a specific field line.
  */
  FieldLine *GetFieldLine(unsigned long long i){ return this->Lines[i]; }

  /**
  Free resources holding the trace geometry. This can be quite large.
  Other data is retained.
  */
  void ClearTrace(unsigned long long i)
    {
    this->Lines[i]->DeleteTrace();
    }

  /**
  Free internal resources.
  */
  void ClearFieldLines();

  /**
  Move scalar data (IntersectColor, SourceId) from the internal
  structure into the vtk output data.
  */
  virtual int SyncScalars()=0;

  /**
  Move field line geometry (trace) from the internal structure
  into the vtk output data.
  */
  virtual int SyncGeometry()=0;

  /**
  Access to the termination object.
  */
  TerminationCondition *GetTerminationCondition(){ return this->Tcon; }

  /**
  Print a legend, can be reduced to the minimal number of colors needed
  or all posibilities may be included. The latter is better for temporal
  animations.
  */
  virtual void PrintLegend(int){}

protected:
  std::vector<FieldLine *> Lines;
  TerminationCondition *Tcon;
};

#endif

// VTK-HeaderTest-Exclude: FieldTraceData.h
