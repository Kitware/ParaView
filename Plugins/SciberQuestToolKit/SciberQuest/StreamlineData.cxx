/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
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
  if (this->SourceId){ this->SourceId->Delete(); }
  this->OutPts=0;
  this->OutCells=0;
  this->Length=0;
  this->SourceId=0;
}

//-----------------------------------------------------------------------------
void StreamlineData::SetSource(vtkSQCellGenerator *sourceGen)
{
  (void)sourceGen;

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
      std::cerr << "Error: Points are not float precision." << std::endl;
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
      std::cerr << "Error: Points are not float precision." << std::endl;
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
      std::cerr << "Error: Polydata doesn't have any supported cells." << std::endl;
      return;
      }
    this->SourceCells->Register(0);
    return;
    }

  std::cerr << "Error: Unsupported input data type." << std::endl;
}

//-----------------------------------------------------------------------------
void StreamlineData::SetOutput(vtkDataSet *o)
{
  this->FieldTopologyMapData::SetOutput(o);

  this->ClearOut();

  vtkPolyData *out=dynamic_cast<vtkPolyData*>(o);
  if (out==0)
    {
    std::cerr << "Error: Out must be polydata. " << o->GetClassName() << std::endl;
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
  out->GetCellData()->AddArray(this->Length);

  this->SourceId=vtkIntArray::New();
  this->SourceId->SetName("SourceId");
  out->GetCellData()->AddArray(this->SourceId);
}

//-----------------------------------------------------------------------------
vtkIdType StreamlineData::InsertCells(IdBlock *SourceIds)
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
  vtkIdType lId=(vtkIdType)this->Lines.size();
  vtkIdType nSeeds=(vtkIdType)SourceIds->size();
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
    float seed[3]={0.0f,0.0f,0.0f};
    for (vtkIdType pId=0; pId<nPtIds; ++pId)
      {
      vtkIdType idx=3*ptIds[pId];
      // compute contribution to cell center.
      seed[0]+=pSourcePts[idx  ];
      seed[1]+=pSourcePts[idx+1];
      seed[2]+=pSourcePts[idx+2];
      }
    // finsih the seed point computation (at cell center).
    seed[0]/=((float)nPtIds);
    seed[1]/=((float)nPtIds);
    seed[2]/=((float)nPtIds);

    this->Lines[lId]=new FieldLine(seed,this->SourceCellGid+cId);
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

  // resize cells
  this->OutCells->SetNumberOfCells(this->OutCells->GetNumberOfCells()+nLines);

  // resize scalars
  vtkIdType nExIds=this->SourceId->GetNumberOfTuples();
  int *pId=this->SourceId->WritePointer(nExIds,nLines);

  float *pLength
    = this->Length->WritePointer(this->Length->GetNumberOfTuples(),nLines);

  vtkIdType ptId=nLinePts;

  for (size_t i=0; i<nLines; ++i)
    {
    // copy the points
    vtkIdType nNewPts=this->Lines[i]->CopyPoints(pLinePts);

    // add gid
    *pId=(int)this->Lines[i]->GetSeedId();
    ++pId;

    // compute the arc-length
    *pLength=0.0f;
    for (int q=1; q<nNewPts; ++q)
      {
      int q0=3*(q-1);
      int q1=3*q;
      float x=pLinePts[q1  ]-pLinePts[q0  ];
      float y=pLinePts[q1+1]-pLinePts[q0+1];
      float z=pLinePts[q1+2]-pLinePts[q0+2];
      *pLength+=((float)sqrt(x*x+y*y+z*z));
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

//-----------------------------------------------------------------------------
void StreamlineData::CullPeriodicTransitions(double *bounds)
{
  float *pPts=this->OutPts->GetPointer(0);
  vtkIdType *pCells=this->OutCells->GetData()->GetPointer(0);
  vtkIdType nCells=this->OutCells->GetNumberOfCells();
  int *pCellIds=this->SourceId->GetPointer(0);
  float *pLen=this->Length->GetPointer(0);
  int *pColor=this->IntersectColor->GetPointer(0);

  float threshold[3] = {
      0.8f*(float)fabs(bounds[1]-bounds[0]),
      0.8f*(float)fabs(bounds[3]-bounds[2]),
      0.8f*(float)fabs(bounds[5]-bounds[4])};

  vtkIdTypeArray *newCells=vtkIdTypeArray::New();
  vtkIntArray *newCellIds=vtkIntArray::New();
  vtkFloatArray *newLen=vtkFloatArray::New();
  vtkIntArray *newColor=vtkIntArray::New();

  std::vector<vtkIdType> newCell;
  vtkIdType nNewCells=0;

  // traverse cells
  for (vtkIdType q=0; q<nCells; ++q)
    {
    vtkIdType nPts=*pCells;
    ++pCells;

    vtkIdType pid0=*pCells;

    // start a new cell, always add the first point
    // under the assumption the first segment will
    // not be a splitting point. if it is we correct
    // our mistake below.
    newCell.resize(nPts+1,0);
    newCell[0]=1;
    newCell[1]=pid0;

    // traverse cell pts
    for (vtkIdType i=1; i<nPts; ++i)
      {
      vtkIdType pid1=pCells[i];

      // compute segment length
      float *x0 = pPts+3*pid0;
      float *x1 = pPts+3*pid1;

      float dx=fabs(x1[0]-x0[0]);
      float dy=fabs(x1[1]-x0[1]);
      float dz=fabs(x1[2]-x0[2]);

      if ( (dx>=threshold[0]) || (dy>=threshold[1]) || (dz>=threshold[2]) )
        {
        if (newCell[0]>1)
          {
          // outside the threshold copy this poriton of the input
          // cell into the output
          vtkIdType n=newCell[0]+1;

          vtkIdType *pNewCell
            = newCells->WritePointer(newCells->GetNumberOfTuples(),n);

          for (vtkIdType j=0; j<n; ++j)
            {
            pNewCell[j]=newCell[j];
            }

          nNewCells+=1;

          // copy cell data
          int *pNewCellIds
            = newCellIds->WritePointer(newCellIds->GetNumberOfTuples(),1);

          *pNewCellIds=*pCellIds;

          float *pNewLen
            = newLen->WritePointer(newLen->GetNumberOfTuples(),1);

          *pNewLen=*pLen;

          int *pNewColor
            = newColor->WritePointer(newColor->GetNumberOfTuples(),1);

          *pNewColor=*pColor;

          // start a new output cell for the remainder of this
          // input cell
          newCell[0]=1;
          newCell[1]=pid1;
          }
        else
          {
          // the first segment was a splitting point, correct
          // our first point.
          newCell[1]=pid1;
          }
        }
      else
        {
        // inside the threshold add this point to the current
        // cell
        vtkIdType idx=newCell[0]+1;
        newCell[idx]=pid1;
        newCell[0]+=1;
        }

      pid0=pid1;
      }

    // copy what remains of the input cell into the output.
    // note, if the last segment is a splitting point there
    // will be nothing to do here.
    if (newCell[0]>1)
      {
      vtkIdType n=newCell[0]+1;

      vtkIdType *pNewCells
        = newCells->WritePointer(newCells->GetNumberOfTuples(),n);

      for (vtkIdType j=0; j<n; ++j)
        {
        pNewCells[j]=newCell[j];
        }

      nNewCells+=1;

      // copy id and length
      int *pNewCellIds
        = newCellIds->WritePointer(newCellIds->GetNumberOfTuples(),1);

      *pNewCellIds=*pCellIds;

      float *pNewLen
        = newLen->WritePointer(newLen->GetNumberOfTuples(),1);

      *pNewLen=*pLen;

      int *pNewColor
        = newColor->WritePointer(newColor->GetNumberOfTuples(),1);

      *pNewColor=*pColor;
      }

    // increment cell and cell data pointers
    pCells+=1;
    pLen+=1;
    pCellIds+=1;
    pColor+=1;
    }

  // update output
  this->OutCells->GetData()->DeepCopy(newCells);
  this->OutCells->SetNumberOfCells(nNewCells);
  newCells->Delete();

  this->Length->DeepCopy(newLen);
  newLen->Delete();

  this->SourceId->DeepCopy(newCellIds);
  newCellIds->Delete();

  this->IntersectColor->DeepCopy(newColor);
  newColor->Delete();
}
