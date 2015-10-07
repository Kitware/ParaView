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
#ifndef UnstructuredGridCellCopier_h
#define UnstructuredGridCellCopier_h

#include "CellCopier.h" // for CellCopier
#include "IdBlock.h" // for IdBlock

class vtkCellArray;
class vtkFloatArray;
class vtkIdTypeArray;
class vtkUnsignedCharArray;

/// Copy geometry and data between unstructured datasets.
/**
Copies specific cells and associated geometry from/to a
pair of unstructured grid datasets.
*/
class UnstructuredGridCellCopier : public CellCopier
{
public:
  UnstructuredGridCellCopier()
        :
    SourcePts(0),
    SourceCells(0),
    SourceTypes(0),
    OutPts(0),
    OutCells(0),
    OutTypes(0),
    OutLocs(0)
      {  }

  virtual ~UnstructuredGridCellCopier();

  /**
  Initialize the object with an input and an output. This
  will set up array copiers. Derived classes must make sure
  this method gets called if they override it.
  */
  virtual void Initialize(vtkDataSet *in, vtkDataSet *out);

  /**
  Free resources used by the object.
  */
  virtual void Clear();

  /**
  Copy a contiguous block of cells and their associated point
  and cell data from iniput to output.
  */
  virtual vtkIdType Copy(IdBlock &block);
  using CellCopier::Copy;


private:
  void ClearSource();
  void ClearOutput();

private:
  vtkFloatArray *SourcePts;
  vtkCellArray *SourceCells;
  vtkUnsignedCharArray *SourceTypes;

  vtkFloatArray *OutPts;
  vtkCellArray *OutCells;
  vtkUnsignedCharArray *OutTypes;
  vtkIdTypeArray *OutLocs;
};

#endif

// VTK-HeaderTest-Exclude: UnstructuredGridCellCopier.h
