/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVisItDatabaseBridge.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVisItDatabaseBridge.h"
#include "vtkVisItDatabase.h"
#include "vtkVisItDatabaseBridgeTypes.h"
#include "vtkCallbackCommand.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkDataArraySelection.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include <vtksys/SystemTools.hxx>

#include <vtkstd/string>
using vtkstd::string;

#include <vtkstd/vector>
using vtkstd::vector;


vtkCxxRevisionMacro(vtkVisItDatabaseBridge, "1.5");
vtkStandardNewMacro(vtkVisItDatabaseBridge);

// Never try to print a null pointer to a string.
const char *safeio(const char *s){ return (s?s:"NULL"); }
// Compare two doubles.
int fequal(double a, double b, double tol)
{
  double pda=fabs(a);
  double pdb=fabs(b);
  pda=pda<tol?tol:pda;
  pdb=pdb<tol?tol:pdb;
  double smaller=pda<pdb?pda:pdb;
  double norm=fabs(b-a)/smaller;
  if (norm<=tol)
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkVisItDatabaseBridge::vtkVisItDatabaseBridge()
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================vtkVisItDatabaseBridge" << endl;
#endif

  this->FileName=0;
  this->PluginId=0;
  this->PluginPath=0;

  const char* executable_path = vtkProcessModule::GetProcessModule()->
    GetOptions()->GetArgv0();
  this->SetPluginPath(
    vtksys::SystemTools::GetFilenamePath(executable_path).c_str());

  this->Clear();
  this->VisitSource=vtkVisItDatabase::New();

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  /// PV Interface.
  // Setup the selection callback to modify this object when
  // selections are changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
          &vtkVisItDatabaseBridge::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);

  this->DatabaseViewMTime=0;
}

//-----------------------------------------------------------------------------
vtkVisItDatabaseBridge::~vtkVisItDatabaseBridge()
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================~vtkVisItDatabaseBridge" << endl;
#endif

  this->Clear();

  if (this->VisitSource) 
    {
    this->VisitSource->Delete();
    }

  /// PV Interface.
  this->SelectionObserver->Delete();

}

