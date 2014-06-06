/* ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSQFieldTracer.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQFieldTracer.h"

#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkIntArray.h"
#include "vtkCellArray.h"
#include "vtkIdList.h"

#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include "vtkInterpolatedVelocityField.h"
#include "vtkInitialValueProblemSolver.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkMultiProcessController.h"
#include "vtkMath.h"

#include "vtkPVInformationKeys.h"

#include "vtkSQLog.h"
#include "vtkSQOOCReader.h"
#include "vtkSQCellGenerator.h"
#include "vtkSQMetaDataKeys.h"
#include "vtkSQLog.h"
#include "FieldLine.h"
#include "TerminationCondition.h"
#include "IdBlock.h"
#include "WorkQueue.h"
#include "FieldTraceData.h"
#include "PolyDataFieldTopologyMap.h"
#include "UnstructuredFieldTopologyMap.h"
#include "PolyDataFieldDisplacementMap.h"
#include "UnstructuredFieldDisplacementMap.h"
#include "StreamlineData.h"
#include "PoincareMapData.h"
#include "XMLUtils.h"
#include "Tuple.hxx"
#include "postream.h"

#ifndef SQTK_WITHOUT_MPI
#include "SQMPICHWarningSupression.h"
#include <mpi.h>
#endif

#include <algorithm>

#ifdef SQTK_DEBUG
#define vtkSQFieldTracerDEBUG 2
#else
#define vtkSQFieldTracerDEBUG -1
#endif

// TODO
// logging currently chews through a tremendous amount of ram
// on the master rank, probably due to log events placed in
// integrate dynamic.
// #define vtkSQFieldTracerTIME

#ifndef vtkSQFieldTracerDEBUG
  // 0 -- no output
  // 1 -- adds integration events
  // 2 -- adds pipeline trace
  // 3 -- adds all integration info
  #define vtkSQFieldTracerDEBUG 0
#endif

const double vtkSQFieldTracer::EPSILON = 1.0E-12;

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQFieldTracer);

//-----------------------------------------------------------------------------
vtkSQFieldTracer::vtkSQFieldTracer()
      :
  WorldSize(1),
  WorldRank(0),
  UseDynamicScheduler(0),
  WorkerBlockSize(16),
  MasterBlockSize(256),
  ForwardOnly(0),
  StepUnit(ARC_LENGTH),
  MinStep(1.0E-8),
  MaxStep(1.0),
  MaxError(1E-4),
  MaxNumberOfSteps(VTK_INT_MAX),
  MaxLineLength(VTK_DOUBLE_MAX),
  MaxIntegrationInterval(VTK_DOUBLE_MAX),
  NullThreshold(1E-3),
  IntegratorType(INTEGRATOR_NONE),
  Integrator(0),
  MinSegmentLength(0.0),
  UseCommWorld(0),
  Mode(MODE_STREAM),
  CullPeriodicTransitions(1),
  SqueezeColorMap(0),
  LogLevel(0)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::vtkSQFieldTracer" << std::endl;
  #endif

  #ifdef SQTK_WITHOUT_MPI
  vtkErrorMacro(
      << "This class requires MPI however it was built without MPI.");
  #else
  MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
  MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);

  int ok;
  MPI_Initialized(&ok);
  if (!ok)
    {
    vtkErrorMacro(
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    }
  #endif

  this->TermCon=new TerminationCondition;

  this->SetNumberOfInputPorts(3);
  this->SetNumberOfOutputPorts(1);
}

//-----------------------------------------------------------------------------
vtkSQFieldTracer::~vtkSQFieldTracer()
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::~vtkSQFieldTracer" << std::endl;
  #endif
  if (this->Integrator)
    {
    this->Integrator->Delete();
    }
  delete this->TermCon;
}

//-----------------------------------------------------------------------------
int vtkSQFieldTracer::Initialize(vtkPVXMLElement *root)
{
  vtkPVXMLElement *elem=0;
  elem=GetOptionalElement(root,"vtkSQFieldTracer");
  if (elem==0)
    {
    return -1;
    }

  int mode=MODE_STREAM;
  GetOptionalAttribute<int,1>(elem,"mode",&mode);
  this->SetMode(mode);

  int integratorType=INTEGRATOR_RK4;
  GetOptionalAttribute<int,1>(elem,"integrator_type",&integratorType);
  this->SetIntegratorType(integratorType);

  double maxArcLength=0.0;
  GetOptionalAttribute<double,1>(elem,"max_arc_length",&maxArcLength);
  if (maxArcLength>0.0)
    {
    this->SetMaxLineLength(maxArcLength);
    }

  double maxIntegrationInterval=0.0;
  GetOptionalAttribute<double,1>(elem,"max_integration_interval",&maxIntegrationInterval);
  if (maxIntegrationInterval>0.0)
    {
    this->SetMaxIntegrationInterval(maxIntegrationInterval);
    }

  double minSegmentLength=0.0;
  GetOptionalAttribute<double,1>(elem,"min_segment_length",&minSegmentLength);
  if (minSegmentLength>0.0)
    {
    this->SetMinSegmentLength(minSegmentLength);
    }

  int maxNumberSteps=0;
  GetOptionalAttribute<int,1>(elem,"max_number_steps",&maxNumberSteps);
  if (maxNumberSteps>0)
    {
    this->SetMaxNumberOfSteps(maxNumberSteps);
    }

  double minStepSize=0.0;
  GetOptionalAttribute<double,1>(elem,"min_step_size",&minStepSize);
  if (minStepSize>0.0)
    {
    this->SetMinStep(minStepSize);
    }

  double maxStepSize=0.0;
  GetOptionalAttribute<double,1>(elem,"max_step_size",&maxStepSize);
  if (maxStepSize>0.0)
    {
    this->SetMaxStep(maxStepSize);
    }

  double maxError=0.0;
  GetOptionalAttribute<double,1>(elem,"max_error",&maxError);
  if (maxError>0.0)
    {
    this->SetMaxError(maxError);
    }

  double nullThreshold=0.0;
  GetOptionalAttribute<double,1>(elem,"null_threshold",&nullThreshold);
  if (nullThreshold>0.0)
    {
    this->SetNullThreshold(nullThreshold);
    }

  int forwardOnly=-1;
  GetOptionalAttribute<int,1>(elem,"forward_only",&forwardOnly);
  if (forwardOnly>=0)
    {
    this->SetForwardOnly(forwardOnly);
    }

  int dynamicScheduler=-1;
  GetOptionalAttribute<int,1>(elem,"dynamic_scheduler",&dynamicScheduler);
  if (dynamicScheduler>=0)
    {
    this->SetUseDynamicScheduler(dynamicScheduler);
    }

  int masterBlockSize=-1;
  GetOptionalAttribute<int,1>(elem,"master_block_size",&masterBlockSize);
  if (masterBlockSize>=0)
    {
    this->SetMasterBlockSize(masterBlockSize);
    }

  int workerBlockSize=-1;
  GetOptionalAttribute<int,1>(elem,"worker_block_size",&workerBlockSize);
  if (workerBlockSize>=0)
    {
    this->SetWorkerBlockSize(workerBlockSize);
    }

  int squeezeColorMap=-1;
  GetOptionalAttribute<int,1>(elem,"squeeze_color_map",&squeezeColorMap);
  if (squeezeColorMap>=0)
    {
    this->SetSqueezeColorMap(squeezeColorMap);
    }

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->GetHeader()
      << "# ::vtkSQFieldTracer" << "\n"
      << "#   mode=" << this->GetMode() << "\n"
      << "#   integrator=" << this->GetIntegratorType() << "\n"
      << "#   minStepSize=" << this->GetMinStep() << "\n"
      << "#   maxStepSize=" << this->GetMaxStep() << "\n"
      << "#   maxNumberOfSteps=" << this->GetMaxNumberOfSteps() << "\n"
      << "#   maxLineLength=" << this->GetMaxLineLength() << "\n"
      << "#   maxIntegrationInterval=" << this->GetMaxIntegrationInterval() << "\n"
      << "#   minSegmentLength=" << this->GetMinSegmentLength() << "\n"
      << "#   maxError=" << this->GetMaxError() << "\n"
      << "#   nullThreshold=" << this->GetNullThreshold() << "\n"
      << "#   forwardOnly=" << this->GetForwardOnly() << "\n"
      << "#   dynamicScheduler=" << this->GetUseDynamicScheduler() << "\n"
      << "#   masterBlockSize=" << this->GetMasterBlockSize() << "\n"
      << "#   workerBlockSize=" << this->GetWorkerBlockSize() << "\n"
      << "#   squeezeColorMap=" << this->GetSqueezeColorMap() << "\n";
    }

  return 0;
}

//-----------------------------------------------------------------------------
int vtkSQFieldTracer::FillInputPortInformation(int port, vtkInformation *info)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::FillInputPortInformation" << std::endl;
  #endif
  switch (port)
    {
    // Vector feild data
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
      break;
    // Seed points
    case 1:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
      break;
    // termination Surface
    case 2:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
      info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(),1);
      info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(),1);
      break;
  default:
      vtkWarningMacro("Invalid input port " << port << " requested.");
      break;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQFieldTracer::FillOutputPortInformation(
      int /*port*/,
      vtkInformation *info)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::FillOutputPortInformation" << std::endl;
  #endif

  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  return 1;

  // switch (port)
  //   {
  //
  //   case 0:
  //     switch (this->Mode)
  //      {
  //
  //       // set the output data type based on the mode the filter
  //       // is run in. See SetMode in header documentation.
  //       case (MODE_TOPOLOGY):
  //         info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataSet");
  //         break;
  //
  //       case (MODE_STREAM):
  //       case (MODE_POINCARE):
  //         info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
  //         break;
  //
  //       default:
  //         vtkErrorMacro("Invalid mode " << this->Mode << ".");
  //         break;
  //       }
  //     break;
  //
  //   default:
  //     vtkWarningMacro("Invalid output port requested.");
  //     break;
  //   }
  // return 1;
}

