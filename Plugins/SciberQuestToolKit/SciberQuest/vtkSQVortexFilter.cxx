/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQVortexFilter.h"

#include "SQVTKTemplateMacroWarningSupression.h"
#include "CartesianExtent.h"
#include "XMLUtils.h"
#include "postream.h"
#include "vtkSQLog.h"

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
#include "vtkPVXMLElement.h"

#include <string>
#include <utility>

#include "Numerics.hxx"

// #define SQTK_DEBUG
#ifdef WIN32
#undef SQTK_DEBUG
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQVortexFilter);

//-----------------------------------------------------------------------------
vtkSQVortexFilter::vtkSQVortexFilter()
    :
  SplitComponents(0),
  ResultMagnitude(0),
  ComputeRotation(1),
  ComputeHelicity(0),
  ComputeNormalizedHelicity(0),
  ComputeQ(0),
  ComputeLambda(0),
  ComputeLambda2(0),
  ComputeDivergence(0),
  ComputeGradient(0),
  ComputeEigenvalueDiagnostic(0),
  ComputeGradientDiagnostic(0),
  Mode(CartesianExtent::DIM_MODE_3D),
  LogLevel(0)

{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQVortexFilter::vtkSQVortexFilter" << std::endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQVortexFilter::~vtkSQVortexFilter()
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQVortexFilter::~vtkSQVortexFilter" << std::endl;
  #endif
}

//-----------------------------------------------------------------------------
int vtkSQVortexFilter::Initialize(vtkPVXMLElement *root)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQVortexFilter::Initialize" << std::endl;
  #endif

  vtkPVXMLElement *elem=GetRequiredElement(root,"vtkSQVortexFilter");
  if (elem==0)
    {
    sqErrorMacro(pCerr(),"Element for vtkSQVortexFilter is not present.");
    return -1;
    }

  // arrays to copy, optional
  elem=GetOptionalElement(elem,"arraysToCopy");
  if (elem)
    {
    ExtractValues(elem->GetCharacterData(),this->ArraysToCopy);
    }

  int splitComponents=0;
  GetOptionalAttribute<int,1>(elem,"splitComponents",&splitComponents);
  this->SetSplitComponents(splitComponents);

  int resultMagnitude=0;
  GetOptionalAttribute<int,1>(elem,"resultMagnitude",&resultMagnitude);
  this->SetResultMagnitude(resultMagnitude);

  int computeRotation=0;
  GetOptionalAttribute<int,1>(elem,"computeRotation",&computeRotation);
  this->SetComputeRotation(computeRotation);

  int computeHelicity=0;
  GetOptionalAttribute<int,1>(elem,"computeHelicity",&computeHelicity);
  this->SetComputeHelicity(computeHelicity);

  int computeNormalizedHelicity=0;
  GetOptionalAttribute<int,1>(elem,"computeNormalizedHelicity",&computeNormalizedHelicity);
  this->SetComputeNormalizedHelicity(computeNormalizedHelicity);

  int computeQ=0;
  GetOptionalAttribute<int,1>(elem,"computeQ",&computeQ);
  this->SetComputeQ(computeQ);

  int computeLambda=0;
  GetOptionalAttribute<int,1>(elem,"computeLambda",&computeLambda);
  this->SetComputeLambda(computeLambda);

  int computeLambda2=0;
  GetOptionalAttribute<int,1>(elem,"computeLambda2",&computeLambda2);
  this->SetComputeLambda2(computeLambda2);

  int computeDivergence=0;
  GetOptionalAttribute<int,1>(elem,"computeDivergence",&computeDivergence);
  this->SetComputeDivergence(computeDivergence);

  int computeGradient=0;
  GetOptionalAttribute<int,1>(elem,"computeGradient",&computeGradient);
  this->SetComputeGradient(computeGradient);

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->GetHeader()
      << "# ::vtkSQVortexFilter" << "\n"
      //<< "#   passInput=" << passInput << "\n" TODO
      << "#   resultMagnitude=" << this->ResultMagnitude << "\n"
      << "#   splitComponents=" << this->SplitComponents << "\n"
      << "#   computeRotation=" << this->ComputeRotation << "\n"
      << "#   computeHelicity=" << this->ComputeHelicity << "\n"
      << "#   computeNormalizedHelicity=" << this->ComputeNormalizedHelicity << "\n"
      << "#   computeQ=" << this->ComputeQ << "\n"
      << "#   computeLambda=" << this->ComputeLambda << "\n"
      << "#   computeLambda2=" << this->ComputeLambda2 << "\n"
      << "#   computeDivergence=" << this->ComputeDivergence << "\n"
      << "#   computeGradient=" << this->ComputeGradient << "\n"
      << "#   arraysToCopy=";

    std::set<std::string>::iterator it=this->ArraysToCopy.begin();
    std::set<std::string>::iterator end=this->ArraysToCopy.end();
    for (; it!=end; ++it)
      {
      log->GetHeader() << " " << *it;
      }
    log->GetHeader() << "\n";
    }

  return 0;
}

