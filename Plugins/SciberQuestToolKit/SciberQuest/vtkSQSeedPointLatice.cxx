/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
/*=====================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQSeedPointLatice.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=====================*/
#include "vtkSQSeedPointLatice.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkType.h"
#include "Tuple.hxx"
#include "vtkPVInformationKeys.h"

// #define SQTK_DEBUG

//*****************************************************************************
static
inline
void indexToIJK(int idx, int nx, int nxy, int &i, int &j, int &k)
{
  // convert a flat array index into a i,j,k three space tuple.
  k=idx/nxy;
  j=(idx-k*nxy)/nx;
  i=idx-k*nxy-j*nx;
}

//*****************************************************************************
template <typename T>
static
void linspace(T lo, T hi, int n, T *data)
{
  if (n==1)
    {
    data[0]=(hi+lo)/((T)2);
    return;
    }

  T delta=(hi-lo)/((T)(n-1));

  for (int i=0; i<n; ++i)
    {
    data[i]=lo+((T)i)*delta;
    }
}

//*****************************************************************************
template <typename T>
static
void logspace(T lo, T hi, int n, T p, T *data)
{
  int mid=n/2;
  int nlo=mid;
  int nhi=n-mid;
  T s=hi-lo;

  T rhi=((T)pow(((T)10),p));

  linspace<T>(((T)1),((T)0.99)*rhi,nlo,data);
  linspace<T>(1.0,rhi,nhi,data+nlo);

  int i=0;
  for (; i<nlo; ++i)
    {
    data[i]=lo+s*(((T)0.5)*((T)log10(data[i]))/p);
    }
  for (; i<n; ++i)
    {
    data[i]=lo+s*(((T)1)-((T)log10(data[i]))/(((T)2)*p));
    }
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQSeedPointLatice);

//----------------------------------------------------------------------------
vtkSQSeedPointLatice::vtkSQSeedPointLatice()
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::vtkSQSeedPointLatice" << std::endl;
  #endif


  this->NX[0]=this->NX[1]=this->NX[2]=4;

  this->Bounds[0]=this->Bounds[2]=this->Bounds[4]=0.0;
  this->Bounds[1]=this->Bounds[3]=this->Bounds[5]=1.0;

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkSQSeedPointLatice::~vtkSQSeedPointLatice()
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::~vtkSQSeedPointLatice" << std::endl;
  #endif

}

//----------------------------------------------------------------------------
void vtkSQSeedPointLatice::SetTransformPower(double itp, double jtp, double ktp)
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::SetTransformPower" << std::endl;
  #endif


  double tp[3]={itp,jtp,ktp};
  this->SetTransformPower(tp);
}

//----------------------------------------------------------------------------
void vtkSQSeedPointLatice::SetTransformPower(double *tp)
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::SetTransformPower" << std::endl;
  #endif


  if (tp[0]<0.0) vtkErrorMacro("Negative transform power i unsupported.");
  if (tp[1]<0.0) vtkErrorMacro("Negative transform power j unsupported.");
  if (tp[2]<0.0) vtkErrorMacro("Negative transform power k unsupported.");

  this->Power[0]=tp[0];
  this->Power[1]=tp[1];
  this->Power[2]=tp[2];

  this->Transform[0]=(tp[0]<0.25?TRANSFORM_NONE:TRANSFORM_LOG);
  this->Transform[1]=(tp[1]<0.25?TRANSFORM_NONE:TRANSFORM_LOG);
  this->Transform[2]=(tp[2]<0.25?TRANSFORM_NONE:TRANSFORM_LOG);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSQSeedPointLatice::SetIBounds(double lo, double hi)
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::SetIBounds" << std::endl;
  #endif


  this->Bounds[0]=lo;
  this->Bounds[1]=hi;

  this->Modified();
}

//----------------------------------------------------------------------------
double *vtkSQSeedPointLatice::GetIBounds()
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::GetIBounds" << std::endl;
  #endif

  return this->Bounds;
}



//----------------------------------------------------------------------------
void vtkSQSeedPointLatice::SetJBounds(double lo, double hi)
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::SetJBounds" << std::endl;
  #endif


  this->Bounds[2]=lo;
  this->Bounds[3]=hi;

  this->Modified();
}

//----------------------------------------------------------------------------
double *vtkSQSeedPointLatice::GetJBounds()
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::GetJBounds" << std::endl;
  #endif

  return this->Bounds+2;
}


//----------------------------------------------------------------------------
void vtkSQSeedPointLatice::SetKBounds(double lo, double hi)
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::SetKBounds" << std::endl;
  #endif


  this->Bounds[4]=lo;
  this->Bounds[5]=hi;

  this->Modified();
}

//----------------------------------------------------------------------------
double *vtkSQSeedPointLatice::GetKBounds()
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::GetKBounds" << std::endl;
  #endif

  return this->Bounds+4;
}

//----------------------------------------------------------------------------
int vtkSQSeedPointLatice::FillInputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::FillInputPortInformation" << std::endl;
  #endif


  // The input is optional,if present it will be used
  // for bounds.
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQSeedPointLatice::RequestInformation(
    vtkInformation * /*req*/,
    vtkInformationVector ** /*inInfos*/,
    vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::RequestInformation" << std::endl;
  #endif

  // tell the excutive that we are handling our own paralelization.
  vtkInformation *outInfo=outInfos->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  // TODO extract bounds and set if the input data set is present.

  return 1;
}