//----------------------------------------------------------------------------
void vtkSQFieldTracer::AddVectorInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::AddDatasetInputConnectiont" << std::endl;
  #endif

  this->AddInputConnection(0, algOutput);
}

//----------------------------------------------------------------------------
void vtkSQFieldTracer::ClearVectorInputConnections()
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::ClearDatasetInputConnections" << std::endl;
  #endif

  this->SetInputConnection(0, 0);
}

//----------------------------------------------------------------------------
void vtkSQFieldTracer::AddSeedPointInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::AddSeedPointInputConnection" << std::endl;
  #endif
  this->AddInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void vtkSQFieldTracer::ClearSeedPointInputConnections()
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::ClearSeedPointInputConnections" << std::endl;
  #endif
  this->SetInputConnection(1, 0);
}

//----------------------------------------------------------------------------
void vtkSQFieldTracer::AddTerminatorInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::AddBoundaryInputConnection" << std::endl;
  #endif
  this->AddInputConnection(2, algOutput);
}

//----------------------------------------------------------------------------
void vtkSQFieldTracer::ClearTerminatorInputConnections()
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::ClearBoundaryInputConnections" << std::endl;
  #endif
  this->SetInputConnection(2, 0);
}

//-----------------------------------------------------------------------------
void vtkSQFieldTracer::SetStepUnit(int unit)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::SetStepUnit" << std::endl;
  #endif
  if (unit==this->StepUnit )
    {
    return;
    }
  if (unit!=ARC_LENGTH)
    {
    // Rev 117 removed support for cell fraction.
    vtkWarningMacro("Unsupported step unit. Using arc length units.");
    unit=ARC_LENGTH;
    }
  this->StepUnit = unit;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQFieldTracer::SetIntegratorType(int type)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::SetIntegratorType" << std::endl;
  #endif

  if (this->IntegratorType==type)
    {
    return;
    }

  if (this->Integrator)
    {
    this->Integrator->Delete();
    this->Integrator=0;
    }
  this->IntegratorType=INTEGRATOR_NONE;
  this->Modified();

  switch (type)
    {
    case INTEGRATOR_RK2:
      this->Integrator=vtkRungeKutta2::New();
      break;

    case INTEGRATOR_RK4:
      this->Integrator=vtkRungeKutta4::New();
      break;

    case INTEGRATOR_RK45:
      this->Integrator=vtkRungeKutta45::New();
      break;

    default:
      vtkErrorMacro("Unsupported integrator type " << type << ".");
      return;
      // break;
    }

  this->IntegratorType=type;
}

