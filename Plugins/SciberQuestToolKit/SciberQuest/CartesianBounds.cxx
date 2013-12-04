/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
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