//----------------------------------------------------------------------------
int vtkSQSeedPointLatice::RequestData(
    vtkInformation * /*req*/,
    vtkInformationVector **inInfos,
    vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
    std::cerr << "=====vtkSQSeedPointLatice::RequestData" << std::endl;
  #endif


  vtkInformation *outInfo=outInfos->GetInformationObject(0);

  vtkPolyData *output
    = dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
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

  // domain decomposition
  int nPoints=this->NX[0]*this->NX[1]*this->NX[2];
  int pieceSize=nPoints/nPieces;
  int nLarge=nPoints%nPieces;
  int nLocal=pieceSize+(pieceNo<nLarge?1:0);
  int startId=pieceSize*pieceNo+(pieceNo<nLarge?pieceNo:nLarge);
  int endId=startId+nLocal;

  // If the input is present then use it for bounds
  // otherwise we assume the user has previously set
  // desired bounds.
  vtkInformation *inInfo=0;
  if (inInfos[0]->GetNumberOfInformationObjects())
    {
    inInfo=inInfos[0]->GetInformationObject(0);
    vtkDataSet *input
      = dynamic_cast<vtkDataSet*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (input)
      {
      if (!inInfo->Has(vtkPVInformationKeys::WHOLE_BOUNDING_BOX()))
        {
        vtkErrorMacro("Input must have WHOLE_BOUNDING_BOX set.");
        return 1;
        }
      double bounds[6];
      inInfo->Get(vtkPVInformationKeys::WHOLE_BOUNDING_BOX(),bounds);

      double dX[3];
      dX[0]=(this->Bounds[1]-this->Bounds[0])/((double)this->NX[0]);
      dX[1]=(this->Bounds[3]-this->Bounds[2])/((double)this->NX[1]);
      dX[2]=(this->Bounds[5]-this->Bounds[4])/((double)this->NX[2]);

      bounds[0]+=dX[0]/2.0;
      bounds[1]-=dX[0]/2.0;
      bounds[2]+=dX[1]/2.0;
      bounds[3]-=dX[1]/2.0;
      bounds[4]+=dX[2]/2.0;
      bounds[5]-=dX[2]/2.0;

      this->SetBounds(bounds);
      }
    }

  // generate the i,j,k coordinate axes
  // note these are not decompoesed, TODO decomposition.
  float *axes[3]={NULL};
  for (int q=0; q<3; ++q)
    {
    axes[q]=new float [this->NX[q]];

    switch (this->Transform[q])
      {
      case TRANSFORM_NONE:
        linspace<float>(
            ((float)this->Bounds[2*q]),
            ((float)this->Bounds[2*q+1]),
            this->NX[q],
            axes[q]);
        break;

      case TRANSFORM_LOG:
        logspace<float>(
            ((float)this->Bounds[2*q]),
            ((float)this->Bounds[2*q+1]),
            this->NX[q],
            ((float)this->Power[q]),
            axes[q]);
        break;

      default:
        vtkErrorMacro("Unsupported transform.");
        return 1;
      }
    }

  // Configure the output
  vtkFloatArray *X=vtkFloatArray::New();
  X->SetNumberOfComponents(3);
  X->SetNumberOfTuples(nLocal);
  float *pX=X->GetPointer(0);

  vtkPoints *pts=vtkPoints::New();
  pts->SetData(X);
  X->Delete();

  output->SetPoints(pts);
  pts->Delete();

  vtkIdTypeArray *ia=vtkIdTypeArray::New();
  ia->SetNumberOfComponents(1);
  ia->SetNumberOfTuples(2*nLocal);
  vtkIdType *pIa=ia->GetPointer(0);

  vtkCellArray *verts=vtkCellArray::New();
  verts->SetCells(nLocal,ia);
  ia->Delete();

  output->SetVerts(verts);
  verts->Delete();


  int nx=this->NX[0];
  int nxy=this->NX[0]*this->NX[1];

  double prog=0.0;
  double progUnit=1.0/nLocal;
  double progRepUnit=0.1;
  double progRepLevel=0.1;

  // generate the point set
  for (int idx=startId,pid=0; idx<endId; ++idx,++pid,prog+=progUnit)
    {
    // update PV progress
    if (prog>=progRepLevel)
      {
      this->UpdateProgress(prog);
      progRepLevel+=progRepUnit;
      }

    int i,j,k;
    indexToIJK(idx,nx,nxy,i,j,k);

    // new latice point
    pX[0]=(axes[0])[i];
    pX[1]=(axes[1])[j];
    pX[2]=(axes[2])[k];
    pX+=3;

    // insert the cell
    pIa[0]=1;
    pIa[1]=pid;
    pIa+=2;
    }

  delete [] axes[0];
  delete [] axes[1];
  delete [] axes[2];


  #ifdef SQTK_DEBUG
  int rank=vtkMultiProcessController::GetGlobalController()->GetLocalProcessId();
  std::cerr
    << "pieceNo = " << pieceNo << std::endl
    << "nPieces = " << nPieces << std::endl
    << "rank    = " << rank << std::endl
    << "nLocal  = " << nLocal << std::endl
    << "startId = " << startId << std::endl
    << "endId   = " << endId << std::endl
    << "NX=" << Tuple<int>(this->NX,3) << std::endl
    << "Bounds=" << Tuple<double>(this->Bounds,6) << std::endl;
  #endif

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQSeedPointLatice::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef SQTK_DEBUG
  std::cerr << "=====vtkSQSeedPointLatice::PrintSelf" << std::endl;
  #endif


  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfPoints: " << this->NumberOfPoints << "\n";
}