//----------------------------------------------------------------------------
int vtkSQFieldTracer::RequestDataObject(
                vtkInformation *,
                vtkInformationVector** inInfos,
                vtkInformationVector* outInfos)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::RequestDataObject" << std::endl;
  #endif
  // get the filters output
  vtkInformation* outInfo = outInfos->GetInformationObject(0);
  vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());

  switch (this->Mode)
    {
    case MODE_DISPLACEMENT:
    case MODE_TOPOLOGY:
      {
      // output type is polydata or unstructured grid depending on the
      // data type of the second input. duplicate the input type for the
      // map output.
      vtkInformation* inInfo=inInfos[1]->GetInformationObject(0);
      vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());
      if (!outData || !outData->IsA(inData->GetClassName()))
        {
        outData=inData->NewInstance();
        outInfo->Set(vtkDataObject::DATA_OBJECT(),outData);
        outData->Delete();
        vtkInformation *portInfo=this->GetOutputPortInformation(0);
        portInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(),VTK_PIECES_EXTENT);
        }
      }
      break;

    case MODE_STREAM:
    case MODE_POINCARE:
      if (!outData)
        {
        // consrtruct a polydata for the field line output. output type
        // is always polydata.
        outData=vtkPolyData::New();
        outInfo->Set(vtkDataObject::DATA_OBJECT(),outData);
        outData->Delete();
        vtkInformation *portInfo=this->GetOutputPortInformation(0);
        portInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(),VTK_PIECES_EXTENT);
        }
      break;

    default:
      vtkErrorMacro("Invalid mode " << this->Mode << ".");
      break;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSQFieldTracer::RequestUpdateExtent(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **inInfos,
                vtkInformationVector *outInfos)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::RequestUpdateExtent" << std::endl;
  #endif

  vtkInformation *outInfo = outInfos->GetInformationObject(0);
  int ghostLevel =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  // This has the effect to run the source on all procs, without it
  // only process 0 gets the source data.
  int piece=0;
  int numPieces=1;

  // The dynamic scheduler requires all processes have all of the seeds,
  // while the static scheduler expects each process has a unique sub set
  // of the seeeds.
  if (!this->UseDynamicScheduler)
    {
    piece=outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    numPieces=outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    }

  // Seed point input.
  int nSources=inInfos[1]->GetNumberOfInformationObjects();
  for (int i=0; i<nSources; ++i)
    {
    vtkInformation *sourceInfo = inInfos[1]->GetInformationObject(i);
    if (sourceInfo)
      {
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),piece);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),numPieces);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),ghostLevel);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(),1);
      }
    }

  // Terminator surface input. Always request all data on all procs.
  nSources=inInfos[2]->GetNumberOfInformationObjects();
  for (int i=0; i<nSources; ++i)
    {
    vtkInformation *sourceInfo = inInfos[2]->GetInformationObject(i);
    if (sourceInfo)
      {
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),0);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),1);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),ghostLevel);
      sourceInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(),1);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSQFieldTracer::RequestInformation(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **vtkNotUsed(inputVector),
                vtkInformationVector *outputVector)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::RequestInformation" << std::endl;
  #endif

  return 1;
}