//-----------------------------------------------------------------------------
int vtkVisItDatabaseBridge::UpdateVisitSource()
{
  int ok;
  // We need to re-configure first.
  if (this->UpdatePlugin)
    {
    this->VisitSource->SetPluginPath(this->PluginPath);
    this->VisitSource->SetPluginId(this->PluginId);
    ok=this->VisitSource->Configure();
    if (!ok)
      {
      vtkWarningMacro("Failed to configure the Visit source.");
      return 0;
      }
    this->UpdatePlugin=0;
    }
  // We need to reload the database first.
  if (this->UpdateDatabase)
    {
    this->VisitSource->CloseDatabase();
    this->VisitSource->SetFileName(this->FileName);
    ok=this->VisitSource->OpenDatabase();
    if (!ok)
      {
      vtkWarningMacro("Failed to open the Visit database.");
      return 0;
      }
    this->UpdateDatabase=0;
    }
  return 1;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkVisItDatabaseBridge::CreateDefaultExecutive()
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================CreateDefaultExecutive" << endl;
#endif

  return vtkCompositeDataPipeline::New();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::Clear()
{
  this->SetFileName(0);
  this->SetPluginId(0);
  //this->SetPluginPath(0);

  this->UpdatePlugin=1;
  this->UpdateDatabase=1;
  this->UpdateDatabaseView=1;
}

//-----------------------------------------------------------------------------
int vtkVisItDatabaseBridge::CanReadFile(const char *file)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================CanReadFile" << endl;
  cerr << "Check " << safeio(file) << "." << endl;
#endif
  // Configure.
  if (this->UpdatePlugin
    || !this->VisitSource->Configured())
    {
    this->VisitSource->SetPluginPath(this->PluginPath);
    this->VisitSource->SetPluginId(this->PluginId);
    int ok=this->VisitSource->Configure();
    if (!ok)
      {
      vtkWarningMacro("Failed to configure the Visit source.");
      return 0;
      }
    this->UpdatePlugin=0;
    }
  // Close.
  this->VisitSource->CloseDatabase();
  // Open with new file.
  this->VisitSource->SetFileName(file);
  int ret=this->VisitSource->OpenDatabase();
  this->VisitSource->SetFileName(NULL); 
  // Note to self, to re-load the database.
  this->UpdateDatabase=1;
  this->UpdateDatabaseView=1;

  return ret;
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetFileName(const char* _arg)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "Set FileName from " << safeio(this->FileName) << " to " << safeio(_arg) << "." << endl;
#endif

  vtkDebugMacro(<< this->GetClassName() << ": setting FileName to " << (_arg?_arg:"(null)"));
  if (this->FileName == NULL && _arg == NULL) { return;}
  if (this->FileName && _arg && (!strcmp(this->FileName,_arg))) { return;}
  if (this->FileName) { delete [] this->FileName; }
  if (_arg)
    {
    size_t n = strlen(_arg) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (_arg);
    this->FileName = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
  else
    {
    this->FileName = NULL;
    }
  // This tells us that the current database must be re-loaded.
  this->UpdateDatabase=1;
  this->UpdateDatabaseView=1;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetPluginPath(const char* _arg)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "Set PluginPath from " << safeio(this->PluginPath) << " to " << safeio(_arg) << "." << endl;
#endif

  vtkDebugMacro(<< this->GetClassName() << ": setting PluginPath to " << (_arg?_arg:"(null)"));
  if (this->PluginPath == NULL && _arg == NULL) { return;}
  if (this->PluginPath && _arg && (!strcmp(this->PluginPath,_arg))) { return;}
  if (this->PluginPath) { delete [] this->PluginPath; }
  if (_arg)
    {
    size_t n = strlen(_arg) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (_arg);
    this->PluginPath = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
  else
    {
    this->PluginPath = NULL;
    }
  // This tells us that the current plugin & database must be 
  // re-loaded.
  this->UpdatePlugin=1;
  this->UpdateDatabase=1;
  this->UpdateDatabaseView=1;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetPluginId(const char* _arg)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "Set PluginId from " << safeio(this->PluginId) << " to " << safeio(_arg) << "." << endl;
#endif

  vtkDebugMacro(<< this->GetClassName() << ": setting PluginId to " << (_arg?_arg:"(null)"));
  if (this->PluginId == NULL && _arg == NULL) { return;}
  if (this->PluginId && _arg && (!strcmp(this->PluginId,_arg))) { return;}
  if (this->PluginId) { delete [] this->PluginId; }
  if (_arg)
    {
    size_t n = strlen(_arg) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (_arg);
    this->PluginId = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
  else
    {
    this->PluginId = NULL;
    }
  // This tells us that the current plugin and database must be
  // re-loaded.
  this->UpdatePlugin=1;
  this->UpdateDatabase=1;
  this->UpdateDatabaseView=1;
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkVisItDatabaseBridge::RequestInformation(
  vtkInformation* req,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================RequestInformation" << endl;
#endif

  if (!this->UpdateDatabaseView)
    {
    // Nothing has changed since the last request for information,
    // so we have nothing to do.
    return 1;
    }
  this->UpdateDatabaseView=0;

  vtkInformation* info=outputVector->GetInformationObject(0);

  const int nMeshes=this->VisitSource->GetNumberOfMeshes();
  // Either, Multiple meshes are stored in this file
  if (nMeshes>1)
    {
    info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
    }
  // Or, a single mesh is stored in this file.
  else
    {
    // Either the data produced is AMR or MultiBlock data...
    if (this->VisitSource->ProducesAMRData(0)
      || this->VisitSource->ProducesMultiBlockData(0))
      {
      info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
      }
    // Or, the data produced is ordinary VTK data.
    else
      {
      if (this->VisitSource->DecomposesDomain())
        {
        info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);
        // TODO: Databases with this property decide how the data is split up.
        }
      else
      if (this->VisitSource->ProducesStructuredData(0))
        {
        if (!info->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
          {
          // i,j,k extents can't be found without reading data...
          // insert this place holder and adjust in RequestData.
          int ext[6]={0,1,0,1,0,1};
          info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext,6);
#if defined vtkVisItDatabaseBridgeDEBUG
          cerr << "Set extents to 0, 1, 0, 1, 0, 1." << endl;
#endif
          }
        }
      else
        {
        info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),1);
        }
      }
    }

  // Create a view of the SIL. The SIL tells us the relationships
  // of the various entities in the database. The view exposes
  // these to the U.I. for convenient manipulation.
  vtkMutableDirectedGraph *view=vtkMutableDirectedGraph::New();
  this->VisitSource->GenerateDatabaseView(view);
  info->Set(vtkDataObject::SIL(), view);
  view->Delete();
  ++this->DatabaseViewMTime;

  // Enumerate Array and Expressions as strings because the client
  // will send a list of id's while VisIt expects the strings.
  this->ArrayNames.clear();
  this->VisitSource->EnumerateDataArrays(this->ArrayNames);
  this->ExpressionNames.clear();
  this->VisitSource->EnumerateExpressions(this->ExpressionNames);

  // Determine which time steps are available.
  vector<double> times;
  this->VisitSource->EnumerateTimeSteps(times);
  info->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(),&times[0],times.size());
  if (times.size()>1)
    {
    double timeRange[2]={times[0],times[times.size()-1]};
    info->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),timeRange,2);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkVisItDatabaseBridge::RequestDataObject(
  vtkInformation* /*req*/,
  vtkInformationVector** /*inputVector*/,
  vtkInformationVector* outputVector)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================RequestDataObject" << endl;
  cerr << safeio(this->FileName) << endl;
