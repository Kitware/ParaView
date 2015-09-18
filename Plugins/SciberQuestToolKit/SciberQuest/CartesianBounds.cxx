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
#include "CartesianBounds.h"

#include "Tuple.hxx"
#include "vtkUnstructuredGrid.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkFloatArray.h"
#include "vtkCellType.h"

//*****************************************************************************
std::ostream &operator<<(std::ostream &os,const CartesianBounds &bounds)
{
  os << Tuple<double>(bounds.GetData(),6);

  return os;
}

//*****************************************************************************
vtkUnstructuredGrid &operator<<(
        vtkUnstructuredGrid &data,
        const CartesianBounds &bounds)
{
  // initialize empty dataset
  if (data.GetNumberOfCells()<1)
    {
    vtkPoints *opts=vtkPoints::New();
    data.SetPoints(opts);
    opts->Delete();

    vtkCellArray *cells=vtkCellArray::New();
    vtkUnsignedCharArray *types=vtkUnsignedCharArray::New();
    vtkIdTypeArray *locs=vtkIdTypeArray::New();

    data.SetCells(types,locs,cells);

    cells->Delete();
    types->Delete();
    locs->Delete();
    }

  // build the cell
  vtkFloatArray *pts=dynamic_cast<vtkFloatArray*>(data.GetPoints()->GetData());
  vtkIdType ptId=pts->GetNumberOfTuples();
  float *ppts=pts->WritePointer(3*ptId,24);

  int id[24]={
        0,2,4,
        1,2,4,
        1,3,4,
        0,3,4,
        0,2,5,
        1,2,5,
        1,3,5,
        0,3,5};

  vtkIdType ptIds[8];

  for (int i=0,q=0; i<8; ++i)
    {
    for (int j=0; j<3; ++j,++q)
      {
      ppts[q]=bounds[id[q]];
      }
    ptIds[i]=ptId+i;
    }

  data.InsertNextCell(VTK_HEXAHEDRON,8,ptIds);

  return data;
}
