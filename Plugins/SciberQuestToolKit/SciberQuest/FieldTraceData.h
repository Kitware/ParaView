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
