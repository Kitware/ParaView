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
#ifndef CellCopier_h
#define CellCopier_h

#include "IdBlock.h" // for IdBlock

#include "vtkType.h"

#include <map> // for map
#include <vector> // for vector

class DataArrayCopier;
class vtkDataSet;

/// Abstract interface for copying geometry and data between datasets.
/**
Copies specific cells and their associated data from one dataset
to another.
*/
class CellCopier
{
public:
  virtual ~CellCopier();

  /**
  Initialize the object with an input and an output. This
  will set up array copiers. Derived classes must make sure
  this method gets called if they override it.
  */
  virtual void Initialize(vtkDataSet *in, vtkDataSet *out);

  /**
  Copy a single cell and its associated point and cell data
  from iniput to output.
  */
  virtual vtkIdType Copy(vtkIdType cellId)
    {
    IdBlock id(cellId);
    return this->Copy(id);
    }

  /**
  Copy a contiguous block of cells and their associated point
  and cell data from iniput to output. Derived classes must make
  sure this method gets called if they override it.
  */
  virtual vtkIdType Copy(IdBlock &block)=0;

  /**
  Free resources used by the object. Dervied classes must makes
  sure this method gets called if they override it.
  */
  virtual void Clear()
    {
    this->ClearDataCopier();
    this->ClearPointIdMap();
    }

  /**
  Insure that point to insert is unique based on its inputId.
  The outputId must contain the id that will be used if the
  id is unique. If the id is unque this value is cached. If
  the id is not unique then outputId is set to the cached
  value and false is returned.
  */
  bool GetUniquePointId(vtkIdType inputId, vtkIdType &outputId);

protected:
  int CopyPointData(IdBlock &block);
  int CopyPointData(vtkIdType id);
  int CopyCellData(IdBlock &block);
  int CopyCellData(vtkIdType id);
  void ClearDataCopier();
  void ClearPointIdMap(){ this->PointIdMap.clear(); }

protected:
  std::map<vtkIdType,vtkIdType> PointIdMap;

  std::vector<DataArrayCopier *> PointDataCopier;
  std::vector<DataArrayCopier *> CellDataCopier;
};

#endif

// VTK-HeaderTest-Exclude: CellCopier.h
