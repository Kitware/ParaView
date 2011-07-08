/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "UnstructuredFieldTopologyMap.h"

#include "IdBlock.h"
#include "FieldLine.h"
#include "TerminationCondition.h"
#include "vtkSQCellGenerator.h"
#include "vtkDataSet.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkIdTypeArray.h"

//-----------------------------------------------------------------------------
UnstructuredFieldTopologyMap::~UnstructuredFieldTopologyMap()
{
  this->ClearSource();
  this->ClearOut();
}

//-----------------------------------------------------------------------------
void UnstructuredFieldTopologyMap::ClearSource()
{
  if (this->SourceGen){ this->SourceGen->Delete(); }
  if (this->SourcePts){ this->SourcePts->Delete(); }
  if (this->SourceCells){ this->SourceCells->Delete(); }
  if (this->SourceTypes){ this->SourceTypes->Delete(); }
  this->SourceGen=0;
  this->SourcePts=0;
  this->SourceCells=0;
  this->SourceTypes=0;
  this->IdMap.clear();
}

//-----------------------------------------------------------------------------
void UnstructuredFieldTopologyMap::ClearOut()
{
  if (this->OutPts){ this->OutPts->Delete(); }
  if (this->OutCells){ this->OutCells->Delete(); }
  if (this->OutTypes){ this->OutTypes->Delete(); }
  if (this->OutLocs){ this->OutLocs->Delete(); }
  this->OutPts=0;
  this->OutCells=0;
  this->OutTypes=0;
  this->OutLocs=0;
  this->IdMap.clear();
}

//-----------------------------------------------------------------------------
void UnstructuredFieldTopologyMap::SetSource(vtkSQCellGenerator *sourceGen)
{
  if (this->SourceGen==sourceGen){ return; }
  if (this->SourceGen){ this->SourceGen->Delete(); }
  this->SourceGen=sourceGen;
  if (this->SourceGen){ this->SourceGen->Register(0); }
}

//-----------------------------------------------------------------------------
void UnstructuredFieldTopologyMap::SetSource(vtkDataSet *s)
{
  this->ClearSource();

  vtkUnstructuredGrid *source=dynamic_cast<vtkUnstructuredGrid*>(s);
  if (source==0)
    {
    cerr << "Error: Source must be unstructured. " << s->GetClassName() << endl;
    return;
    }

  this->SourcePts=dynamic_cast<vtkFloatArray*>(source->GetPoints()->GetData());
  if (this->SourcePts==0)
    {
    cerr << "Error: Points are not float precision." << endl;
    return;
    }
  this->SourcePts->Register(0);

  this->SourceCells=source->GetCells();
  this->SourceCells->Register(0);

  this->SourceTypes=source->GetCellTypesArray();
  this->SourceTypes->Register(0);
}