//----------------------------------------------------------------------------
int vtkSQFieldTracer::RequestData(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **inputVector,
                vtkInformationVector *outputVector)
{
  #if vtkSQFieldTracerDEBUG>1
  pCerr() << "=====vtkSQFieldTracer::RequestData" << std::endl;
  #endif

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->StartEvent("vtkSQFieldTracer::RequestData");
    }

  #ifdef SQTK_WITHOUT_MPI
  (void)inputVector;
  (void)outputVector;
  #else
  vtkInformation *info;

  /// Reader
  // Get the input on the first port. This should be the dummy dataset
  // produced by a meta-reader. The information object should have the
  // OOC Reader.
  info=inputVector[0]->GetInformationObject(0);

  if (!info->Has(vtkSQOOCReader::READER()))
    {
    vtkErrorMacro(
        "OOCReader object not present in input pipeline information.");
    return 1;
    }
  vtkSmartPointer<vtkSQOOCReader> oocr;
  oocr=dynamic_cast<vtkSQOOCReader*>(info->Get(vtkSQOOCReader::READER()));
  if (!oocr)
    {
    vtkErrorMacro(
        "OOCReader object not initialized.");
    return 1;
    }
  // Configure the reader
  vtkInformation *arrayInfo=this->GetInputArrayInformation(0);
  const char *fieldName=arrayInfo->Get(vtkDataObject::FIELD_NAME());
  oocr->Register(0);
  oocr->DeActivateAllArrays();
  oocr->ActivateArray(fieldName);
  oocr->SetCommunicator(this->UseCommWorld?MPI_COMM_WORLD:MPI_COMM_SELF);
  oocr->Open();
  // We provide the integrator a pointer to a cache. The integrator
  // uses this as it sees fit to reduce the number of reads. If its
  // not null after the integrator returns we have to delete.
  vtkDataSet *oocrCache=0;

  // the bounds (problem domain) of the data should be provided by the
  // meta reader.
  if (!info->Has(vtkPVInformationKeys::WHOLE_BOUNDING_BOX()))
    {
    vtkErrorMacro("Bounds are not present in the pipeline information.");
    return 1;
    }
  double pDomain[6];
  info->Get(vtkPVInformationKeys::WHOLE_BOUNDING_BOX(),pDomain);

  // the bounadry condition flags should be provided by the reader
  if (!info->Has(vtkSQOOCReader::PERIODIC_BC()))
    {
    vtkErrorMacro(
        "Boundary condition flags not found in pipeline information.");
    return 1;
    }
  int periodicBC[3];
  info->Get(vtkSQOOCReader::PERIODIC_BC(),periodicBC);

  /// Seed source
  // TODO for now we consider a seed single source, but we should handle
  // multiple sources.
  info=inputVector[1]->GetInformationObject(0);

  vtkSmartPointer<vtkSQCellGenerator> sourceGen=0;
  if ( this->UseDynamicScheduler && !(this->Mode==MODE_STREAM) )
    {
    sourceGen
    =dynamic_cast<vtkSQCellGenerator*>(info->Get(vtkSQCellGenerator::CELL_GENERATOR()));
    }

  vtkDataSet *source
    = dynamic_cast<vtkDataSet*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (source==0)
    {
    vtkErrorMacro("Seed source input was not present.");
    return 1;
    }

  /// Output
  // Get the filter's output.
  info=outputVector->GetInformationObject(0);
  vtkDataSet *out
    = dynamic_cast<vtkDataSet*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (out==0)
    {
    vtkErrorMacro("Output dataset was not present.");
    return 1;
    }

  /// Map
  // Configure the map.
  FieldTraceData *traceData=0;

  // There are multiple modes the filter can be used in, see documentation
  // for See SetMode.
  switch (this->Mode)
    {
    case MODE_DISPLACEMENT:
    case MODE_TOPOLOGY:
      {
      vtkPolyData *sourcePd, *outPd;
      vtkUnstructuredGrid *sourceUg, *outUg;

      if ((sourcePd=dynamic_cast<vtkPolyData*>(source))
        && (outPd=dynamic_cast<vtkPolyData*>(out)))
        {
        if (this->Mode==MODE_TOPOLOGY)
          {
          traceData=new PolyDataFieldTopologyMap;
          }
        else
          {
          traceData=new PolyDataFieldDisplacementMap;
          }
        }
      else
      if ((sourceUg=dynamic_cast<vtkUnstructuredGrid*>(source))
        && (outUg=dynamic_cast<vtkUnstructuredGrid*>(out)))
        {
        if (this->Mode==MODE_TOPOLOGY)
          {
          traceData=new UnstructuredFieldTopologyMap;
          }
        else
          {
          traceData=new UnstructuredFieldDisplacementMap;
          }
        }
      else
        {
        vtkErrorMacro(
          << "Unsupported combination of seed("
          << safeio(source->GetClassName()) << ") or input("
          << safeio(out->GetClassName()) << ") types.");
        }
      if (sourceGen)
        {
        traceData->SetSource(sourceGen);
        }
      else
        {
        traceData->SetSource(source);
        }
      traceData->SetOutput(out);
      }
      break;

    case MODE_STREAM:
      {
      unsigned long gid=0;
      if (!this->UseDynamicScheduler)
        {
        gid=this->GetGlobalCellId(source);
        }
      StreamlineData *lineData=new StreamlineData;
      lineData->SetSourceCellGid(gid);
      lineData->SetSource(source);
      lineData->SetOutput(out);
      traceData=lineData;
      }
      break;

    case MODE_POINCARE:
      {
      PoincareMapData *mapData=new PoincareMapData;
      if (sourceGen)
        {
        mapData->SetSource(sourceGen);
        }
      else
        {
        unsigned long gid=0;
        if (!this->UseDynamicScheduler)
          {
          gid=this->GetGlobalCellId(source);
          }
        mapData->SetSourceCellGid(gid);
        mapData->SetSource(source);
        }
      mapData->SetOutput(out);
      traceData=mapData;
      }
      break;

    default:
      vtkErrorMacro("Invalid mode " << this->Mode << ".");
      break;
    }

  TerminationCondition *tcon=traceData->GetTerminationCondition();
  // Initialize termination condition with the problem
  // domain, any other termination conditions are optional.
  tcon->SetProblemDomain(pDomain,periodicBC);
  // Include and additional termination surfaces from the third and
  // optional input.
  int nSurf=inputVector[2]->GetNumberOfInformationObjects();
  for (int i=0; i<nSurf; ++i)
    {
    info=inputVector[2]->GetInformationObject(i);
    vtkPolyData *pd=
    dynamic_cast<vtkPolyData*>(info->Get(vtkDataObject::DATA_OBJECT()));
    if (pd==0)
      {
      vtkWarningMacro("Termination surface is not polydata. Skipping.");
      continue;
      }
    // SciberQuest filters provide a name that is use when creating
    // the legend. (optional).
    const char *surfName=0;
    if (info->Has(vtkSQMetaDataKeys::DESCRIPTIVE_NAME()))
      {
      surfName=info->Get(vtkSQMetaDataKeys::DESCRIPTIVE_NAME());
      }
    tcon->PushTerminationSurface(pd,surfName);
    }
  tcon->InitializeColorMapper();

  /// Work loops
  if (this->UseDynamicScheduler)
    {
    #if vtkSQFieldTracerDEBUG>1
    pCerr() << "Starting dynamic scheduler." << std::endl;
    #endif
    // This requires all process to have all the seed source data
    // present.
    vtkIdType nSourceCells
      = (sourceGen!=0?sourceGen->GetNumberOfCells():source->GetNumberOfCells());

    this->IntegrateDynamic(
          this->WorldRank,
          this->WorldSize,
          nSourceCells,
          fieldName,
          oocr.GetPointer(),
          oocrCache,
          traceData);
    }
  else
    {
    #if vtkSQFieldTracerDEBUG>1
    pCerr() << "Static distribution assumed." << std::endl;
    #endif
    // This assumes that seed source is distrubuted such that each
    // process has a unique portion of the work.
    this->IntegrateStatic(
          source->GetNumberOfCells(),
          fieldName,
          oocr.GetPointer(),
          oocrCache,
          traceData);
    }

  /// Remove segments where a periodic bc was applied.
  if ( (this->Mode==MODE_STREAM)
    && this->CullPeriodicTransitions
    && (periodicBC[0] || periodicBC[1] || periodicBC[2]) )
    {
    ((StreamlineData*)traceData)->CullPeriodicTransitions(pDomain);
    }

  /// Clean up
  // print a legend, and (optionally) reduce the number of colors to that which
  // are used. The reduction makes use of global communication.
  traceData->PrintLegend(this->SqueezeColorMap);

  // close the open file and release reader.
  oocr->Close();
  oocr->Delete();

  delete traceData;
  #endif

  if (this->LogLevel || globalLogLevel)
    {
    log->EndEvent("vtkSQFieldTracer::RequestData");
    }

  return 1;
}

