/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQEdgeFilter.h"

#include "SQVTKTemplateMacroWarningSupression.h"
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

#include <string>

#include "Numerics.hxx"

//#define SQTK_DEBUG

vtkStandardNewMacro(vtkSQEdgeFilter);

//-----------------------------------------------------------------------------
vtkSQEdgeFilter::vtkSQEdgeFilter()
    :
  PassInput(0),
  SplitComponents(0),
  ComputeGradient(0),
  ComputeLaplacian(0),
  Mode(CartesianExtent::DIM_MODE_3D)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQEdgeFilter::vtkSQEdgeFilter" << std::endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

}

//-----------------------------------------------------------------------------
vtkSQEdgeFilter::~vtkSQEdgeFilter()
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQEdgeFilter::~vtkSQEdgeFilter" << std::endl;
  #endif

}

//-----------------------------------------------------------------------------
int vtkSQEdgeFilter::RequestDataObject(
    vtkInformation* /* request */,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQEdgeFilter::RequestDataObject" << std::endl;
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
    //outData->SetPipelineInformation(outInfo);
    outData->Delete();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQEdgeFilter::RequestInformation(
      vtkInformation * /*req*/,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQEdgeFilter::RequestInformation" << std::endl;
  #endif
  //this->Superclass::RequestInformation(req,inInfos,outInfos);

  // We will work in a restricted problem domain so that we have
  // always a single layer of ghost cells available. To make it so
  // we'll take the upstream's domain and shrink it by half the
  // kernel width.
  int nGhosts = 1;

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

  #ifdef SQTK_DEBUG
  pCerr()
    << "WHOLE_EXTENT(input)=" << inputDomain << std::endl
    << "WHOLE_EXTENT(output)=" << outputDomain << std::endl
    << "ORIGIN=" << Tuple<double>(X0,3) << std::endl
    << "SPACING=" << Tuple<double>(dX,3) << std::endl
    << "nGhost=" << nGhosts << std::endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQEdgeFilter::RequestUpdateExtent(
      vtkInformation *req,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQEdgeFilter::RequestUpdateExtent" << std::endl;
  #endif

  (void)req;

  typedef vtkStreamingDemandDrivenPipeline vtkSDDPipeline;

  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);

  // We will modify the extents we request from our input so
  // that we will have a layers of ghost cells. We also pass
  // the number of ghosts through the piece based key.
  int nGhosts = 1;

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

  #ifdef SQTK_DEBUG
  pCerr()
    << "WHOLE_EXTENT=" << wholeExt << std::endl
    << "UPDATE_EXTENT=" << outputExt << std::endl
    << "nGhosts=" << nGhosts << std::endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQEdgeFilter::RequestData(
    vtkInformation * /*req*/,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQEdgeFilter::RequestData" << std::endl;
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
  CartesianExtent outputExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outputExt.GetData());
  CartesianExtent domainExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        domainExt.GetData());

  // Check that we have the ghost cells that we need (more is OK).
  int nGhost = 1;
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

  // NOTE You can't do a shallow copy because the array dimensions are
  // different on output and input because of the ghost layer.
  // outData->ShallowCopy(inData);

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

    #ifdef SQTK_DEBUG
    pCerr()
      << "WHOLE_EXTENT=" << domainExt << std::endl
      << "UPDATE_EXTENT(input)=" << inputExt << std::endl
      << "UPDATE_EXTENT(output)=" << outputExt << std::endl
      << "ORIGIN" << Tuple<double>(X0,3) << std::endl
      << "SPACING" << Tuple<double>(dX,3) << std::endl
      << std::endl;
    #endif

    vtkDataArray *V=this->GetInputArrayToProcess(0,inImData);

    if (!V->IsA("vtkFloatArray") && !V->IsA("vtkDoubleArray"))
      {
      vtkErrorMacro(
        << "This filter operates on vector floating point arrays."
        << "You provided " << V->GetClassName() << ".");
      return 1;
      }

    // Copy the input field, unfortunately it's a deep copy
    // since the input and output have different extents.
    if (this->PassInput)
      {
      vtkDataArray *W=V->NewInstance();
      outImData->GetPointData()->AddArray(W);
      W->Delete();
      int nCompsV=V->GetNumberOfComponents();
      W->SetNumberOfComponents(nCompsV);
      W->SetNumberOfTuples(outputTups);
      W->SetName(V->GetName());
      switch(V->GetDataType())
        {
        vtkFloatTemplateMacro(
          Copy<VTK_TT>(
              inputExt.GetData(),
              outputExt.GetData(),
              (VTK_TT*)V->GetVoidPointer(0),
              (VTK_TT*)W->GetVoidPointer(0),
              nCompsV,
              this->Mode,
              USE_OUTPUT_BOUNDS));
        }
      }

    // Gradient.
    if (this->ComputeGradient)
      {
      std::string name;

      vtkDataArray *Gx=V->NewInstance();
      Gx->SetNumberOfComponents(1);
      Gx->SetNumberOfTuples(outputTups);
      name="grad-";
      name+=V->GetName();
      name+="x";
      Gx->SetName(name.c_str());

      vtkDataArray *Gy=V->NewInstance();
      Gy->SetNumberOfComponents(1);
      Gy->SetNumberOfTuples(outputTups);
      name="grad-";
      name+=V->GetName();
      name+="y";
      Gy->SetName(name.c_str());

      vtkDataArray *Gz=V->NewInstance();
      Gz->SetNumberOfComponents(1);
      Gz->SetNumberOfTuples(outputTups);
      name="grad-";
      name+=V->GetName();
      name+="z";
      Gz->SetName(name.c_str());

      switch(V->GetDataType())
        {
        vtkFloatTemplateMacro(
          Gradient<VTK_TT>(
              inputExt.GetData(),
              outputExt.GetData(),
              this->Mode,
              dX,
              (VTK_TT*)V->GetVoidPointer(0),
              (VTK_TT*)Gx->GetVoidPointer(0),
              (VTK_TT*)Gy->GetVoidPointer(0),
              (VTK_TT*)Gz->GetVoidPointer(0));
          );
        }

      if (this->SplitComponents)
        {
        outImData->GetPointData()->AddArray(Gx);
        outImData->GetPointData()->AddArray(Gy);
        outImData->GetPointData()->AddArray(Gz);
        }
      else
        {
        vtkDataArray *G=V->NewInstance();
        outImData->GetPointData()->AddArray(G);
        G->Delete();
        G->SetNumberOfComponents(3);
        G->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        G->SetName(name.c_str());

        switch(V->GetDataType())
          {
          vtkFloatTemplateMacro(
            Interleave<VTK_TT>(
                outputTups,
                (VTK_TT*)Gx->GetVoidPointer(0),
                (VTK_TT*)Gy->GetVoidPointer(0),
                (VTK_TT*)Gz->GetVoidPointer(0),
                (VTK_TT*)G->GetVoidPointer(0)));
          }
        }
      Gx->Delete();
      Gy->Delete();
      Gz->Delete();
      }

    // Laplacian.
    if (this->ComputeLaplacian)
      {
      vtkDataArray *L=V->NewInstance();
      outImData->GetPointData()->AddArray(L);
      L->Delete();
      L->SetNumberOfComponents(1);
      L->SetNumberOfTuples(outputTups);
      std::string name("lapl-");
      name+=V->GetName();
      L->SetName(name.c_str());
      //
      switch(V->GetDataType())
        {
        vtkFloatTemplateMacro(
          Laplacian<VTK_TT>(
              inputExt.GetData(),
              outputExt.GetData(),
              this->Mode,
              dX,
              (VTK_TT*)V->GetVoidPointer(0),
              (VTK_TT*)L->GetVoidPointer(0)));
        }
      }
    // outImData->Print(std::cerr);
    }
  else
  if (isRecti)
    {
    vtkWarningMacro("TODO : implment difference opperators on stretched grids.");
    }

 return 1;
}

//-----------------------------------------------------------------------------
void vtkSQEdgeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQEdgeFilter::PrintSelf" << std::endl;
  #endif

  this->Superclass::PrintSelf(os,indent);
  // TODO
}
