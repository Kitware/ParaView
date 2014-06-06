/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQBOVReader.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkPVXMLElement.h"
#include "vtkPVInformationKeys.h"

#include "vtkSQLog.h"
#include "vtkSQOOCReader.h"
#include "vtkSQOOCBOVReader.h"
#include "BOVReader.h"
#include "GDAMetaData.h"
#include "BOVTimeStepImage.h"
#include "ImageDecomp.h"
#include "RectilinearDecomp.h"
#include "Numerics.hxx"
#include "Tuple.hxx"
#include "XMLUtils.h"
#include "PrintUtils.h"
#include "SQMacros.h"
#include "postream.h"

#ifndef SQTK_WITHOUT_MPI
#include "SQMPICHWarningSupression.h"
#include <mpi.h>
#endif

#include <algorithm>
#include <sstream>

// #define SQTK_DEBUG

#ifdef WIN32
  #undef SQTK_DEBUG
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQBOVReader);

//-----------------------------------------------------------------------------
vtkSQBOVReader::vtkSQBOVReader()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReader::vtkSQBOVReader" << std::endl;
  #endif

  // Initialize pipeline.
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQBOVReader::~vtkSQBOVReader()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReader::~vtkSQBOVReader" << std::endl;
  #endif

  this->Clear();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::Clear()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReader::Clear" << std::endl;
  #endif

  vtkSQBOVReaderBase::Clear();
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::Initialize(
      vtkPVXMLElement *root,
      const char *fileName,
      std::vector<std::string> &arrays)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReader::Initialize" << std::endl;
  #endif

  vtkPVXMLElement *elem=GetOptionalElement(root,"vtkSQBOVReader");
  if (elem==0)
    {
    return -1;
    }

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->GetHeader() << "# ::vtkSQBOVReader" << "\n";
    }

  return vtkSQBOVReaderBase::Initialize(root,fileName,arrays);
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::RequestInformation(
  vtkInformation *req,
  vtkInformationVector **inInfos,
  vtkInformationVector* outInfos)
{
  #if defined SQTK_DEBUG
  std::ostringstream oss;
  oss << "=====vtkSQBOVReader::RequestInformation" << std::endl;
  #endif

  if (!this->Reader->IsOpen())
    {
    vtkWarningMacro("No file open, cannot process RequestInformation!");
    return 1;
    }

  vtkInformation *info=outInfos->GetInformationObject(0);

  // The actual read we need to populate the VTK keys with actual
  // values. The mechanics of the pipeline require that the data set
  // dimensions and whole extent key reflect the global index space
  // of the dataset, the data set extent will have the decomposed
  // index space.
  int wholeExtent[6];
  this->GetSubset(wholeExtent);
  info->Set(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    wholeExtent,
    6);
  // req->Append(
  //     vtkExecutive::KEYS_TO_COPY(),
  //     vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  info->Set(CAN_PRODUCE_SUB_EXTENT(), 1);

  #if defined SQTK_DEBUG
  oss << "WHOLE_EXTENT=" << Tuple<int>(wholeExtent,6) << std::endl;
  #endif

  if (this->Reader->DataSetTypeIsImage())
    {
    double X0[3];
    this->Reader->GetMetaData()->GetOrigin(X0);
    info->Set(vtkDataObject::ORIGIN(),X0,3);
    // req->Append(vtkExecutive::KEYS_TO_COPY(),vtkDataObject::ORIGIN());

    double dX[3];
    this->Reader->GetMetaData()->GetSpacing(dX);
    info->Set(vtkDataObject::SPACING(),dX,3);
    // req->Append(vtkExecutive::KEYS_TO_COPY(),vtkDataObject::SPACING());

    #if defined SQTK_DEBUG
    oss
      << "ORIGIN=" << Tuple<double>(X0,3) << std::endl
      << "SPACING=" << Tuple<double>(dX,3) << std::endl;
    #endif
    }



  #if defined SQTK_DEBUG
  pCerr() << oss.str() << std::endl;
  #endif

  return vtkSQBOVReaderBase::RequestInformation(req,inInfos,outInfos);
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::RequestData(
        vtkInformation *req,
        vtkInformationVector ** /*input*/,
        vtkInformationVector *outInfos)
{
  #if defined SQTK_DEBUG
  std::ostringstream oss;
  oss << "=====vtkSQBOVReader::RequestData" << std::endl;
  #endif

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->StartEvent("vtkSQBOVReader::RequestData");
    }

  vtkInformation *info=outInfos->GetInformationObject(0);

  // Get the output dataset.
  vtkDataSet *output
    = dynamic_cast<vtkDataSet *>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (output==NULL)
    {
    vtkErrorMacro("Filter data has not been configured correctly.");
    return 1;
    }

  // get the step id of requested time.
  int stepId=this->GetTimeStepId(info,output->GetInformation());

  // The subset is the what the user selected in the GUI. This is what will
  // be loaded in aggregate across the entire run.
  BOVMetaData *md=this->Reader->GetMetaData();
  CartesianExtent subset=md->GetSubset();

  #if defined SQTK_DEBUG
  oss
    << "WHOLE_EXTENT=" << subset
    << " size=" << subset.Size()*sizeof(float)
    << std::endl;
  #endif

  // shift to the dual grid
  subset.NodeToCell();

  // this is a hack to accomodate 2D grids.
  for (int q=0; q<3; ++q)
    {
    int qq=2*q;
    if (subset[qq+1]<subset[qq])
      {
      subset[qq+1]=subset[qq];
      }
    }

  // ParaView sends the update extent to inform us of the domain decomposition.
  // The decomp is what will be loaded by this process.
  CartesianExtent decomp;
  //int decomp[6];
  info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),decomp.GetData());

  // Set the region to be read.
  md->SetDecomp(decomp);

  #if defined SQTK_DEBUG
  oss << "UPDATE_EXTENT=" << decomp << std::endl;
  #endif

  // Construct MPI File hints for the reader.
  this->SetMPIFileHints();

  // A read pulls data from file on a cell centered grid into memory
  // on a node centered grid. file->nCells==memory->nPoints

  // Set the timestep to be read.
  BOVTimeStepImage *stepImg=this->Reader->OpenTimeStep(stepId);

  if (this->Reader->DataSetTypeIsImage())
    {
    // read onto a uniform grid

    // Pull origin and spacing we stored during RequestInformation pass.
    double dX[3];
    info->Get(vtkDataObject::SPACING(),dX);
    double X0[3];
    info->Get(vtkDataObject::ORIGIN(),X0);

    int nPoints[3];
    subset.Size(nPoints);

    // Configure the output.
    vtkImageData *idds=dynamic_cast<vtkImageData*>(output);
    idds->SetDimensions(nPoints);
    idds->SetOrigin(X0);
    idds->SetSpacing(dX);
    idds->SetExtent(decomp.GetData());

    // Store the bounds of the aggregate dataset.
    double subsetBounds[6];
    CartesianExtent::GetBounds(
          subset,
          X0,
          dX,
          CartesianExtent::DIM_MODE_3D,
          subsetBounds);

    info->Set(
        vtkPVInformationKeys::WHOLE_BOUNDING_BOX(),
        subsetBounds,
        6);
    req->Append(
        vtkExecutive::KEYS_TO_COPY(),
        vtkPVInformationKeys::WHOLE_BOUNDING_BOX());
    }
  else
  if (this->Reader->DataSetTypeIsRectilinear())
    {
    // read onto the stretched grid
    int nPoints[3];
    decomp.Size(nPoints);

    // configure the output
    vtkRectilinearGrid *rgds=dynamic_cast<vtkRectilinearGrid*>(output);
    rgds->SetExtent(decomp.GetData());

    vtkFloatArray *fa;
    fa=vtkFloatArray::New();
    fa->SetArray(md->SubsetCoordinate(0,decomp),nPoints[0],0);
    rgds->SetXCoordinates(fa);
    fa->Delete();

    fa=vtkFloatArray::New();
    fa->SetArray(md->SubsetCoordinate(1,decomp),nPoints[1],0);
    rgds->SetYCoordinates(fa);
    fa->Delete();

    fa=vtkFloatArray::New();
    fa->SetArray(md->SubsetCoordinate(2,decomp),nPoints[2],0);
    rgds->SetZCoordinates(fa);
    fa->Delete();

    // Store the bounds of the aggregate dataset.
    double subsetBounds[6];
    CartesianExtent::GetBounds(
        subset,
        md->GetCoordinate(0)->GetPointer(),
        md->GetCoordinate(1)->GetPointer(),
        md->GetCoordinate(2)->GetPointer(),
        CartesianExtent::DIM_MODE_3D,
        subsetBounds);

    info->Set(
        vtkPVInformationKeys::WHOLE_BOUNDING_BOX(),
        subsetBounds,
        6);
    req->Append(
        vtkExecutive::KEYS_TO_COPY(),
        vtkPVInformationKeys::WHOLE_BOUNDING_BOX());
    }
  else
  if (this->Reader->DataSetTypeIsStructured())
    {
    /// Structured data
    vtkErrorMacro("vtkStructuredData is not implemented yet.");
    return 1;
    }
  else
    {
    // unrecognized dataset type
    vtkErrorMacro(
      << "Error: invalid dataset type \""
      << md->GetDataSetType() << "\".");
    }

  // Read the selected arrays into the output.
  int ok=this->Reader->ReadTimeStep(stepImg,output,this);
  this->Reader->CloseTimeStep(stepImg);
  if (!ok)
    {
    vtkErrorMacro(
      << "Read failed." << endl << *md);
    output->Initialize();
    return 1;
    }

  // Give implementation classes a chance to store specialized keys
  // into the pipeline.
  md->PushPipelineInformation(req, info);

  #if defined SQTK_DEBUG
  pCerr() << oss.str() << std::endl;
  //this->Reader->PrintSelf(pCerr());
  //output->Print(pCerr());
  #endif

  if (this->LogLevel || globalLogLevel)
    {
    log->EndEvent("vtkSQBOVReader::RequestData");
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName:        " << safeio(this->FileName) << std::endl;
  os << indent << "FileNameChanged: " << this->FileNameChanged << std::endl;
  os << indent << "Raeder: " << std::endl;
  this->Reader->PrintSelf(os);
  os << std::endl;
}
