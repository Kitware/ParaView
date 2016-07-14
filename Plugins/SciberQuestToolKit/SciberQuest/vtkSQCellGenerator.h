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
#ifndef vtkSQCellGenerator_h
#define vtkSQCellGenerator_h

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkObject.h"

class vtkInformationObjectBaseKey;

/// Interface to sources that provide data on demand
/**

*/
class VTKSCIBERQUEST_EXPORT vtkSQCellGenerator : public vtkObject
{
public:
  /**
  Key that provides access to the source object.
  */
  static vtkInformationObjectBaseKey *CELL_GENERATOR();

  /**
  Return the total number of cells available.
  */
  virtual vtkIdType GetNumberOfCells()=0;

  /**
  Return the cell type of the cell at id.
  */
  virtual int GetCellType(vtkIdType id)=0;

  /**
  Return the number of points required for the named
  cell. For homogeneous datasets its always the same.
  */
  virtual int GetNumberOfCellPoints(vtkIdType id)=0;

  /**
  Copy the points from a cell into the provided buffer,
  buffer is expected to be large enought to hold all of the
  points.
  */
  virtual int GetCellPoints(vtkIdType id, float *pts)=0;

  /**
  Copy the point's indexes into the provided bufffer,
  buffer is expected to be large enough. Return the
  number of points coppied. The index is unique across
  all processes but is not the same as the point id
  in a VTK dataset.
  */
  virtual int GetCellPointIndexes(vtkIdType cid, vtkIdType *idx)=0;

public:
  vtkTypeMacro(vtkSQCellGenerator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkSQCellGenerator(const vtkSQCellGenerator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSQCellGenerator&) VTK_DELETE_FUNCTION;


protected:
  vtkSQCellGenerator(){}
  virtual ~vtkSQCellGenerator(){}

};

#endif
