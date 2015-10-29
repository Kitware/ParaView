/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef PoincareMapData_h
#define PoincareMapData_h

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
