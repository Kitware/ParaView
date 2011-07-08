/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "StreamlineData.h"

#include "postream.h"
#include "WorkQueue.h"
#include "FieldLine.h"
#include "TerminationCondition.h"

#include "vtkDataSet.h"
#include "vtkCellData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkIdTypeArray.h"

//-----------------------------------------------------------------------------
StreamlineData::~StreamlineData()
{
  this->ClearSource();
  this->ClearOut();
}

//-----------------------------------------------------------------------------
void StreamlineData::ClearSource()
{
  if (this->SourcePts){ this->SourcePts->Delete(); }
  if (this->SourceCells){ this->SourceCells->Delete(); }
  this->SourcePts=0;
  this->SourceCells=0;
}

//-----------------------------------------------------------------------------
void StreamlineData::ClearOut()
{
  if (this->OutPts){ this->OutPts->Delete(); }
  if (this->OutCells){ this->OutCells->Delete(); }
  if (this->Length){ this->Length->Delete(); }
  this->OutPts=0;
  this->OutCells=0;
  this->Length=0;
}

//-----------------------------------------------------------------------------
void StreamlineData::SetSource(vtkSQCellGenerator *sourceGen)
{
  sqErrorMacro(pCerr(),"Cell generator source is not supported."); 
}

//-----------------------------------------------------------------------------
void StreamlineData::SetSource(vtkDataSet *s)
{
  this->ClearSource();

  // some process may not have any work, in that case we still
  // should initialize with empty objects.
  if (s->GetNumberOfCells()==0)
    {
    this->SourceCells=vtkCellArray::New();
    this->SourcePts=vtkFloatArray::New();
    return;
    }

  // unstructured (the easier)
  vtkUnstructuredGrid *sourceug=dynamic_cast<vtkUnstructuredGrid*>(s);
  if (sourceug)
    {
    this->SourcePts=dynamic_cast<vtkFloatArray*>(sourceug->GetPoints()->GetData());
    if (this->SourcePts==0)
      {
      cerr << "Error: Points are not float precision." << endl;
      return;
      }
    this->SourcePts->Register(0);

    this->SourceCells=sourceug->GetCells();
    this->SourceCells->Register(0);

    return;
    }

  // polydata
  vtkPolyData *sourcepd=dynamic_cast<vtkPolyData*>(s);
  if (sourcepd)
    {
    this->SourcePts=dynamic_cast<vtkFloatArray*>(sourcepd->GetPoints()->GetData());
    if (this->SourcePts==0)
      {
      cerr << "Error: Points are not float precision." << endl;
      return;
      }
    this->SourcePts->Register(0);

    if (sourcepd->GetNumberOfPolys())
      {
      this->SourceCells=sourcepd->GetPolys();
      }
    else
    if (sourcepd->GetNumberOfLines())
      {
      this->SourceCells=sourcepd->GetLines();
      }
    else
    if (sourcepd->GetNumberOfVerts())
      {
      this->SourceCells=sourcepd->GetVerts();
      }
    else
      {
      cerr << "Error: Polydata doesn't have any supported cells." << endl;
      return;
      }
    this->SourceCells->Register(0);
    return;
    }

  cerr << "Error: Unsupported input data type." << endl;
}

//-----------------------------------------------------------------------------
void StreamlineData::SetOutput(vtkDataSet *o)
{
  this->FieldTraceData::SetOutput(o);

  this->ClearOut();

  vtkPolyData *out=dynamic_cast<vtkPolyData*>(o);
  if (out==0)
    {
    cerr << "Error: Out must be polydata. " << o->GetClassName() << endl;
    return;
    }

  vtkPoints *opts=vtkPoints::New();
  out->SetPoints(opts);
  opts->Delete();
  this->OutPts=dynamic_cast<vtkFloatArray*>(opts->GetData());
  this->OutPts->Register(0);

  this->OutCells=vtkCellArray::New();
  out->SetLines(this->OutCells);

  this->Length=vtkFloatArray::New();
  this->Length->SetName("length");
  this->Length->Register(0);
  out->GetCellData()->AddArray(this->Length);
}