//-----------------------------------------------------------------------------
void vtkSQVortexFilter::AddInputArray(const char *name)
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQVortexFilter::AddInputArray"
    << "name=" << name << std::endl;
  #endif

  if (this->InputArrays.insert(name).second)
    {
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQVortexFilter::ClearInputArrays()
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQVortexFilter::ClearInputArrays" << std::endl;
  #endif

  if (this->InputArrays.size())
    {
    this->InputArrays.clear();
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQVortexFilter::AddArrayToCopy(const char *name)
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQVortexFilter::ArraysToCopy" << std::endl
    << "name=" << name << std::endl;
  #endif

  if (this->ArraysToCopy.insert(name).second)
    {
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQVortexFilter::ClearArraysToCopy()
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQVortexFilter::ClearArraysToCopy" << std::endl;
  #endif

  if (this->ArraysToCopy.size())
    {
    this->ArraysToCopy.clear();
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
int vtkSQVortexFilter::RequestDataObject(
    vtkInformation* request,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQVortexFilter::RequestDataObject" << std::endl;
  #endif

  (void)request;

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
    outData->Delete();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQVortexFilter::RequestInformation(
      vtkInformation *req,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQVortexFilter::RequestInformation" << std::endl;
  #endif

  (void)req;

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
int vtkSQVortexFilter::RequestUpdateExtent(
      vtkInformation *req,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQVortexFilter::RequestUpdateExtent" << std::endl;
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
int vtkSQVortexFilter::RequestData(
    vtkInformation *req,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQVortexFilter::RequestData" << std::endl;
  #endif

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->StartEvent("vtkSQVortexFilter::RequestData");
    }

  (void)req;

  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataSet *inData=vtkDataSet::GetData(inInfo);

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataSet *outData=vtkDataSet::GetData(outInfo);

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

  int nGhost = 1;

  // Get the input and output extents.
  CartesianExtent inputExt;
  inData->GetInformation()->Get(vtkDataObject::DATA_EXTENT(),
                                inputExt.GetData());


  CartesianExtent outputExt = CartesianExtent::Grow(
          inputExt,
          -nGhost,
          this->Mode);

  CartesianExtent domainExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        domainExt.GetData());

  if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()) > 1)
    {
    vtkDataArray* gl = inData->GetCellData()->GetArray("vtkGhostLevels");
    if (gl)
      {
      double range[2];
      gl->GetRange(range);
      if (range[1] < nGhost)
        {
        vtkErrorMacro("Did not receive " << nGhost << " ghost levels as requested."
          " Received " << range[1] << " instead. Cannot execute.")
        return 0;
        }
      }
    else
      {
      vtkErrorMacro("Did not receive ghost levels. Cannot execute.");
      return 0;
      }
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


    // Copy the input field, unfortunately it's a deep copy
    // since the input and output have different extents.
    if (this->LogLevel || globalLogLevel)
      {
      log->StartEvent("vtkSQVortexFilter::PassInput");
      }
    std::set<std::string>::iterator it;
    std::set<std::string>::iterator begin=this->ArraysToCopy.begin();
    std::set<std::string>::iterator end=this->ArraysToCopy.end();
    for (it=begin; it!=end; ++it)
      {
      vtkDataArray *M=inImData->GetPointData()->GetArray((*it).c_str());
      if (M==0)
        {
        vtkErrorMacro(
          << "Array " << (*it).c_str()
          << " was requested but is not present");
        continue;
        }

      vtkDataArray *W=M->NewInstance();
      outImData->GetPointData()->AddArray(W);
      W->Delete();
      int nCompsM=M->GetNumberOfComponents();
      W->SetNumberOfComponents(nCompsM);
      W->SetNumberOfTuples(outputTups);
      W->SetName(M->GetName());
      switch(M->GetDataType())
        {
        vtkTemplateMacro(
          Copy<VTK_TT>(
              inputExt.GetData(),
              outputExt.GetData(),
              (VTK_TT*)M->GetVoidPointer(0),
              (VTK_TT*)W->GetVoidPointer(0),
              nCompsM,
              this->Mode,
              USE_OUTPUT_BOUNDS));
        }
      }
    if (this->LogLevel || globalLogLevel)
      {
      log->EndEvent("vtkSQVortexFilter::PassInput");
      }

    begin=this->InputArrays.begin();
    end=this->InputArrays.end();
    for (it=begin; it!=end; ++it)
      {
      vtkDataArray *V=inImData->GetPointData()->GetArray((*it).c_str());
      if (V==0)
        {
        vtkErrorMacro(
          << "Array " << (*it).c_str()
          << " was requested but is not present");
        continue;
        }

      if (!V->IsA("vtkFloatArray") && !V->IsA("vtkDoubleArray"))
        {
        vtkErrorMacro(
          << "This filter operates on vector floating point arrays."
          << "You provided " << V->GetClassName() << ".");
        return 1;
        }

      // Rotation.
      if (this->ComputeRotation)
        {
        if (this->LogLevel || globalLogLevel)
          {
          log->StartEvent("vtkSQVortexFilter::Rotation");
          }
        std::string name;

        vtkDataArray *Rx=V->NewInstance();
        Rx->SetNumberOfComponents(1);
        Rx->SetNumberOfTuples(outputTups);
        name="rot-";
        name+=V->GetName();
        name+="x";
        Rx->SetName(name.c_str());

        vtkDataArray *Ry=V->NewInstance();
        Ry->SetNumberOfComponents(1);
        Ry->SetNumberOfTuples(outputTups);
        name="rot-";
        name+=V->GetName();
        name+="y";
        Ry->SetName(name.c_str());

        vtkDataArray *Rz=V->NewInstance();
        Rz->SetNumberOfComponents(1);
        Rz->SetNumberOfTuples(outputTups);
        name="rot-";
        name+=V->GetName();
        name+="z";
        Rz->SetName(name.c_str());

        switch(V->GetDataType())
          {
          vtkFloatTemplateMacro(
            Rotation<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                dX,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)Rx->GetVoidPointer(0),
                (VTK_TT*)Ry->GetVoidPointer(0),
                (VTK_TT*)Rz->GetVoidPointer(0));
            );
          }

        if (this->SplitComponents)
          {
          outImData->GetPointData()->AddArray(Rx);
          outImData->GetPointData()->AddArray(Ry);
          outImData->GetPointData()->AddArray(Rz);
          }
        else
          {
          vtkDataArray *R=V->NewInstance();
          outImData->GetPointData()->AddArray(R);
          R->Delete();
          R->SetNumberOfComponents(3);
          R->SetNumberOfTuples(outputTups);
          name="rot-";
          name+=V->GetName();
          R->SetName(name.c_str());

          switch(V->GetDataType())
            {
            vtkFloatTemplateMacro(
              Interleave<VTK_TT>(
                  outputTups,
                  (VTK_TT*)Rx->GetVoidPointer(0),
                  (VTK_TT*)Ry->GetVoidPointer(0),
                  (VTK_TT*)Rz->GetVoidPointer(0),
                  (VTK_TT*)R->GetVoidPointer(0)));
            }
          }
        Rx->Delete();
        Ry->Delete();
        Rz->Delete();
        if (this->LogLevel || globalLogLevel)
          {
          log->EndEvent("vtkSQVortexFilter::Rotation");
          }
        }

      // Helicity.
      if (this->ComputeHelicity)
        {
        if (this->LogLevel || globalLogLevel)
          {
          log->StartEvent("vtkSQVortexFilter::Helicity");
          }
        vtkDataArray *H=V->NewInstance();
        outImData->GetPointData()->AddArray(H);
        H->Delete();
        H->SetNumberOfComponents(1);
        H->SetNumberOfTuples(outputTups);
        std::string name("hel-");
        name+=V->GetName();
        H->SetName(name.c_str());
        //
        switch(V->GetDataType())
          {
          vtkFloatTemplateMacro(
            Helicity<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                dX,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)H->GetVoidPointer(0)));
          default:
            vtkErrorMacro(
              << "Cannot compute helicity on type "
              << V->GetClassName());
          }

        if (this->LogLevel || globalLogLevel)
          {
          log->EndEvent("vtkSQVortexFilter::Helicity");
          }
        }

      // Normalized Helicity.
      if (this->ComputeNormalizedHelicity)
        {
        if (this->LogLevel || globalLogLevel)
          {
          log->StartEvent("vtkSQVortexFilter::NormalizedHelicty");
          }
        vtkDataArray *HN=V->NewInstance();
        outImData->GetPointData()->AddArray(HN);
        HN->Delete();
        HN->SetNumberOfComponents(1);
        HN->SetNumberOfTuples(outputTups);
        std::string name("norm-hel-");
        name+=V->GetName();
        HN->SetName(name.c_str());
        //
        switch(V->GetDataType())
          {
          vtkFloatTemplateMacro(
            NormalizedHelicity<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                dX,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)HN->GetVoidPointer(0)));
          default:
            vtkErrorMacro(
              << "Cannot compute normalized helicity on type "
              << V->GetClassName());
          }
        if (this->LogLevel || globalLogLevel)
          {
          log->EndEvent("vtkSQVortexFilter::NormalizedHelicty");
          }
        }

      // Q Criteria
      if (this->ComputeQ)
        {
        if (this->LogLevel || globalLogLevel)
          {
          log->StartEvent("vtkSQVortexFilter::Q");
          }
        vtkDataArray *Q=V->NewInstance();
        outImData->GetPointData()->AddArray(Q);
        Q->Delete();
        Q->SetNumberOfComponents(1);
        Q->SetNumberOfTuples(outputTups);
        std::string name("q-");
        name+=V->GetName();
        Q->SetName(name.c_str());
        //
        switch(V->GetDataType())
          {
          vtkFloatTemplateMacro(
            QCriteria<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                dX,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)Q->GetVoidPointer(0)));
          default:
            vtkErrorMacro(
              << "Cannot compute Q on type "
              << V->GetClassName());
          }
        if (this->LogLevel || globalLogLevel)
          {
          log->EndEvent("vtkSQVortexFilter::Q");
          }
        }

      // Lambda-1,2,3.
      if (this->ComputeLambda)
        {
        if (this->LogLevel || globalLogLevel)
          {
          log->StartEvent("vtkSQVortexFilter::Lambda123");
          }
        vtkDataArray *L=V->NewInstance();
        outImData->GetPointData()->AddArray(L);
        L->Delete();
        L->SetNumberOfComponents(3);
        L->SetNumberOfTuples(outputTups);
        std::string name("lam-");
        name+=V->GetName();
        L->SetName(name.c_str());
        //
        switch(V->GetDataType())
          {
          vtkFloatTemplateMacro(
            Lambda<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                dX,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)L->GetVoidPointer(0)));
          default:
            vtkErrorMacro(
              << "Cannot compute lambda-1,2,3 on type "
              << V->GetClassName());
          }
        if (this->LogLevel || globalLogLevel)
          {
          log->EndEvent("vtkSQVortexFilter::Lambda123");
          }
        }

      // Lambda-2.
      if (this->ComputeLambda2)
        {
        if (this->LogLevel || globalLogLevel)
          {
          log->StartEvent("vtkSQVortexFilter::Lambda2");
          }
        vtkDataArray *L2=V->NewInstance();
        outImData->GetPointData()->AddArray(L2);
        L2->Delete();
        L2->SetNumberOfComponents(1);
        L2->SetNumberOfTuples(outputTups);
        std::string name("lam2-");
        name+=V->GetName();
        L2->SetName(name.c_str());
        //
        switch(V->GetDataType())
          {
          vtkFloatTemplateMacro(
            Lambda2<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                dX,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)L2->GetVoidPointer(0)));
          default:
            vtkErrorMacro(
              << "Cannot compute lambda-2 on type "
              << V->GetClassName());
          }
        if (this->LogLevel || globalLogLevel)
          {
          log->EndEvent("vtkSQVortexFilter::Lambda2");
          }
        }

      // Divergence.
      if (this->ComputeDivergence)
        {
        if (this->LogLevel || globalLogLevel)
          {
          log->StartEvent("vtkSQVortexFilter::Divergence");
          }
        vtkDataArray *D=V->NewInstance();
        outImData->GetPointData()->AddArray(D);
        D->Delete();
        D->SetNumberOfComponents(1);
        D->SetNumberOfTuples(outputTups);
        std::string name("div-");
        name+=V->GetName();
        D->SetName(name.c_str());
        //
        switch(V->GetDataType())
          {
          vtkFloatTemplateMacro(
            Divergence<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                dX,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)D->GetVoidPointer(0)));
          default:
            vtkErrorMacro(
              << "Cannot compute divergence on type "
              << V->GetClassName());
          }
        if (this->LogLevel || globalLogLevel)
          {
          log->EndEvent("vtkSQVortexFilter::Divergence");
          }
        }

      // Gradient.
      if (this->ComputeGradient)
        {
        if (this->LogLevel || globalLogLevel)
          {
          log->StartEvent("vtkSQVortexFilter::Gradient");
          }
        std::string name;

        vtkDataArray *Gxx=V->NewInstance();
        Gxx->SetNumberOfComponents(1);
        Gxx->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="xx";
        Gxx->SetName(name.c_str());

        vtkDataArray *Gxy=V->NewInstance();
        Gxy->SetNumberOfComponents(1);
        Gxy->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="xy";
        Gxy->SetName(name.c_str());

        vtkDataArray *Gxz=V->NewInstance();
        Gxz->SetNumberOfComponents(1);
        Gxz->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="xz";
        Gxz->SetName(name.c_str());

        vtkDataArray *Gyx=V->NewInstance();
        Gyx->SetNumberOfComponents(1);
        Gyx->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="yx";
        Gyx->SetName(name.c_str());

        vtkDataArray *Gyy=V->NewInstance();
        Gyy->SetNumberOfComponents(1);
        Gyy->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="yy";
        Gyy->SetName(name.c_str());

        vtkDataArray *Gyz=V->NewInstance();
        Gyz->SetNumberOfComponents(1);
        Gyz->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="yz";
        Gyz->SetName(name.c_str());

        vtkDataArray *Gzx=V->NewInstance();
        Gzx->SetNumberOfComponents(1);
        Gzx->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="zx";
        Gzx->SetName(name.c_str());

        vtkDataArray *Gzy=V->NewInstance();
        Gzy->SetNumberOfComponents(1);
        Gzy->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="zy";
        Gzy->SetName(name.c_str());

        vtkDataArray *Gzz=V->NewInstance();
        Gzz->SetNumberOfComponents(1);
        Gzz->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="zz";
        Gzz->SetName(name.c_str());

        switch(V->GetDataType())
          {
          vtkFloatTemplateMacro(
            Gradient<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                dX,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)Gxx->GetVoidPointer(0),
                (VTK_TT*)Gxy->GetVoidPointer(0),
                (VTK_TT*)Gxz->GetVoidPointer(0),
                (VTK_TT*)Gyx->GetVoidPointer(0),
                (VTK_TT*)Gyy->GetVoidPointer(0),
                (VTK_TT*)Gyz->GetVoidPointer(0),
                (VTK_TT*)Gzx->GetVoidPointer(0),
                (VTK_TT*)Gzy->GetVoidPointer(0),
                (VTK_TT*)Gzz->GetVoidPointer(0));
            );
          default:
            vtkErrorMacro(
              << "Cannot compute gradient on type "
              << V->GetClassName());
          }

        if (this->SplitComponents)
          {
          outImData->GetPointData()->AddArray(Gxx);
          outImData->GetPointData()->AddArray(Gxy);
          outImData->GetPointData()->AddArray(Gxz);
          outImData->GetPointData()->AddArray(Gyx);
          outImData->GetPointData()->AddArray(Gyy);
          outImData->GetPointData()->AddArray(Gyz);
          outImData->GetPointData()->AddArray(Gzx);
          outImData->GetPointData()->AddArray(Gzy);
          outImData->GetPointData()->AddArray(Gzz);
          }
        else
          {
          vtkDataArray *G=V->NewInstance();
          outImData->GetPointData()->AddArray(G);
          G->Delete();
          G->SetNumberOfComponents(9);
          G->SetNumberOfTuples(outputTups);
          name="grad-";
          name+=V->GetName();
          G->SetName(name.c_str());

          switch(V->GetDataType())
            {
            vtkFloatTemplateMacro(
              Interleave<VTK_TT>(
                  outputTups,
                  (VTK_TT*)Gxx->GetVoidPointer(0),
                  (VTK_TT*)Gxy->GetVoidPointer(0),
                  (VTK_TT*)Gxz->GetVoidPointer(0),
                  (VTK_TT*)Gyx->GetVoidPointer(0),
                  (VTK_TT*)Gyy->GetVoidPointer(0),
                  (VTK_TT*)Gyz->GetVoidPointer(0),
                  (VTK_TT*)Gzx->GetVoidPointer(0),
                  (VTK_TT*)Gzy->GetVoidPointer(0),
                  (VTK_TT*)Gzz->GetVoidPointer(0),
                  (VTK_TT*)G->GetVoidPointer(0)));
            }
          }
        Gxx->Delete();
        Gxy->Delete();
        Gxz->Delete();
        Gyx->Delete();
        Gyy->Delete();
        Gyz->Delete();
        Gzx->Delete();
        Gzy->Delete();
        Gzz->Delete();
        if (this->LogLevel || globalLogLevel)
          {
          log->EndEvent("vtkSQVortexFilter::Gradient");
          }
        }

      // EigenvalueDiagnostic.
      if (this->ComputeEigenvalueDiagnostic)
        {
        if (this->LogLevel || globalLogLevel)
          {
          log->StartEvent("vtkSQVortexFilter::EigenvalueDiagnostic");
          }
        vtkDataArray *D=V->NewInstance();
        outImData->GetPointData()->AddArray(D);
        D->Delete();
        D->SetNumberOfComponents(1);
        D->SetNumberOfTuples(outputTups);
        std::string name("eigen-diag-");
        name+=V->GetName();
        D->SetName(name.c_str());
        //
        switch(V->GetDataType())
          {
          vtkFloatTemplateMacro(
            EigenvalueDiagnostic<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                dX,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)D->GetVoidPointer(0)));
          default:
            vtkErrorMacro(
              << "Cannot compute eigenvalue diagnostic on type "
              << V->GetClassName());
          }
        if (this->LogLevel || globalLogLevel)
          {
          log->EndEvent("vtkSQVortexFilter::EigenvalueDiagnostic");
          }
        }

      // Gardient-tensor diagnostic.
      if (this->ComputeGradientDiagnostic)
        {
        if (this->LogLevel || globalLogLevel)
          {
          log->StartEvent("vtkSQVortexFilter::GradientDiagnostic");
          }
        std::string name;

        vtkDataArray *Gxx=V->NewInstance();
        Gxx->SetNumberOfComponents(1);
        Gxx->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="xx";
        Gxx->SetName(name.c_str());

        vtkDataArray *Gxy=V->NewInstance();
        Gxy->SetNumberOfComponents(1);
        Gxy->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="xy";
        Gxy->SetName(name.c_str());

        vtkDataArray *Gxz=V->NewInstance();
        Gxz->SetNumberOfComponents(1);
        Gxz->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="xz";
        Gxz->SetName(name.c_str());

        vtkDataArray *Gyx=V->NewInstance();
        Gyx->SetNumberOfComponents(1);
        Gyx->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="yx";
        Gyx->SetName(name.c_str());

        vtkDataArray *Gyy=V->NewInstance();
        Gyy->SetNumberOfComponents(1);
        Gyy->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="yy";
        Gyy->SetName(name.c_str());

        vtkDataArray *Gyz=V->NewInstance();
        Gyz->SetNumberOfComponents(1);
        Gyz->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="yz";
        Gyz->SetName(name.c_str());

        vtkDataArray *Gzx=V->NewInstance();
        Gzx->SetNumberOfComponents(1);
        Gzx->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="zx";
        Gzx->SetName(name.c_str());

        vtkDataArray *Gzy=V->NewInstance();
        Gzy->SetNumberOfComponents(1);
        Gzy->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="zy";
        Gzy->SetName(name.c_str());

        vtkDataArray *Gzz=V->NewInstance();
        Gzz->SetNumberOfComponents(1);
        Gzz->SetNumberOfTuples(outputTups);
        name="grad-";
        name+=V->GetName();
        name+="zz";
        Gzz->SetName(name.c_str());

        vtkDataArray *W=V->NewInstance();
        W->SetNumberOfComponents(3);
        W->SetNumberOfTuples(outputTups);

        vtkDataArray *S=V->NewInstance();
        S->SetNumberOfTuples(outputTups);
        name="mag(";
        name+=V->GetName();
        name=".grad(";
        name+=V->GetName();
        name+="))/mag(";
        name+=V->GetName();
        name+=")";
        S->SetName(name.c_str());
        outImData->GetPointData()->AddArray(S);
        S->Delete();

        switch(V->GetDataType())
          {
          vtkFloatTemplateMacro(

            Gradient<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                dX,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)Gxx->GetVoidPointer(0),
                (VTK_TT*)Gxy->GetVoidPointer(0),
                (VTK_TT*)Gxz->GetVoidPointer(0),
                (VTK_TT*)Gyx->GetVoidPointer(0),
                (VTK_TT*)Gyy->GetVoidPointer(0),
                (VTK_TT*)Gyz->GetVoidPointer(0),
                (VTK_TT*)Gzx->GetVoidPointer(0),
                (VTK_TT*)Gzy->GetVoidPointer(0),
                (VTK_TT*)Gzz->GetVoidPointer(0));

            VectorMatrixMul<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)Gxx->GetVoidPointer(0),
                (VTK_TT*)Gxy->GetVoidPointer(0),
                (VTK_TT*)Gxz->GetVoidPointer(0),
                (VTK_TT*)Gyx->GetVoidPointer(0),
                (VTK_TT*)Gyy->GetVoidPointer(0),
                (VTK_TT*)Gyz->GetVoidPointer(0),
                (VTK_TT*)Gzx->GetVoidPointer(0),
                (VTK_TT*)Gzy->GetVoidPointer(0),
                (VTK_TT*)Gzz->GetVoidPointer(0),
                (VTK_TT*)W->GetVoidPointer(0));

            Magnitude<VTK_TT>(
                outputTups,
                (VTK_TT*)W->GetVoidPointer(0),
                (VTK_TT*)S->GetVoidPointer(0));

            Normalize<VTK_TT>(
                inputExt.GetData(),
                outputExt.GetData(),
                this->Mode,
                (VTK_TT*)V->GetVoidPointer(0),
                (VTK_TT*)S->GetVoidPointer(0));
            );
          default:
            vtkErrorMacro(
              << "Cannot compute gradient diagnostic on type "
              << V->GetClassName());
          }
        W->Delete();
        Gxx->Delete();
        Gxy->Delete();
        Gxz->Delete();
        Gyx->Delete();
        Gyy->Delete();
        Gyz->Delete();
        Gzx->Delete();
        Gzy->Delete();
        Gzz->Delete();

        if (this->LogLevel || globalLogLevel)
          {
          log->EndEvent("vtkSQVortexFilter::GradientDiagnostic");
          }
        }
      }

    if (this->ResultMagnitude)
      {
      if (this->LogLevel || globalLogLevel)
        {
        log->StartEvent("vtkSQVortexFilter::ResultMagnitude");
        }
      int nOutArrays=outImData->GetPointData()->GetNumberOfArrays();
      for (int i=0; i<nOutArrays; ++i)
        {
        vtkDataArray *da=outImData->GetPointData()->GetArray(i);
        size_t daNc=da->GetNumberOfComponents();
        if (daNc==1)
          {
          continue;
          }
        vtkDataArray *mda=da->NewInstance();
        size_t daNt=da->GetNumberOfTuples();
        mda->SetNumberOfTuples(daNt);
        std::string name="mag-";
        name+=da->GetName();
        mda->SetName(name.c_str());
        outImData->GetPointData()->AddArray(mda);
        mda->Delete();
        switch(da->GetDataType())
          {
          vtkFloatTemplateMacro(
            Magnitude<VTK_TT>(
                daNt,
                daNc,
                (VTK_TT*)da->GetVoidPointer(0),
                (VTK_TT*)mda->GetVoidPointer(0)));
        default:
          vtkErrorMacro(
            << "Cannot compute magnitude on type "
            << da->GetClassName());
          }
        }
      if (this->LogLevel || globalLogLevel)
        {
        log->EndEvent("vtkSQVortexFilter::ResultMagnitude");
        }
      }
    // outImData->Print(std::cerr);
    }
  else
  if (isRecti)
    {
    vtkWarningMacro("TODO : implment difference opperators on stretched grids.");
    }

  if (this->LogLevel || globalLogLevel)
    {
    log->EndEvent("vtkSQVortexFilter::RequestData");
    }

 return 1;
}

//-----------------------------------------------------------------------------
void vtkSQVortexFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQVortexFilter::PrintSelf" << std::endl;
  #endif

  this->Superclass::PrintSelf(os,indent);

  // TODO

}