#endif

  int ok;
  ok=this->UpdateVisitSource();
  if (!ok)
    {
    vtkWarningMacro("Failed to update the visit source.");
    return 0;
    }
  ok=this->VisitSource->UpdateMetaData();
  if (!ok)
    {
    vtkWarningMacro("Failed to read the meta-data.");
    return 0;
    }

  vtkInformation* info=outputVector->GetInformationObject(0);

  // Construct the data object.
  // TODO no need to construct each time.
  const char *datasetType=this->VisitSource->GetDataObjectType();
  vtkDataObject *dataset=this->VisitSource->GetDataObject();
  if (!datasetType||!dataset)
    {
    vtkWarningMacro(
        "Failed to get the dataset or type. DatasetType is " 
        << safeio(datasetType) <<  ". DataSet is " << dataset << ". ");
    return 0;
    }
  info->Set(vtkDataObject::DATA_TYPE_NAME(),datasetType);
  info->Set(vtkDataObject::DATA_OBJECT(),dataset);
  info->Set(vtkDataObject::DATA_EXTENT_TYPE(), dataset->GetExtentType());
  dataset->SetPipelineInformation(info);
  dataset->Delete();
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "datasetType=" << info->Get(vtkDataObject::DATA_TYPE_NAME()) << endl;
  cerr << "dataset=" << info->Get(vtkDataObject::DATA_OBJECT()) << endl;
#endif

  // TODO delete data

  return 1;
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::DistributeBlocks(
        int procId,
        int nProcs,
        int nBlocks,
        vtkstd::vector<int> &blockIds)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================DistributeBlocks" << endl;
#endif

  if (nProcs>nBlocks)
    {
    vtkWarningMacro("More processes than data blocks, some will have no data.");
    if (procId<nBlocks)
      {
      blockIds.push_back(procId);
      }
    }
  else
    {
    int nBlocksPerProc=nBlocks/nProcs;
    int nLeftOver=nBlocks%nProcs;
    int lastProc=nProcs-1;
    int nBlocks
      = nBlocksPerProc+(procId==lastProc?nLeftOver:0);
    int blockId=procId*nBlocksPerProc;
    blockIds.resize(nBlocks);
    for (int i=0; i<nBlocks; ++i)
      {
      blockIds[i]=blockId;
      ++blockId;
      }
    }

  size_t n=blockIds.size();
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "Proc Id " << procId << " owns blocks "
       << blockIds[0] << " through " << blockIds[n-1] << "." << endl;
#endif
}

//-----------------------------------------------------------------------------
int vtkVisItDatabaseBridge::RequestData(
        vtkInformation * /*req*/,
        vtkInformationVector ** /*input*/,
        vtkInformationVector *output)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================RequestData" << endl;