//-----------------------------------------------------------------------------
int StreamlineData::InsertCells(IdBlock *SourceIds)
{
  vtkIdType startId=SourceIds->first();
  vtkIdType endId=SourceIds->last();

  // Cells are sequentially acccessed (not random) so explicitly 
  // skip all cells we aren't interested in.
  this->SourceCells->InitTraversal();
  for (vtkIdType i=0; i<startId; ++i)
    {
    vtkIdType n;
    vtkIdType *ptIds;
    SourceCells->GetNextCell(n,ptIds);
    }

  // for the cells assigned to us, generate seed points.
  vtkIdType lId=this->Lines.size();
  vtkIdType nSeeds=SourceIds->size();
  this->Lines.resize(lId+nSeeds,0);

  float *pSourcePts=this->SourcePts->GetPointer(0);

  // Compute the center of the cell, and use this for
  // a seed point.
  for (vtkIdType cId=startId; cId<endId; ++cId)
    {
    vtkIdType nPtIds;
    vtkIdType *ptIds;
    SourceCells->GetNextCell(nPtIds,ptIds);

    // the seed point we will use the center of the cell
    double seed[3]={0.0};
    for (vtkIdType pId=0; pId<nPtIds; ++pId)
      {
      vtkIdType idx=3*ptIds[pId];
      // compute contribution to cell center.
      seed[0]+=pSourcePts[idx  ];
      seed[1]+=pSourcePts[idx+1];
      seed[2]+=pSourcePts[idx+2];
      }
    // finsih the seed point computation (at cell center).
    seed[0]/=nPtIds;
    seed[1]/=nPtIds;
    seed[2]/=nPtIds;

    this->Lines[lId]=new FieldLine(seed,cId);
    this->Lines[lId]->AllocateTrace();
    ++lId;
    }

  return nSeeds;
}

//-----------------------------------------------------------------------------
int StreamlineData::SyncGeometry()
{
  size_t nLines=this->Lines.size();

  vtkIdType nNewPtsTotal=0;
  for (size_t i=0; i<nLines; ++i)
    {
    nNewPtsTotal+=this->Lines[i]->GetNumberOfPoints();
    }
  if (nNewPtsTotal==0)
    {
    return 1;
    }

  vtkIdType nLinePts=this->OutPts->GetNumberOfTuples();
  float *pLinePts=this->OutPts->WritePointer(3*nLinePts,3*nNewPtsTotal);

  vtkIdTypeArray *lineCells=this->OutCells->GetData();
  vtkIdType *pLineCells
    = lineCells->WritePointer(lineCells->GetNumberOfTuples(),nNewPtsTotal+nLines);

  //
  this->OutCells->SetNumberOfCells(this->OutCells->GetNumberOfCells()+nLines);

  float *pLength
    = this->Length->WritePointer(this->Length->GetNumberOfTuples(),nLines);

  vtkIdType ptId=nLinePts;

  for (size_t i=0; i<nLines; ++i)
    {
    // copy the points
    vtkIdType nNewPts=this->Lines[i]->CopyPoints(pLinePts);

    // compute the arc-length
    *pLength=0.0;
    for (int q=1; q<nNewPts; ++q)
      {
      int q0=3*(q-1);
      int q1=3*q;
      float x=pLinePts[q1  ]-pLinePts[q0  ];
      float y=pLinePts[q1+1]-pLinePts[q0+1];
      float z=pLinePts[q1+2]-pLinePts[q0+2];
      *pLength+=sqrt(x*x+y*y+z*z);
      }
    ++pLength;

    pLinePts+=3*nNewPts;

    // build the cell
    *pLineCells=nNewPts;
    ++pLineCells;
    for (vtkIdType q=0; q<nNewPts; ++q)
      {
      *pLineCells=ptId;
      ++pLineCells;
      ++ptId;
      }
    }

  return 1;
}


