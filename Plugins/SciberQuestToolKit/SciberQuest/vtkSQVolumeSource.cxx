/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
/*=====================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQVolumeSource.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=====================*/
#include "vtkSQVolumeSource.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkType.h"
#include "vtkCellType.h"
#include "Numerics.hxx"
#include "XMLUtils.h"
#include "Tuple.hxx"
#include "vtkSQVolumeSourceCellGenerator.h"
#include "vtkSQCellGenerator.h"
#include "vtkSQLog.h"

#include <map>

typedef std::pair<std::map<vtkIdType,vtkIdType>::iterator,bool> MapInsert;
typedef std::pair<vtkIdType,vtkIdType> MapElement;

// #define SQTK_DEBUG

vtkStandardNewMacro(vtkSQVolumeSource);

//----------------------------------------------------------------------------
vtkSQVolumeSource::vtkSQVolumeSource()
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQVolumeSource::vtkSQVolumeSource" << std::endl;
  #endif

  this->ImmediateMode=1;

  this->Resolution[0]=
  this->Resolution[1]=
  this->Resolution[2]=1;

  this->Origin[0]=
  this->Origin[1]=
  this->Origin[2]=0.0;

  this->Point1[0]=1.0;
  this->Point1[1]=0.0;
  this->Point1[2]=0.0;

  this->Point2[0]=0.0;
  this->Point2[1]=1.0;
  this->Point2[2]=0.0;

  this->Point3[0]=0.0;
  this->Point3[1]=0.0;
  this->Point3[2]=1.0;

  this->LogLevel=0;

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkSQVolumeSource::~vtkSQVolumeSource()
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQVolumeSource::~vtkSQVolumeSource" << std::endl;
  #endif
}