#endif

  int ok;
  vtkInformation *outputInfo=output->GetInformationObject(0);
  // Get the output.
  vtkDataObject *dataOut=outputInfo->Get(vtkDataObject::DATA_OBJECT());

  if (dataOut==NULL)
    {
    vtkWarningMacro("Filter data has not been configured correctly. Aborting.");
    return 1;
    }

  const size_t nActiveMeshes=this->MeshIds.size();
  if (nActiveMeshes==0)
    {
    // No meshes selected then nothing to do.
    return 1;
    }

  // Determine what time step we are being asked for.
  if (outputInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    double *step
      = outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    int nSteps =
      outputInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double* steps =
      outputInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    int stepId=-1;
    for (int i=0; i<nSteps; ++i)
      {
      if (fequal(steps[i],*step,1E-10))
        {
        stepId=i;
        this->VisitSource->SetActiveTimeStep(i);
        break;
        }
      }
    outputInfo->Set(vtkDataObject::DATA_TIME_STEPS(),step,1);
#if defined vtkVisItDatabaseBridgeDEBUG
    cerr << "Requested time " << step[0] << " at " << stepId << "." << endl;
#endif
    }

  // TODO Make use of UPDATE_EXTENT ??

  const int procId
    = outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  const int nProcs
    = outputInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "I am " << procId << " of " << nProcs << "." << endl;
#endif

  // Configure the data object. The pipeline Initializes dataobjects
  // after RequestDataObject so this has to be done here.
  ok=this->VisitSource->ConfigureDataObject(this->MeshIds,dataOut);
  if (!ok)
    {
    vtkWarningMacro("Failed to configure the output. Aborting.");
    return 1;
    }

  // Configure for the read. This involves sorting the arrays by mesh and 
  // load balancing for multiblock data.
  vector<vector<int> >blockIdsThisProc(nActiveMeshes);
  vector<vector<string> >activeArrayNames(nActiveMeshes);
  for (int meshIdx=0; meshIdx<nActiveMeshes; ++meshIdx)
    {
    int meshId=this->MeshIds[meshIdx];
    // Convert array id's into names for VisIt. Id's are negative
    // so that they do not conflict with SIL set ids.
    const size_t nActiveArrays=this->ArrayIds[meshIdx].size();
    for (size_t arrayIdx=0; arrayIdx<nActiveArrays; ++arrayIdx)
      {
      int id=-this->ArrayIds[meshIdx][arrayIdx]-1; // ids are < 0 and off by one in the view.
      activeArrayNames[meshIdx].push_back(this->ArrayNames[meshId][id]);
#if defined vtkVisItDatabaseBridgeDEBUG
      cerr << "Selected " << this->ArrayNames[meshId][id] << endl;
#endif
      }
    // Distribute blocks to processes.
    const int nBlocks=this->VisitSource->GetNumberOfBlocks(this->MeshIds[meshIdx]);
    this->DistributeBlocks(procId,nProcs,nBlocks,blockIdsThisProc[meshIdx]);
    }

  this->UpdateProgress(0.5);

  // Read.
  ok=this->VisitSource->ReadData(
        this->MeshIds,
        activeArrayNames,
        blockIdsThisProc,
        this->DomainSSIds,
        this->MaterialSSIds,
        dataOut);
  ok=1; //TODO This is because AMR support is only partially complete. Remove this line after 3.6 release.
  if (!ok)
    {
    vtkWarningMacro(
      << "Read database encountered an error. Aborting execution.");
    return 1;
    }

  // FIXME
  if (!this->VisitSource->HasMultipleMeshes()
    && !this->VisitSource->ProducesAMRData(0)
    && !this->VisitSource->ProducesMultiBlockData(0)
    && this->VisitSource->ProducesStructuredData(0))
      {
      // For structured data fix WHOLE_EXTENTS. In RequestDataObject 
      // we set a place holder because we don't have a way of determining
      // index space extents before we read the meshes.
      vtkDataSet *ds;
      ds=dynamic_cast<vtkDataSet *>(dataOut);
      int ext[6]={0,1,0,1,0,1};
      vtkRectilinearGrid *rg=dynamic_cast<vtkRectilinearGrid *>(dataOut);
      vtkStructuredGrid *sg=dynamic_cast<vtkStructuredGrid *>(dataOut);
      if (rg)
        {
        rg->GetExtent(ext);
        }
      else
      if (sg)
        {
        sg->GetExtent(ext);
        }
      outputInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),ext,6);
      }

  //dataOut->Print(cerr);
  return 1;
}