//-----------------------------------------------------------------------------
inline
int vtkSQFieldTracer::IntegrateStatic(
      vtkIdType nCells,
      const char *fieldName,
      vtkSQOOCReader *oocr,
      vtkDataSet *&oocrCache,
      FieldTraceData *traceData)
{
  #if defined vtkSQFieldTracerTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQFieldTracer::IntegrateStatic");
  #endif

  // do all local ids in a single pass.
  IdBlock sourceIds;
  sourceIds.first()=0;
  sourceIds.size()=(int)nCells;

  int ok=this->IntegrateBlock(
            &sourceIds,
            traceData,
            fieldName,
            oocr,
            oocrCache);

  #if defined vtkSQFieldTracerTIME
  log->EndEvent("vtkSQFieldTracer::IntegrateStatic");
  #endif

  return ok;
}

//-----------------------------------------------------------------------------
int vtkSQFieldTracer::IntegrateDynamic(
      int procId,
      int nProcs,
      vtkIdType nCells,
      const char *fieldName,
      vtkSQOOCReader *oocr,
      vtkDataSet *&oocrCache,
      FieldTraceData *traceData)
{
  #if defined vtkSQFieldTracerTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQFieldTracer::IntegrateDynamic");
  #endif

  #ifdef SQTK_WITHOUT_MPI
  (void)procId;
  (void)nProcs;
  (void)nCells;
  (void)fieldName;
  (void)oocr;
  (void)oocrCache;
  (void)traceData;
  #else
  const int masterProcId=(nProcs>1?1:0); // NOTE: proc 0 is busy with PV overhead.
  const int BLOCK_REQ=12345;
  // Master process distributes the work and integrates
  // in between servicing requests for work.
  // when running poincare mapper master should not start integrating until
  // all workers have started because a single trace can take a long time
  // leaving workers idle.
  if (procId==masterProcId)
    {
    std::vector<MPI_Request> reqs;
    int workerBlockSize=std::min(this->WorkerBlockSize,std::max((int)nCells/nProcs,1));
    int masterBlockSize;
    if (nProcs==1) masterBlockSize=workerBlockSize;
    else masterBlockSize=std::min(workerBlockSize,this->MasterBlockSize);
    WorkQueue Q((int)nCells);
    int nActiveWorkers=nProcs-1;
    int moreWork=1;
    while (nActiveWorkers || moreWork)
      {
      #if defined vtkSQFieldTracerTIME
      vtkSQLog *log=vtkSQLog::GetGlobalInstance();
      log->StartEvent("vtkSQFieldTracer::ServiceWorkRequest");
      #endif
      // dispatch any and all pending requests
      int pendingReq=0;
      do
        {
        MPI_Status stat;
        MPI_Iprobe(MPI_ANY_SOURCE,BLOCK_REQ,MPI_COMM_WORLD,&pendingReq,&stat);
        if (pendingReq)
          {
          // the pending message lets us know someone needs work. The
          // message itself contains no useful information.
          int buf;
          int otherProc=stat.MPI_SOURCE;
          MPI_Recv(&buf,0,MPI_INT,otherProc,BLOCK_REQ,MPI_COMM_WORLD,&stat);
          // get a block of work.
          IdBlock sourceIds;
          moreWork=Q.GetBlock(sourceIds,workerBlockSize);
          // send the work. If there is no more, send zero sized block
          // this closes all workers.
          MPI_Request req;
          MPI_Isend(
              sourceIds.data(),
              (int)sourceIds.dataSize(),
              MPI_UNSIGNED_LONG_LONG,
              otherProc,
              BLOCK_REQ,
              MPI_COMM_WORLD,
              &req);
          reqs.push_back(req);
          if (!moreWork)
            {
            --nActiveWorkers;
            }
           #if vtkSQFieldTracerDEBUG>0
           pCerr() << "Master filled request from " << otherProc << " " << sourceIds << std::endl;
           #endif
          }
        }
      while (pendingReq);
      #if defined vtkSQFieldTracerTIME
      log->EndEvent("vtkSQFieldTracer::ServiceWorkRequest");
      #endif

      // now that all the worker that need work have it. Do a small amount
      // of work while the others are busy.
      if ((masterBlockSize>0) || (nProcs==1))
        {
        IdBlock sourceIds;
        moreWork=Q.GetBlock(sourceIds,masterBlockSize);
        if (moreWork)
          {
          #if vtkSQFieldTracerDEBUG>0
          pCerr() << "Master integrating " << sourceIds << std::endl;
          #endif
          this->IntegrateBlock(
                  &sourceIds,
                  traceData,
                  fieldName,
                  oocr,
                  oocrCache);

          double prog=(double)sourceIds.last()/(double)nCells;
          this->UpdateProgress(prog);
          }
        }
      }
      MPI_Waitall((int)reqs.size(),&reqs[0],MPI_STATUS_IGNORE);
    }
  // Work processes receive chunks of seed cell ids and
  // integrate.
  else
    {
    while (1)
      {
      #if vtkSQFieldTracerDEBUG>0
      pCerr() << "Slave " << procId << " requesting work" << std::endl;
      #endif
      #if defined vtkSQFieldTracerTIME
      vtkSQLog *log=vtkSQLog::GetGlobalInstance();
      log->StartEvent("vtkSQFieldTracer::WorkRequest");
      #endif

      // get a block of seed cell ids to process.
      MPI_Send(&procId,0,MPI_INT,masterProcId,BLOCK_REQ,MPI_COMM_WORLD);
      MPI_Status stat;
      IdBlock sourceIds;
      MPI_Recv(
          sourceIds.data(),
          (int)sourceIds.dataSize(),
          MPI_UNSIGNED_LONG_LONG,
          masterProcId,
          BLOCK_REQ,
          MPI_COMM_WORLD,
          &stat);

      #if vtkSQFieldTracerDEBUG>0
      pCerr() << "Slave " << procId << " received " << sourceIds << std::endl;
      #endif
      #if defined vtkSQFieldTracerTIME
      log->EndEvent("vtkSQFieldTracer::WorkRequest");
      #endif

      // stop when no work has been issued.
      if (sourceIds.size()==0){ break; }

      // integrate this block
      this->IntegrateBlock(
                &sourceIds,
                traceData,
                fieldName,
                oocr,
                oocrCache);

      double prog=(double)sourceIds.last()/(double)nCells;
      this->UpdateProgress(prog);
      }
    }
  #endif

  #if defined vtkSQFieldTracerTIME
  log->EndEvent("vtkSQFieldTracer::IntegrateDynamic");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQFieldTracer::IntegrateBlock(
      IdBlock *sourceIds,
      FieldTraceData *traceData,
      const char *fieldName,
      vtkSQOOCReader *oocr,
      vtkDataSet *&oocrCache)

{
  // build the output.
  #if defined vtkSQFieldTracerTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQFieldTracer::InsertCells");
  #endif
  vtkIdType nLines=traceData->InsertCells(sourceIds);
  #if defined vtkSQFieldTracerTIME
  log->EndEvent("vtkSQFieldTracer::InsertCells");
  #endif

  TerminationCondition *tcon=traceData->GetTerminationCondition();

  for (vtkIdType i=0; i<nLines; ++i) //, prog+=progInc)
    {
    // progress report for static load balance. the report
    // for dunamic load balance is done once for each block.
    if (!this->UseDynamicScheduler && !(i%10))
      {
      double prog=(double)i/(double)nLines;
      this->UpdateProgress(prog);
      }

    // trace a stream line
    FieldLine *line=traceData->GetFieldLine(i);
    this->IntegrateOne(oocr,oocrCache,fieldName,line,tcon);

    #if vtkSQFieldTracerDEBUG>=0
    cstd::err << ".";
    #endif
    }

  // sync results to output. free resources in preparation
  // for the next pass.
  #if defined vtkSQFieldTracerTIME
  log->StartEvent("vtkSQFieldTracer::SyncOutput");
  #endif
  traceData->SyncScalars();
  traceData->SyncGeometry();
  traceData->ClearFieldLines();
  #if defined vtkSQFieldTracerTIME
  log->EndEvent("vtkSQFieldTracer::SyncOutput");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQFieldTracer::IntegrateOne(
      vtkSQOOCReader *oocR,
      vtkDataSet *&oocRCache,
      const char *fieldName,
      FieldLine *line,
      TerminationCondition *tcon)
{
  #if defined vtkSQFieldTracerTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQFieldTracer::Integrate");
  #endif

  // Sanity check -- seed point is in bounds. If not skip it.
  double seed[3];
  line->GetSeedPoint(seed);
  if (tcon->OutsideProblemDomain(seed))
    {
    #if vtkSQFieldTracerDEBUG>1
    pCerr()
      << "Terminated: Seed " << Tuple<float>(line->GetSeedPoint(),3)
      << " outside problem domain.";
    #endif
    #if defined vtkSQFieldTracerTIME
    log->EndEvent("vtkSQFieldTracer::Integrate");
    #endif
    return;
    }

  // start by adding the seed point to the forward trace.
  // adding here simplifies the logic in corner case failures
  // below, at the cost of a float 3-tuple's worth of memory.
  if ((this->Mode==MODE_STREAM)||(this->MODE_DISPLACEMENT))
    {
    line->PushPoint(1,seed);
    }

  // make the integration, possibly in two parts a backward and then
  // a forward trace.
  int i=(this->ForwardOnly?1:0);
  int stepSign=(this->ForwardOnly?1:-1);

  for (; i<2; stepSign+=2, ++i)
    {
    double minStep=this->MinStep;
    double maxStep=this->MaxStep;
    double stepSize=stepSign*this->MaxStep; // initial step size
    double lineLength=0.0;                  // cumulative length of stream line
    double timeInterval=0.0;                // cumulative integration time
    vtkIdType numSteps=0;                   // cumulative number of steps taken in integration
    double V0[3]={0.0};                     // vector field interpolated at the start point
    double p0[3]={0.0};                     // start point
    double p1[3]={0.0};                     // integrated point, periodic coordinate space.
    double p2[3]={0.0};                     // integrated point, non-periodic coordinate space.
    double s0[3]={0.0};                     // segment start point
    int bcSurf=0;                           // set when a periodic boundary condition has been applied.
    static                                  // interpolator
    vtkInterpolatedVelocityField *interp=0;
    #if vtkSQFieldTracerDEBUG>1
    double minStepTaken=VTK_DOUBLE_MAX;
    double maxStepTaken=VTK_DOUBLE_MIN;
    #endif

    // initialize at the seed point.
    line->GetSeedPoint(p0);
    line->GetSeedPoint(p1);
    line->GetSeedPoint(p2);
    line->GetSeedPoint(s0);

    // Integrate until the maximum line length is reached, maximum number of
    // steps is reached or until a termination surface is encountered.
    while (1)
      {
      #if vtkSQFieldTracerDEBUG>4
      pCerr() << " " << Tuple<double>(p0,3) << std::endl;
      #endif

      // Load a block if the seed point is not contained in the current block.
      if (tcon->OutsideWorkingDomain(p0))
        {
        #if defined vtkSQFieldTracerTIME
        log->EndEvent("vtkSQFieldTracer::Integrate");
        log->StartEvent("vtkSQFieldTracer::LoadBlock");
        #endif
        oocRCache=oocR->ReadNeighborhood(p0,tcon->GetWorkingDomain());
        if (!oocRCache)
          {
          vtkErrorMacro("Read neighborhood failed.");
          return;
          }
        // Initialize the vector field interpolator.
        interp=vtkInterpolatedVelocityField::New();
        interp->AddDataSet(oocRCache);
        interp->SelectVectors(vtkDataObject::FIELD_ASSOCIATION_POINTS,fieldName);
        this->Integrator->SetFunctionSet(interp);
        interp->Delete();
        #if defined vtkSQFieldTracerTIME
        log->EndEvent("vtkSQFieldTracer::LoadBlock");
        log->StartEvent("vtkSQFieldTracer::Integrate");
        #endif
        }

      // interpolate vector field at seed point.
      interp->FunctionValues(p0,V0);
      double speed=vtkMath::Norm(V0);
      // check for field null
      if ((speed==0) || (speed<=this->NullThreshold))
        {
        #if vtkSQFieldTracerDEBUG>1
        pCerr() << "Terminated: Interpolated field null." << std::endl;
        #endif
        line->SetTerminator(i,tcon->GetFieldNullId());
        break;
        }

      if (this->IntegratorType==INTEGRATOR_RK45)
        {
        // clear step sign
        stepSize=fabs(stepSize);

        // check step size for error bound violation.
        minStep=this->MinStep;
        maxStep=this->MaxStep;
        if (stepSize<=minStep)
          {
          interp->FunctionValues(p0,V0);
          vtkErrorMacro(
            << "Error bound of "
            << this->MaxError << " could not be met with "
            << "minimum step size of " << this->MinStep << ". "
            << "Either raise the error bound or reduce the "
            << "minimum step size."
            << "x=" << Tuple<double>(p0,3) << ", "
            << "v=" << Tuple<double>(V0,3));
          #if defined vtkSQFieldTracerTIME
          log->EndEvent("vtkSQFieldTracer::Integrate");
          #endif
          return;
          }
        // clip step to max step size.
        else
        if (stepSize>maxStep)
          {
          stepSize=maxStep;
          }
        // fix sign on step bounds
        minStep*=stepSign;
        maxStep*=stepSign;
        // fix sign of step size
        stepSize*=stepSign;
        }

      /// Integrate
      // Note, both stepSize and stepTaken are updated
      // by the rk45 integrator.
      interp->SetNormalizeVector(true);
      double error=0.0;
      double stepTaken=0.0;
      int iErr=this->Integrator->ComputeNextStep(
          p0,p1,0,
          stepSize,
          stepTaken,
          minStep,
          maxStep,
          this->MaxError,
          error);
      interp->SetNormalizeVector(false);

      // integrator errors, 1=out of bounds, 2=uninitialized,
      // 3=unexepcted val. Have to handle out of bounds because
      // in some cases p1 is not updated which leads to incorrect
      // classification as a field null.
      if (iErr==1)
        {
        #if vtkSQFieldTracerDEBUG>1
        pCerr() << "Terminated: Integrator reports outside problem domain." << std::endl;
        #endif
        line->SetTerminator(i,tcon->GetProblemDomainSurfaceId());
        // don't push the point , if p1 was not updated then it potentially
        // leads to an incorrect result.
        break;
        }

      #if vtkSQFieldTracerDEBUG>2
      pCerr() << (stepSign<0?"<":">");

      if (iErr)
        {
        pCerr() << "Integrator error " << iErr << ".";
        //break;
        }
      #endif

      #if vtkSQFieldTracerDEBUG>1
      if (this->IntegratorType==INTEGRATOR_RK45)
        {
        minStepTaken=std::min(minStepTaken,fabs(stepTaken));
        maxStepTaken=std::max(maxStepTaken,fabs(stepTaken));
        }
      #endif

      double delta[3];
      delta[0]=p1[0]-p0[0];
      delta[1]=p1[1]-p0[1];
      delta[2]=p1[2]-p0[2];

      // update the seed in the non-peridoic coordinate system.
      if (this->Mode==MODE_DISPLACEMENT)
        {
        p2[0]+=delta[0];
        p2[1]+=delta[1];
        p2[2]+=delta[2];
        }

      // update the arc length and number of steps taken.
      double dx=sqrt(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);
      lineLength+=dx;

      ++numSteps;

      // Use v=dx/dt to calculate speed and check if it is below
      // stagnation threshold. (test prior to tests that modify p1)
      double dt=fabs(stepTaken);
      double v=dx/dt;
      if (v<=this->NullThreshold)
        {
        #if vtkSQFieldTracerDEBUG>1
        pCerr()
          << "Terminated: Field null encountered. "
          << "v=" << v << ". "
          << "thresh=" << this->NullThreshold << "." << std::endl;
        #endif
        line->SetTerminator(i,tcon->GetFieldNullId());

        if (this->Mode==MODE_STREAM)
          {
          line->PushPoint(i,p1);
          }
        else
        if (this->Mode==MODE_DISPLACEMENT)
          {
          line->PushPoint(i,p2);
          }
        break;
        }

      // check the integration interval
      timeInterval+=dt;
      if (timeInterval>=this->MaxIntegrationInterval)
        {
        #if vtkSQFieldTracerDEBUG>1
        pCerr()
          << "Terminated: Max integration interval exceeded. "
          << "nSteps= " << numSteps << "."
          << "arcLen= " << lineLength << "."
          << "timeInterval= " << timeInterval << "."
          << std::endl;
        #endif
        line->SetTerminator(i,tcon->GetShortIntegrationId());

        if (this->Mode==MODE_STREAM)
          {
          line->PushPoint(i,p1);
          }
        else
        if (this->Mode==MODE_DISPLACEMENT)
          {
          line->PushPoint(i,p2);
          }
        break;

        }

      // check arc-length for a termination condition
      if (lineLength>this->MaxLineLength)
        {
        #if vtkSQFieldTracerDEBUG>1
        pCerr()
          << "Terminated: Max arc length exceeded. "
          << "nSteps= " << numSteps << "."
          << "arcLen= " << lineLength << "."
          << "timeInterval= " << timeInterval << "."
          << std::endl;
        #endif
        line->SetTerminator(i,tcon->GetShortIntegrationId());

        if (this->Mode==MODE_STREAM)
          {
          line->PushPoint(i,p1);
          }
        else
        if (this->Mode==MODE_DISPLACEMENT)
          {
          line->PushPoint(i,p2);
          }
        break;
        }

      // check step number agnainst the fail-safe termination condition.
      if ( (this->IntegratorType==INTEGRATOR_RK45)
        && (numSteps>this->MaxNumberOfSteps) )
        {
        #if vtkSQFieldTracerDEBUG>1
        pCerr()
          << "Terminated: Max number of steps exceeded. "
          << "nSteps= " << numSteps << "."
          << "arcLen= " << lineLength << "."
          << "timeInterval= " << timeInterval << "."
          << std::endl;
        #endif
        line->SetTerminator(i,tcon->GetShortIntegrationId());

        if (this->Mode==MODE_STREAM)
          {
          line->PushPoint(i,p1);
          }
        else
        if (this->Mode==MODE_DISPLACEMENT)
          {
          line->PushPoint(i,p2);
          }
        break;
        }

      // Check for instersection with a terminator surface.
      double pi[3];
      int surfIsect=tcon->IntersectsTerminationSurface(p0,p1,pi);
      if (surfIsect)
        {
        #if vtkSQFieldTracerDEBUG>1
        pCerr()
          << (this->Mode==MODE_POINCARE?"Continued:":"Terminated:")
          << "Surface " << surfIsect-1 << " intersected." << std::endl;
        #endif
        line->SetTerminator(i,surfIsect);

        if (this->Mode==MODE_DISPLACEMENT)
          {
          line->PushPoint(i,p2);
          }
        else
        if ((this->Mode==MODE_STREAM)||(this->Mode==MODE_POINCARE))
          {
          line->PushPoint(i,pi);
          }

        if (!(this->Mode==MODE_POINCARE)) break;
        }

      // Check for and apply periodic BC on p1 only if in the last
      // a periodic boundary condition wasn't applied (otherwise
      // you can ping pong back and forth)
      if (!bcSurf)
        {
        bcSurf=tcon->ApplyPeriodicBC(p0,p1);
        }
      else
        {
        bcSurf=0;
        }
      if (bcSurf)
        {
        #if vtkSQFieldTracerDEBUG>1
        pCerr() << "Periodic BC Applied: At " << bcSurf << "." << std::endl;
        #endif
        }
      // If we aren't applying a periodic boundary condition
      // then make a sure we are still integrating inside the
      // vaid region.
      else
      if (tcon->OutsideProblemDomain(p0,p1))
        {
        #if vtkSQFieldTracerDEBUG>1
        pCerr() << "Terminated: Integration outside problem domain." << std::endl;
        #endif
        line->SetTerminator(i,tcon->GetProblemDomainSurfaceId());
        if (this->Mode==MODE_STREAM)
          {
          line->PushPoint(i,p1);
          }
        else
        if (this->Mode==MODE_DISPLACEMENT)
          {
          line->PushPoint(i,p2);
          }
        break;
        }

      // add the point to the stream line if it would create a segment
      // geometry longer than the minimum. This drastically reduces the
      // number of poly line segments in the output while maintaining
      // visual quality.
      if (this->Mode==MODE_STREAM)
        {
        double ds=0.0;
        ds+=(p1[0]-s0[0])*(p1[0]-s0[0]);
        ds+=(p1[1]-s0[1])*(p1[1]-s0[1]);
        ds+=(p1[2]-s0[2])*(p1[2]-s0[2]);
        ds=sqrt(ds);
        if (ds>=this->MinSegmentLength)
          {
          line->PushPoint(i,p1);
          s0[0]=p1[0];
          s0[1]=p1[1];
          s0[2]=p1[2];
          }
        }

      // Update the seed point for the next integraiton.
      p0[0]=p1[0];
      p0[1]=p1[1];
      p0[2]=p1[2];
      }

    #if vtkSQFieldTracerDEBUG>1
    if (this->IntegratorType==INTEGRATOR_RK45)
      {
      pCerr()
        << "rk-4-5 step sizes "
        << minStepTaken << ", " << maxStepTaken <<  "."
        << std::endl;
      }
    #endif
    }

  #if defined vtkSQFieldTracerTIME
  log->EndEvent("vtkSQFieldTracer::Integrate");
  #endif

  return;
}

//-----------------------------------------------------------------------------
unsigned long vtkSQFieldTracer::GetGlobalCellId(vtkDataSet *data)
{
  unsigned long gid=0;

  #ifdef SQTK_WITHOUT_MPI
  (void)data;
  #else
  unsigned long nLocal=data->GetNumberOfCells();

  unsigned long *nGlobal
    = (unsigned long *)malloc(this->WorldSize*sizeof(unsigned long));

  MPI_Allgather(
        &nLocal,1,MPI_UNSIGNED_LONG,
        nGlobal,1,MPI_UNSIGNED_LONG,
        MPI_COMM_WORLD);

  for (int i=0; i<this->WorldRank; ++i)
    {
    gid+=nGlobal[i];
    }

  free(nGlobal);
  #endif

  return gid;
}

//-----------------------------------------------------------------------------
double vtkSQFieldTracer::ConvertToLength(
      double interval,
      int unit,
      double cellLength)
{
  double retVal = 0.0;
  switch (unit)
    {
    case ARC_LENGTH:
      retVal=interval;
      break;
    case CELL_FRACTION:
      retVal=interval*cellLength;
      break;
    }
  return retVal;
}

//-----------------------------------------------------------------------------
void vtkSQFieldTracer::ClipStep(
      double& step,
      int stepSign,
      double& minStep,
      double& maxStep,
      double cellLength,
      double lineLength)
{
  // clip to cell fraction
  minStep=this->ConvertToLength(this->MinStep,this->StepUnit,cellLength);
  maxStep=this->ConvertToLength(this->MaxStep,this->StepUnit,cellLength);
  if (step<minStep)
    {
    step=minStep;
    }
  else
  if (step>maxStep)
    {
    step=maxStep;
    }
  // clip to max line length
  double newLineLength=step+lineLength;
  if (newLineLength>this->MaxLineLength)
    {
    step=newLineLength-this->MaxLineLength;
    }
  // fix up the sign (this assumes that step is always > 0)
  step*=stepSign;
  minStep*=stepSign;
  maxStep*=stepSign;
}


//-----------------------------------------------------------------------------
void vtkSQFieldTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
