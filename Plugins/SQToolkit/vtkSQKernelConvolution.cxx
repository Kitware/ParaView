/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.

*/
#include "vtkSQKernelConvolution.h"

#include "CartesianExtent.h"
#include "CartesianExtent.h"
#include "postream.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <vtkstd/string>
using vtkstd::string;

#include "Numerics.hxx"

// #define vtkSQKernelConvolutionDEBUG

vtkCxxRevisionMacro(vtkSQKernelConvolution, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQKernelConvolution);

//-----------------------------------------------------------------------------
vtkSQKernelConvolution::vtkSQKernelConvolution()
    :
  KernelWidth(3),
  KernelType(KERNEL_TYPE_GAUSIAN),
  Kernel(0),
  KernelModified(1),
  Mode(CartesianExtent::DIM_MODE_3D),
  NumberOfIterations(1)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::vtkSQKernelConvolution" << endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQKernelConvolution::~vtkSQKernelConvolution()
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::~vtkSQKernelConvolution" << endl;
  #endif

  if (this->Kernel)
    {
    delete [] this->Kernel;
    this->Kernel=0;
    }
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetMode(int mode)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::SetMode" << endl;
  #endif

  if (mode==this->Mode)
    {
    return;
    }

  this->Mode=mode;
  this->Modified();
  this->KernelModified=1;
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetKernelType(int type)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::SetKernelType" << endl;
  #endif

  if (type==this->KernelType)
    {
    return;
    }

  this->KernelType=type;
  this->Modified();
  this->KernelModified=1;
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetKernelWidth(int width)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::SetKernelWidth" << endl;
  #endif

  if (width==this->KernelWidth)
    {
    return;
    }

  if ((this->KernelWidth%2)==0)
    {
    vtkErrorMacro("KernelWidth must be odd.");
    return;
    }

  this->KernelWidth=width;
  this->Modified();
  this->KernelModified=1;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::UpdateKernel()
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::UpdateKernel" << endl;
  #endif

  if (!this->KernelModified)
    {
    return 0;
    }

  if (this->Kernel)
    {
    delete [] this->Kernel;
    this->Kernel=0;
    }

  int nk2 = this->KernelWidth/2;
  CartesianExtent ext(-nk2, nk2, -nk2, nk2, -nk2, nk2);
  switch(this->Mode)
    {
    case CartesianExtent::DIM_MODE_2D_XY:
      ext[4]=0;
      ext[5]=0;
      break;
    case CartesianExtent::DIM_MODE_2D_XZ:
      ext[2]=0;
      ext[3]=0;
      break;
    case CartesianExtent::DIM_MODE_2D_YZ:
      ext[0]=0;
      ext[1]=0;
      break;
    case CartesianExtent::DIM_MODE_3D:
      break;
    }
  this->KernelExt=ext;

  int size = ext.Size();

  this->Kernel=new float [size];
  float kernelNorm=0.0;

  if (this->KernelType==KERNEL_TYPE_GAUSIAN)
    {
    float *X=new float[this->KernelWidth];
    linspace<float>(-1.0,1.0, this->KernelWidth, X);

    float B[3]={0.0,0.0,0.0};
    float a=1.0;
    float c=0.55;

    int H=(this->Mode==CartesianExtent::DIM_MODE_3D?this->KernelWidth:1);

    for (int k=0; k<H; ++k)
      {
      for (int j=0; j<this->KernelWidth; ++j)
        {
        for (int i=0; i<this->KernelWidth; ++i)
          {
          float x[3]={X[i],X[j],this->Mode==CartesianExtent::DIM_MODE_3D?X[k]:0.0};

          int q = this->KernelWidth*this->KernelWidth*k+this->KernelWidth*j+i;

          this->Kernel[q]=Gaussian(x,a,B,c);
          kernelNorm+=this->Kernel[q];
          }
        }
      }
    }
  else
  if (this->KernelType==KERNEL_TYPE_CONSTANT)
    {
    kernelNorm=size;
    for (int i=0; i<size; ++i)
      {
      this->Kernel[i]=1.0;
      }
    }
  else
    {
    vtkErrorMacro("Unsupported KernelType " << this->KernelType << ".");
    delete [] this->Kernel;
    this->Kernel=0;
    return -1;
    }

  // normalize
  for (int i=0; i<size; ++i)
    {
    this->Kernel[i]/=kernelNorm;
    }

  this->KernelModified = 0;

  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "Kernel=[";
  for (int i=0; i<size; ++i)
    {
    cerr << this->Kernel[i] << " ";
    }
  cerr << "]" << endl;
  #endif

  return 0;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::RequestDataObject(
    vtkInformation* /* request */,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::RequestDataObject" << endl;
  #endif


  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());
  const char *inputType=inData->GetClassName();

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());

  if ( !outData || !outData->IsA(inputType))
    {
    outData=inData->NewInstance();
    outInfo->Set(vtkDataObject::DATA_TYPE_NAME(),inputType);
    outInfo->Set(vtkDataObject::DATA_OBJECT(),outData);
    outInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(), inData->GetExtentType());
    outData->SetPipelineInformation(outInfo);
    outData->Delete();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::RequestInformation(
      vtkInformation * /*req*/,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::RequestInformation" << endl;
  #endif
  //this->Superclass::RequestInformation(req,inInfos,outInfos);

  // We will work in a restricted problem domain so that we have
  // always a single layer of ghost cells available. To make it so
  // we'll take the upstream's domain and shrink it by half the 
  // kernel width.
  int nGhosts = this->KernelWidth/2;

  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);
  CartesianExtent inputDomain;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        inputDomain.GetData());

  // determine the dimensionality of the input.
  this->Mode
    = CartesianExtent::GetDimensionMode(
          inputDomain,
          nGhosts);

  // shrink the output problem domain by the requisite number of
  // ghost cells.
  CartesianExtent outputDomain
    = CartesianExtent::Grow(
          inputDomain,
          -nGhosts,
          this->Mode);

  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  outInfo->Set(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        outputDomain.GetData(),
        6);

  // other keys that need to be coppied
  double dX[3];
  inInfo->Get(vtkDataObject::SPACING(),dX);
  outInfo->Set(vtkDataObject::SPACING(),dX,3);

  double X0[3];
  inInfo->Get(vtkDataObject::ORIGIN(),X0);
  outInfo->Set(vtkDataObject::ORIGIN(),X0,3);

  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "WHOLE_EXTENT(input)=" << inputDomain << endl
    << "WHOLE_EXTENT(output)=" << outputDomain << endl
    << "ORIGIN=" << Tuple<double>(X0,3) << endl
    << "SPACING=" << Tuple<double>(dX,3) << endl
    << "nGhost=" << nGhosts << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::RequestUpdateExtent(
      vtkInformation *req,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::RequestUpdateExtent" << endl;
  #endif

  typedef vtkStreamingDemandDrivenPipeline vtkSDDPipeline;

  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);

  // We will modify the extents we request from our input so
  // that we will have a layers of ghost cells. We also pass
  // the number of ghosts through the piece based key.
  int nGhosts = this->KernelWidth/2;

  inInfo->Set(
        vtkSDDPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
        nGhosts);

  CartesianExtent outputExt;
  outInfo->Get(
        vtkSDDPipeline::UPDATE_EXTENT(),
        outputExt.GetData());

  CartesianExtent wholeExt;
  inInfo->Get(
        vtkSDDPipeline::WHOLE_EXTENT(),
        wholeExt.GetData());

  outputExt = CartesianExtent::Grow(
        outputExt,
        wholeExt,
        nGhosts,
        this->Mode);

  inInfo->Set(
        vtkSDDPipeline::UPDATE_EXTENT(),
        outputExt.GetData(),
        6);

  int piece
    = outInfo->Get(vtkSDDPipeline::UPDATE_PIECE_NUMBER());

  int numPieces
    = outInfo->Get(vtkSDDPipeline::UPDATE_NUMBER_OF_PIECES());

  inInfo->Set(vtkSDDPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkSDDPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkSDDPipeline::EXACT_EXTENT(), 1);

  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "WHOLE_EXTENT=" << wholeExt << endl
    << "UPDATE_EXTENT=" << outputExt << endl
    << "nGhosts=" << nGhosts << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::RequestData(
    vtkInformation * /*req*/,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::RequestData" << endl;
  #endif

  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());

  // Guard against empty input.
  if (!inData || !outData)
    {
    vtkErrorMacro(
      << "Empty input(" << inData << ") or output(" << outData << ") detected.");
    return 1;
    }
  // We need extent based data here.
  int isImage=inData->IsA("vtkImageData");
  int isRecti=inData->IsA("vtkrectilinearGrid");
  if (!isImage && !isRecti)
    {
    vtkErrorMacro(
      << "This filter is designed for vtkStructuredData and subclasses."
      << "You are trying to use it with " << inData->GetClassName() << ".");
    return 1;
    }

  // Get the input and output extents.
  CartesianExtent inputExt;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        inputExt.GetData());

  CartesianExtent inputDom;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        inputDom.GetData());

  CartesianExtent outputExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outputExt.GetData());

  CartesianExtent domainExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        domainExt.GetData());

  // Check that we have the ghost cells that we need (more is OK).
  int nGhost = this->KernelWidth/2;

  CartesianExtent inputBox(inputExt);
  CartesianExtent outputBox
    = CartesianExtent::Grow(outputExt, nGhost, this->Mode);

  if (!inputBox.Contains(outputBox))
    {
    vtkErrorMacro(
      << "This filter requires ghost cells to function correctly. "
      << "The input must conatin the output plus " << nGhost
      << " layers of ghosts. The input is " << inputBox
      << ", but it must be at least "
      << outputBox << ".");
    return 1;
    }

  // generate the requested kernel, if needed.
  if (this->UpdateKernel())
    {
    vtkErrorMacro("Failed to create the requested kernel.");
    return 1;
    }

  // NOTE You can't do a shallow copy because the array dimensions are
  // different on output and input because of the ghost layers.

  if (isImage)
    {
    vtkImageData *inImData=dynamic_cast<vtkImageData *>(inData);
    vtkImageData *outImData=dynamic_cast<vtkImageData *>(outData);

    // set up the output.
    double X0[3];
    outInfo->Get(vtkDataObject::ORIGIN(),X0);
    outImData->SetOrigin(X0);

    double dX[3];
    outInfo->Get(vtkDataObject::SPACING(),dX);
    outImData->SetSpacing(dX);

    outImData->SetExtent(outputExt.GetData());

    int outputDims[3];
    outImData->GetDimensions(outputDims);
    int outputTups=outputDims[0]*outputDims[1]*outputDims[2];

    #ifdef vtkSQKernelConvolutionDEBUG
    pCerr()
      << "WHOLE_EXTENT=" << domainExt << endl
      << "UPDATE_EXTENT(input)=" << inputExt << endl
      << "UPDATE_EXTENT(output)=" << outputExt << endl
      << "ORIGIN" << Tuple<double>(X0,3) << endl
      << "SPACING" << Tuple<double>(dX,3) << endl
      << endl;
    #endif

    vtkDataArray *V=this->GetInputArrayToProcess(0,inImData);

    if (!V->IsA("vtkFloatArray") && !V->IsA("vtkDoubleArray"))
      {
      vtkErrorMacro(
        << "This filter operates on vector floating point arrays."
        << "You provided " << V->GetClassName() << ".");
      return 1;
      }

    int nComps = V->GetNumberOfComponents();

    vtkDataArray *W=V->NewInstance();
    W->SetNumberOfComponents(nComps);
    W->SetNumberOfTuples(outputTups);
    W->SetName(V->GetName());

    switch (V->GetDataType())
      {
      vtkTemplateMacro(
        Convolution<VTK_TT>(
            inputExt.GetData(),
            outputExt.GetData(),
            this->KernelExt.GetData(),
            nComps,
            this->Mode,
            (VTK_TT*)V->GetVoidPointer(0),
            (VTK_TT*)W->GetVoidPointer(0),
            this->Kernel));
      }

    outImData->GetPointData()->AddArray(W);
    W->Delete();

    // outImData->Print(cerr);
    }
  else
  if (isRecti)
    {
    vtkWarningMacro("TODO : implment difference opperators on stretched grids.");
    }

 return 1;
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "===============================vtkSQKernelConvolution::PrintSelf" << endl;
  #endif

  this->Superclass::PrintSelf(os,indent);

  // TODO

}