//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName:   " << safeio(this->FileName)   << endl;
  os << indent << "PluginPath: " << safeio(this->PluginPath) << endl;
  os << indent << "PluginId:   " << safeio(this->PluginId)   << endl;

  this->VisitSource->PrintSelf(os,indent.GetNextIndent());
}



/// U.I.
//----------------------------------------------------------------------------
// observe PV interface and set modified if user makes changes
void vtkVisItDatabaseBridge::SelectionModifiedCallback(
        vtkObject*,
        unsigned long,
        void* clientdata,
        void*)
{
  vtkVisItDatabaseBridge *dbb
    = static_cast<vtkVisItDatabaseBridge*>(clientdata);

  dbb->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::InitializeUI()
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================InitializeUI" << endl;
#endif

  // initialize read lists.
  this->MeshIds.clear();
  this->ArrayIds.clear();
  this->ExpressionIds.clear();
  this->DomainSSIds.clear();
  this->BlockSSIds.clear();
  this->AssemblySSIds.clear();
  this->MaterialSSIds.clear();
  this->SpeciesSSIds.clear();
  this->Modified();

  // Reset SIL Set operations.
//   this->DomainBlockSSOp=SIL_OPERATION_INTERSECT;
//   this->DomainAssemblySSOp=SIL_OPERATION_INTERSECT;
//   this->DomainMaterialSSOp=SIL_OPERATION_INTERSECT;
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetNumberOfMeshes(int nMeshes)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetNumberOfMeshes " << endl;
#endif

  this->ArrayIds.clear();
  this->ArrayIds.resize(nMeshes);
  this->ExpressionIds.clear();
  this->ExpressionIds.resize(nMeshes);
  this->DomainSSIds.clear();
  this->DomainSSIds.resize(nMeshes);
  this->BlockSSIds.clear();
  this->BlockSSIds.resize(nMeshes);
  this->AssemblySSIds.clear();
  this->AssemblySSIds.resize(nMeshes);
  this->MaterialSSIds.clear();
  this->MaterialSSIds.resize(nMeshes);
  this->SpeciesSSIds.clear();
  this->SpeciesSSIds.resize(nMeshes);
  this->Modified();
}