//-----------------------------------------------------------------------------
int vtkSQVolumeSource::Initialize(vtkPVXMLElement *root)
{
  vtkPVXMLElement *elem=0;
  elem=GetOptionalElement(root,"vtkSQVolumeSource");
  if (elem==0)
    {
    return -1;
    }

  double origin[3]={0.0,0.0,0.0};
  GetOptionalAttribute<double,3>(elem,"origin",origin);
  this->SetOrigin(origin);

  double point1[3]={1.0,0.0,0.0};;
  GetOptionalAttribute<double,3>(elem,"point1",point1);
  this->SetPoint1(point1);

  double point2[3]={0.0,1.0,0.0};
  GetOptionalAttribute<double,3>(elem,"point2",point2);
  this->SetPoint2(point2);

  double point3[3]={0.0,1.0,0.0};
  GetOptionalAttribute<double,3>(elem,"point3",point3);
  this->SetPoint3(point3);

  int resolution[3]={1,1,1};
  GetOptionalAttribute<int,3>(elem,"resolution",resolution);
  this->SetResolution(resolution);

  int immediate_mode=1;
  GetOptionalAttribute<int,1>(elem,"immediate_mode",&immediate_mode);
  this->SetImmediateMode(immediate_mode);

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
     log->GetHeader()
      << "# ::vtkSQVolumeSource" << "\n"
      << "#   origin=" << origin[0] << ", " << origin[1] << ", " << origin[2] << "\n"
      << "#   point1=" << point1[0] << ", " << point1[1] << ", " << point1[2] << "\n"
      << "#   point2=" << point2[0] << ", " << point2[1] << ", " << point2[2] << "\n"
      << "#   point3=" << point3[0] << ", " << point3[1] << ", " << point3[2] << "\n"
      << "#   resolution=" << resolution[0] << ", " << resolution[1] << ", " << resolution[2] << "\n"
      << "#   immediate_mode=" << immediate_mode << "\n";
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkSQVolumeSource::RequestInformation(
    vtkInformation *req,
    vtkInformationVector **inInfos,
    vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQVolumeSource::RequestInformation" << std::endl;
  #endif

  (void)req;
  (void)inInfos;

  // tell the excutive that we are handling our own paralelization.
  vtkInformation *outInfo=outInfos->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  // TODO extract bounds and set if the input data set is present.

  return 1;
}

//----------------------------------------------------------------------------
int vtkSQVolumeSource::RequestData(
    vtkInformation *req,
    vtkInformationVector **inInfos,
    vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQVolumeSource::RequestData" << std::endl;
  #endif

  (void)req;
  (void)inInfos;

  vtkInformation *outInfo=outInfos->GetInformationObject(0);

  vtkUnstructuredGrid *output
    = dynamic_cast<vtkUnstructuredGrid*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (output==NULL)
    {
    vtkErrorMacro("Empty output.");
    return 1;
    }

  // paralelize by piece information.
  int pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // sanity - the requst cannot be fullfilled
  if (pieceNo>=nPieces)
    {
    output->Initialize();
    return 1;
    }

  // sanity - user set invalid number of cells
  if ( (this->Resolution[0]<1)
    || (this->Resolution[1]<1)
    || (this->Resolution[2]<1) )
    {
    vtkErrorMacro("Number of cells must be greater than 0.");
    return 1;
    }

  // The default domain decomposition of one cell per process
  // is used in demand mode. If immediate mode is on then these
  // will be reset.
  int nLocal=1;
  int startId=pieceNo;
  int endId=pieceNo+1;
  int resolution[3]={1,1,nPieces};

  if (!this->ImmediateMode)
    {
    // In demand mode a pseduo dataset is generated here for
    // display in PV, a demand plane source is inserted into
    // the pipeline for down stream access to any of the plane's
    // cell's
    vtkSQVolumeSourceCellGenerator *gen=vtkSQVolumeSourceCellGenerator::New();
    gen->SetOrigin(this->Origin);
    gen->SetPoint1(this->Point1);
    gen->SetPoint2(this->Point2);
    gen->SetPoint3(this->Point3);
    gen->SetResolution(this->Resolution);

    outInfo->Set(vtkSQCellGenerator::CELL_GENERATOR(),gen);
    gen->Delete();

    req->Append(
          vtkExecutive::KEYS_TO_COPY(),
          vtkSQCellGenerator::CELL_GENERATOR());
    }
  else
    {
    // Immediate mode domain decomposition
    int nCells=this->Resolution[0]*this->Resolution[1]*this->Resolution[2];
    int pieceSize=nCells/nPieces;
    int nLarge=nCells%nPieces;
    nLocal=pieceSize+(pieceNo<nLarge?1:0);
    startId=pieceSize*pieceNo+(pieceNo<nLarge?pieceNo:nLarge);
    endId=startId+nLocal;

    resolution[0]=this->Resolution[0];
    resolution[1]=this->Resolution[1];
    resolution[2]=this->Resolution[2];
    }

  // points
  vtkPoints *pts=vtkPoints::New();
  output->SetPoints(pts);
  pts->Delete();
  vtkFloatArray *X=dynamic_cast<vtkFloatArray*>(pts->GetData());
  float *pX=X->WritePointer(0,24*nLocal);
  vtkIdType ptId=0;

  // cells
  vtkCellArray *cells=vtkCellArray::New();
  vtkIdType *pCells=cells->WritePointer(nLocal,9*nLocal);

  // cell types
  vtkUnsignedCharArray *types=vtkUnsignedCharArray::New();
  types->SetNumberOfTuples(nLocal);
  unsigned char *pTypes=types->GetPointer(0);

  // cell locations
  vtkIdTypeArray *locs=vtkIdTypeArray::New();
  locs->SetNumberOfTuples(nLocal);
  vtkIdType *pLocs=locs->GetPointer(0);
  vtkIdType loc=0;

  // to prevent duplicate insertion of points
  std::map<vtkIdType,vtkIdType> usedPointIds;

  // cell generator
  vtkSQVolumeSourceCellGenerator *source=vtkSQVolumeSourceCellGenerator::New();
  source->SetOrigin(this->Origin);
  source->SetPoint1(this->Point1);
  source->SetPoint2(this->Point2);
  source->SetPoint3(this->Point3);
  source->SetResolution(resolution);

  for (int cid=startId; cid<endId; ++cid)
    {
    // get the grid indexes which are used as keys to
    // insure a single insertion of each coordinate.
    vtkIdType cpids[8];
    source->GetCellPointIndexes(cid,cpids);

    // get the poly's vertices coordinates
    float cpts[24];
    source->GetCellPoints(cid,cpts);

    pCells[0]=8; // set number of verts for this cell
    ++pCells;

    for (int i=0; i<8; ++i)
      {
      // Attempt an insert of the new point's index
      MapElement elem(cpids[i],ptId);
      MapInsert ins=usedPointIds.insert(elem);
      if (ins.second)
        {
        // this is a new point id and point. Add the point.
        int qq=3*i;
        pX[0]=cpts[qq];
        pX[1]=cpts[qq+1];
        pX[2]=cpts[qq+2];
        pX+=3;
        ++ptId;
        }
      // convert the index to a local pt id, and add to the cell.
      *pCells=(*ins.first).second;
      ++pCells;
      }

    // cell location
    *pLocs=loc;
    ++pLocs;
    loc+=9;

    // cell type
    *pTypes=VTK_HEXAHEDRON;
    ++pTypes;
    }

  // correct possible over-estimation.
  X->Resize(ptId);

  // transfer
  output->SetCells(types,locs,cells);

  types->Delete();
  locs->Delete();
  cells->Delete();

  source->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQVolumeSource::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQVolumeSource::PrintSelf" << std::endl;
  #endif

  this->Superclass::PrintSelf(os,indent);

  // TODO
}