//-----------------------------------------------------------------------------
void UnstructuredFieldTopologyMap::SetOutput(vtkDataSet *o)
{
  this->FieldTraceData::SetOutput(o);

  this->ClearOut();

  vtkUnstructuredGrid *out=dynamic_cast<vtkUnstructuredGrid*>(o);
  if (out==0)
    {
    cerr << "Error: Out must be unstructured grid. " << o->GetClassName() << endl;
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
int UnstructuredFieldTopologyMap::InsertCells(IdBlock *SourceIds)
{
  if (this->SourceGen)
    {
    return this->InsertCellsFromGenerator(SourceIds);
    }
  return this->InsertCellsFromDataset(SourceIds);
}

//-----------------------------------------------------------------------------
int UnstructuredFieldTopologyMap::InsertCellsFromGenerator(IdBlock *SourceIds)
{
  vtkIdType startCellId=SourceIds->first();
  vtkIdType nCellsLocal=SourceIds->size();

//   // input points
//   float *pSourcePts=this->SourcePts->GetPointer(0);
//   unsigned char *pSourceTypes=this->SourceTypes->GetPointer(0);

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

  // field lines
  int lId=this->Lines.size();
  this->Lines.resize(lId+nCellsLocal,0);

  vector<float> sourcePts;
  vector<vtkIdType> sourceIds;

  vtkIdType sourceCellId=startCellId;

  // For each cell asigned to us we'll get its center to use as a
  // seed point and copy the cell into the output, The output only
  // will have the cells assigned to this process, while the input
  // may have all of the cells.
  for (vtkIdType i=0; i<nCellsLocal; ++i)
    {
    // get the cell that belong to us.
    vtkIdType nPtIds=this->SourceGen->GetNumberOfCellPoints(sourceCellId);
    sourcePts.resize(3*nPtIds);
    sourceIds.resize(nPtIds);

    this->SourceGen->GetCellPointIndexes(sourceCellId,&sourceIds[0]);
    this->SourceGen->GetCellPoints(sourceCellId,&sourcePts[0]);

    // set the new cell's location
    *pOutLocs=nCellIds;
    ++pOutLocs;

    // copy its type.
    *pOutTypes=this->SourceGen->GetCellType(sourceCellId);
    ++pOutTypes;

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
    // the  point we will use the center of the cell
    double seed[3]={0.0};
    // transfer from input to output (only what we own)
    for (vtkIdType j=0; j<nPtIds; ++j)
      {
      vtkIdType idx=3*j;
      // do we already have this point?
      MapElement elem(sourceIds[j],nOutPts);
      MapInsert ret=this->IdMap.insert(elem);
      if (ret.second==true)
        {
        // this point hasn't previsouly been coppied
        // copy the point.
        pOutPts[0]=sourcePts[idx  ];
        pOutPts[1]=sourcePts[idx+1];
        pOutPts[2]=sourcePts[idx+2];
        pOutPts+=3;
        ++nOutPts;
        }

      // insert the point id.
      *pOutCells=(*ret.first).second;
      ++pOutCells;

      // compute contribution to cell center.
      seed[0]+=sourcePts[idx  ];
      seed[1]+=sourcePts[idx+1];
      seed[2]+=sourcePts[idx+2];
      }
    // finsih the seed point computation (at cell center).
    seed[0]/=nPtIds;
    seed[1]/=nPtIds;
    seed[2]/=nPtIds;

    this->Lines[lId]=new FieldLine(seed,sourceCellId);
    ++sourceCellId;
    ++lId;
    }

  // correct the length of the point array, above we assumed 
  // that all points from each cell needed to be inserted
  // and allocated that much space.
  this->OutPts->Resize(nOutPts);

  return nCellsLocal;
}

//-----------------------------------------------------------------------------
int UnstructuredFieldTopologyMap::InsertCellsFromDataset(IdBlock *SourceIds)
{
  vtkIdType startCellId=SourceIds->first();
  vtkIdType nCellsLocal=SourceIds->size();

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
  unsigned char *pSourceTypes=this->SourceTypes->GetPointer(0);

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

  // field lines
  int lId=this->Lines.size();
  this->Lines.resize(lId+nCellsLocal,0);

  vtkIdType sourceCellId=startCellId;

  // For each cell asigned to us we'll get its center to use as a
  // seed point and copy the cell into the output, The output only
  // will have the cells assigned to this process, while the input
  // may have all of the cells.
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
    *pOutTypes=pSourceTypes[sourceCellId];
    ++pOutTypes;

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
    // the  point we will use the center of the cell
    double seed[3]={0.0};
    // transfer from input to output (only what we own)
    for (vtkIdType j=0; j<nPtIds; ++j)
      {
      vtkIdType idx=3*ptIds[j];
      // do we already have this point?
      MapElement elem(ptIds[j],nOutPts);
      MapInsert ret=this->IdMap.insert(elem);
      if (ret.second==true)
        {
        // this point hasn't previsouly been coppied
        // copy the point.
        pOutPts[0]=pSourcePts[idx  ];
        pOutPts[1]=pSourcePts[idx+1];
        pOutPts[2]=pSourcePts[idx+2];
        pOutPts+=3;

        // insert the new point id.
        *pOutCells=nOutPts;
        ++nOutPts;
        }
      else
        {
        // this point has been coppied, do not add a duplicate.
        // insert the other point id.
        *pOutCells=(*ret.first).second;
        }
      ++pOutCells;

      // compute contribution to cell center.
      seed[0]+=pSourcePts[idx  ];
      seed[1]+=pSourcePts[idx+1];
      seed[2]+=pSourcePts[idx+2];
      }
    // finsih the seed point computation (at cell center).
    seed[0]/=nPtIds;
    seed[1]/=nPtIds;
    seed[2]/=nPtIds;

    this->Lines[lId]=new FieldLine(seed,sourceCellId);
    ++sourceCellId;
    ++lId;
    }

  // correct the length of the point array, above we assumed 
  // that all points from each cell needed to be inserted
  // and allocated that much space.
  this->OutPts->Resize(nOutPts);

  return nCellsLocal;
}