//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetMeshIds(int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetMeshIds " <<  endl;
#endif

  const int n=ids[0];
  if (n>1)
    {
    this->MeshIds.assign(ids+1,ids+n);
    }
  else
    {
    this->MeshIds.clear();
    }
  this->SetNumberOfMeshes(n-1);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetMeshIds(int n, int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetMeshIds " << endl;
#endif

  if (n)
    {
    this->MeshIds.assign(ids,ids+n);
    }
  else
    {
    this->MeshIds.clear();
    }
  this->SetNumberOfMeshes(n);
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetArrayIds(int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetArrayIds " << endl;
#endif

  const size_t n=ids[0];
  const size_t meshId=ids[1];
  if (n>2 && meshId>=0 && meshId<this->ArrayIds.size())
    {
    this->ArrayIds[meshId].assign(ids+2,ids+n);
    }
  else
    {
    this->ArrayIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetArrayIds(int meshId, int n, int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetArrayIds " << endl;
#endif

  if (n>0 && meshId>=0 && meshId<this->ArrayIds.size())
    {
    this->ArrayIds[meshId].assign(ids,ids+n);
    }
  else
    {
    this->ArrayIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetExpressionIds(int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetExpressionIds " << endl;
#endif

  const size_t n=ids[0];
  const size_t meshId=ids[1];
  if (n>2 && meshId>=0 && meshId<this->ExpressionIds.size())
    {
    this->ExpressionIds[meshId].assign(ids+2,ids+n);
    }
  else
    {
    this->ExpressionIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetExpressionIds(int meshId, int n, int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetExpressionIds " << endl;
#endif

  if (n>0 && meshId>=0 && meshId<this->ExpressionIds.size())
    {
    this->ExpressionIds[meshId].assign(ids,ids+n);
    }
  else
    {
    this->ExpressionIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetDomainSSIds(int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetDomainSSIds " << endl;
#endif

  const size_t n=ids[0];
  const size_t meshId=ids[1];
  if (n>2 && meshId>=0 && meshId<this->DomainSSIds.size())
    {
    this->DomainSSIds[meshId].assign(ids+2,ids+n);
    }
  else
    {
    this->DomainSSIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetDomainSSIds(int meshId, int n, int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetDomainSSIds " << endl;
#endif

  if (n>0 && meshId>=0 && meshId<this->DomainSSIds.size())
    {
    this->DomainSSIds[meshId].assign(ids,ids+n);
    }
  else
    {
    this->DomainSSIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetBlockSSIds(int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetBlockSSIds " << endl;
#endif

  const size_t n=ids[0];
  const size_t meshId=ids[1];
  if (n>2 && meshId>=0 && meshId<this->BlockSSIds.size())
    {
    this->BlockSSIds[meshId].assign(ids+2,ids+n);
    }
  else
    {
    this->BlockSSIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetBlockSSIds(int meshId, int n, int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetBlockSSIds " << endl;
#endif

  if (n>0 && meshId>=0 && meshId<this->BlockSSIds.size())
    {
    this->BlockSSIds[meshId].assign(ids,ids+n);
    }
  else
    {
    this->BlockSSIds[meshId].clear();
    }
  this->Modified();
}


//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetAssemblySSIds(int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetAssemblySSIds " << endl;
#endif

  const size_t n=ids[0];
  const size_t meshId=ids[1];
  if (n>2 && meshId>=0 && meshId<this->AssemblySSIds.size())
    {
    this->AssemblySSIds[meshId].assign(ids+2,ids+n);
    }
  else
    {
    this->AssemblySSIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetAssemblySSIds(int meshId, int n, int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetAssemblySSIds " << endl;
#endif

  if (n>0 && meshId>=0 && meshId<this->AssemblySSIds.size())
    {
    this->AssemblySSIds[meshId].assign(ids,ids+n);
    }
  else
    {
    this->AssemblySSIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetMaterialSSIds(int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetMaterialSSIds " << endl;
#endif

  const size_t n=ids[0];
  const size_t meshId=ids[1];
  if (n>2 && meshId>=0 && meshId<this->MaterialSSIds.size())
    {
    this->MaterialSSIds[meshId].assign(ids+2,ids+n);
    }
  else
    {
    this->MaterialSSIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetMaterialSSIds(int meshId, int n, int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetMaterialSSIds " << endl;
#endif

  if (n>0 && meshId>=0 && meshId<this->MaterialSSIds.size())
    {
    this->MaterialSSIds[meshId].assign(ids,ids+n);
    }
  else
    {
    this->MaterialSSIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetSpeciesSSIds(int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetSpeciesSSIds " << endl;
#endif

  const size_t n=ids[0];
  const size_t meshId=ids[1];
  if (n>2 && meshId>=0 && meshId<this->SpeciesSSIds.size())
    {
    this->SpeciesSSIds[meshId].assign(ids+2,ids+n);
    }
  else
    {
    this->SpeciesSSIds[meshId].clear();
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetSpeciesSSIds(int meshId, int n, int *ids)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "====================================================================SetSpeciesSSIds " << endl;
#endif

  if (n>0 && meshId>=0 && meshId<this->SpeciesSSIds.size())
    {
    this->SpeciesSSIds[meshId].assign(ids,ids+n);
    }
  else
    {
    this->SpeciesSSIds[meshId].clear();
    }
  this->Modified();
}





//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetDomainToBlockSetOperation(int op)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "SetDomainToBlockSetOperation " << op << endl;
#endif
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetDomainToAssemblySetOperation(int op)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "SetDomainToAssemblyOperation " << op << endl;
#endif
}

//-----------------------------------------------------------------------------
void vtkVisItDatabaseBridge::SetDomainToMaterialSetOperation(int op)
{
#if defined vtkVisItDatabaseBridgeDEBUG
  cerr << "SetDomainToMaterialOperation " << op << endl;
#endif
}
