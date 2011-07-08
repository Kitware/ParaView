/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include "UnstructuredGridCellCopier.h"

#include "vtkUnstructuredGrid.h"

#include "IdBlock.h"
#include "FieldLine.h"
#include "TerminationCondition.h"
#include "vtkDataSet.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"

//-----------------------------------------------------------------------------
UnstructuredGridCellCopier::~UnstructuredGridCellCopier()
{
  this->ClearSource();
  this->ClearOutput();
}

//-----------------------------------------------------------------------------
void UnstructuredGridCellCopier::Clear()
{
  this->CellCopier::Clear();
  this->ClearSource();
  this->ClearOutput();
}

//-----------------------------------------------------------------------------
void UnstructuredGridCellCopier::ClearSource()
{
  if (this->SourcePts){ this->SourcePts->Delete(); }
  if (this->SourceCells){ this->SourceCells->Delete(); }
  if (this->SourceTypes){ this->SourceTypes->Delete(); }
  this->SourcePts=0;
  this->SourceCells=0;
  this->SourceTypes=0;
}

//-----------------------------------------------------------------------------
void UnstructuredGridCellCopier::ClearOutput()
{
  if (this->OutPts){ this->OutPts->Delete(); }
  if (this->OutCells){ this->OutCells->Delete(); }
  if (this->OutTypes){ this->OutTypes->Delete(); }
  if (this->OutLocs){ this->OutLocs->Delete(); }
  this->OutPts=0;
  this->OutCells=0;
  this->OutTypes=0;
  this->OutLocs=0;
}

//-----------------------------------------------------------------------------
void UnstructuredGridCellCopier::Initialize(vtkDataSet *s, vtkDataSet *o)
{
  this->CellCopier::Initialize(s,o);

  this->ClearSource();
  this->ClearOutput();

  // source
  vtkUnstructuredGrid *source=dynamic_cast<vtkUnstructuredGrid*>(s);
  if (source==0)
    {
    sqErrorMacro(cerr,
       "Error: Source must be unstructured. " << s->GetClassName());
    return;
    }

  if (source->GetNumberOfCells()==0)
    {
    // nothing to do if there are no cells.
    return;
    }

  this->SourcePts=dynamic_cast<vtkFloatArray*>(source->GetPoints()->GetData());
  if (this->SourcePts==0)
    {
    sqErrorMacro(cerr,"Error: Points are not float precision.");
    return;
    }
  this->SourcePts->Register(0);

  this->SourceCells=source->GetCells();
  this->SourceCells->Register(0);

  this->SourceTypes=source->GetCellTypesArray();
  this->SourceTypes->Register(0);

  // output
  vtkUnstructuredGrid *out=dynamic_cast<vtkUnstructuredGrid*>(o);
  if (out==0)
    {
    sqErrorMacro(cerr,
      "Error: Out must be unstructured grid. " << o->GetClassName());
    return;
    }

  vtkPoints *opts=vtkPoints::New();
  out->SetPoints(opts);
  opts->Delete();

  this->OutPts=dynamic_cast<vtkFloatArray*>(opts->GetData());
  this->OutPts->Register(0);

  this->OutCells=vtkCellArray::New();
  this->OutTypes=vtkUnsignedCharArray::New();
  this->OutLocs=vtkIdTypeArray::New();

  out->SetCells(this->OutTypes,this->OutLocs,this->OutCells);
}

//-----------------------------------------------------------------------------
int UnstructuredGridCellCopier::Copy(IdBlock &SourceIds)
{
  // copy cell data
  this->CopyCellData(SourceIds);

  // copy cell geometry and point data
  vtkIdType startCellId=SourceIds.first();
  vtkIdType nCellsLocal=SourceIds.size();

  // Cells are sequentially acccessed so explicitly skip all cells
  // we aren't interested in.
  this->SourceCells->InitTraversal();
  for (vtkIdType i=0; i<startCellId; ++i)
    {
    vtkIdType n;
    vtkIdType *ptIds;
    this->SourceCells->GetNextCell(n,ptIds);
    }

  // input points
  float *pSourcePts=this->SourcePts->GetPointer(0);

  // input types
  unsigned char *pSourceTypes
    = this->SourceTypes->GetPointer(startCellId);

  // output points
  vtkIdType nOutPts=this->OutPts->GetNumberOfTuples();

  // output cells
  vtkIdTypeArray *outCells=this->OutCells->GetData();
  vtkIdType nCellIds=outCells->GetNumberOfTuples();
  vtkIdType nOutCells=this->OutCells->GetNumberOfCells();
  this->OutCells->SetNumberOfCells(nOutCells+nCellsLocal);

  // output cell types
  vtkIdType endOfTypes=this->OutTypes->GetNumberOfTuples();
  unsigned char *pOutTypes=this->OutTypes->WritePointer(endOfTypes,nCellsLocal);

  // output cell locations
  vtkIdType endOfLocs=this->OutLocs->GetNumberOfTuples();
  vtkIdType *pOutLocs=this->OutLocs->WritePointer(endOfLocs,nCellsLocal);

  // For each cell asigned
  for (vtkIdType i=0; i<nCellsLocal; ++i)
    {
    // get the cell that belong to us.
    vtkIdType nPtIds=0;
    vtkIdType *ptIds=0;
    this->SourceCells->GetNextCell(nPtIds,ptIds);

    // set the new cell's location
    *pOutLocs=nCellIds;
    ++pOutLocs;

    // copy its type.
    *pOutTypes=*pSourceTypes;
    ++pOutTypes;
    ++pSourceTypes;

    // Get location to write new cell.
    vtkIdType *pOutCells=outCells->WritePointer(nCellIds,nPtIds+1);

    // update next cell write location.
    nCellIds+=nPtIds+1;

    // number of points in this cell
    *pOutCells=nPtIds;
    ++pOutCells;

    // Get location to write new point. assumes we need to copy all
    // but this is wrong as there will be many duplicates. ignored.
    float *pOutPts=this->OutPts->WritePointer(3*nOutPts,3*nPtIds);

    // transfer from input to output (only what we own)
    for (vtkIdType j=0; j<nPtIds; ++j)
      {
      vtkIdType idx=3*ptIds[j];
      vtkIdType outId=nOutPts;
      // do we already have this point?
      if (this->GetUniquePointId(ptIds[j],outId))
        {
        // this point hasn't previsouly been coppied
        // copy the point.
        pOutPts[0]=pSourcePts[idx  ];
        pOutPts[1]=pSourcePts[idx+1];
        pOutPts[2]=pSourcePts[idx+2];
        pOutPts+=3;
        ++nOutPts;
        // copy point data
        this->CopyPointData(ptIds[j]);
        }
      // insert the point id into the new cell.
      *pOutCells=outId;
      ++pOutCells;
      }
    }

  // correct the length of the point array, above we assumed 
  // that all points from each cell needed to be inserted
  // and allocated that much space.
  this->OutPts->Resize(nOutPts);

  return nCellsLocal;
}
