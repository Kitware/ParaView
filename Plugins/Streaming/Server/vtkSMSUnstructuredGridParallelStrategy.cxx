/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSUnstructuredGridParallelStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSUnstructuredGridParallelStrategy.h"
#include "vtkSMStreamingHelperProxy.h"
#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIceTMultiDisplayRenderViewProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkPVInformation.h"

vtkStandardNewMacro(vtkSMSUnstructuredGridParallelStrategy);
vtkCxxRevisionMacro(vtkSMSUnstructuredGridParallelStrategy, "1.3");
//----------------------------------------------------------------------------
vtkSMSUnstructuredGridParallelStrategy::vtkSMSUnstructuredGridParallelStrategy()
{
}

//----------------------------------------------------------------------------
vtkSMSUnstructuredGridParallelStrategy::~vtkSMSUnstructuredGridParallelStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
#define ReplaceSubProxy(orig, name) \
{\
  vtkTypeUInt32 servers = orig->GetServers(); \
  orig = vtkSMSourceProxy::SafeDownCast(this->GetSubProxy(name));\
  orig->SetServers(servers);\
}

//----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  //replace all of UpdateSuppressorProxies with StreamingUpdateSuppressorProxies
  ReplaceSubProxy(this->UpdateSuppressor,"StreamingUpdateSuppressor");
  ReplaceSubProxy(this->UpdateSuppressorLOD, "StreamingUpdateSuppressorLOD");
  ReplaceSubProxy(this->PreCollectUpdateSuppressor,"StreamingPreCollectUpdateSuppressor");
  ReplaceSubProxy(this->PreCollectUpdateSuppressorLOD, "StreamingPreCollectUpdateSuppressorLOD");
  ReplaceSubProxy(this->PreDistributorSuppressor, "StreamingPreDistributorSuppressor");
  ReplaceSubProxy(this->PreDistributorSuppressorLOD,"StreamingPreDistributorSuppressorLOD");

  //Get hold of the caching filter proxy
  this->PieceCache = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PieceCache"));
  this->PieceCache->SetServers(vtkProcessModule::DATA_SERVER);

  //Get hold of the filter that does view dependent prioritization
  this->ViewSorter = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("ViewSorter"));
  this->ViewSorter->SetServers(vtkProcessModule::DATA_SERVER);
}

//----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::CreatePipeline(vtkSMSourceProxy* input, int outputport)
{
  //turn off caching for animation it will interfere with streaming
  vtkSMSourceProxy *cacher =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("CacheKeeper"));
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    cacher->GetProperty("CachingEnabled"));
  ivp->SetElement(0, 0);

  this->Connect(input, this->ViewSorter);
  this->Connect(this->ViewSorter, this->PieceCache);
  this->Superclass::CreatePipeline(this->PieceCache, outputport);
  //input->ViewSorter->PieceCache->Collect(UGRID)->PreDistUS->Distr
  //                             |                          \>US
  //                             \>PreCollectUS

  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("SetMPIMoveData"));
  if (pp)
    {
    pp->AddProxy(this->Collect);
    }
}

//----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::CreateLODPipeline(vtkSMSourceProxy* input, int outputport)
{
  this->Connect(input, this->ViewSorter);
  this->Connect(this->ViewSorter, this->PieceCache);
  this->Superclass::CreateLODPipeline(this->PieceCache, outputport);
  //input->VS->PC->LODDec->CollectLOD(POLY)->PreDistUSLOD->DistrLOD
  //                     |                               \>USLOD
  //                     \>PreCollectUSLOD
}

//----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::SetPassNumber(int val, int force)
{
  int nPasses = vtkSMStreamingHelperProxy::GetHelper()->GetStreamedPasses();
  vtkSMIntVectorProperty* ivp;
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("PassNumber"));
  ivp->SetElement(0, val);
  ivp->SetElement(1, nPasses);
  if (force)
    {
    ivp->Modified();
    this->UpdateSuppressor->UpdateVTKObjects(); 
    vtkSMProperty *p = this->UpdateSuppressor->GetProperty("ForceUpdate");
    p->Modified();
    this->UpdateSuppressor->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
int vtkSMSUnstructuredGridParallelStrategy::ComputePriorities()
{
  int nPasses = vtkSMStreamingHelperProxy::GetHelper()->GetStreamedPasses();
  int ret = nPasses;

  vtkSMIntVectorProperty* ivp;

  //put diagnostic settings transfer here in case info not gathered yet
  int doPrints = vtkSMStreamingHelperProxy::GetHelper()->GetEnableStreamMessages();
  int cacheLimit = vtkSMStreamingHelperProxy::GetHelper()->GetPieceCacheLimit();
  int useCulling = vtkSMStreamingHelperProxy::GetHelper()->GetUseCulling();
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PieceCache->GetProperty("EnableStreamMessages"));
  ivp->SetElement(0, doPrints);
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PieceCache->GetProperty("SetCacheSize"));
  ivp->SetElement(0, cacheLimit);
  this->PieceCache->UpdateVTKObjects();
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("EnableStreamMessages"));
  ivp->SetElement(0, doPrints);
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("UseCulling"));
  ivp->SetElement(0, useCulling);

  //Note: Parallel Strategy has to use the PreCollectUS, because that
  //is has access to the data server's pipeline, which can compute the
  //priority.

  //let US know NumberOfPasses for CP
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PreCollectUpdateSuppressor->GetProperty("SetNumberOfPasses"));
  ivp->SetElement(0, nPasses); 

  this->PreCollectUpdateSuppressor->UpdateVTKObjects();

  //ask it to compute the priorities
  vtkSMProperty* cp = 
    this->PreCollectUpdateSuppressor->GetProperty("ComputePriorities");
  vtkSMIntVectorProperty* rp = vtkSMIntVectorProperty::SafeDownCast(
    this->PreCollectUpdateSuppressor->GetProperty("GetMaxPass"));
  cp->Modified();
  this->PreCollectUpdateSuppressor->UpdateVTKObjects();      
  //get the result
  this->PreCollectUpdateSuppressor->UpdatePropertyInformation(rp);
  ret = rp->GetElement(0);

  //now that we've computed the priority and piece ordering, share that
  //with the other UpdateSuppressors to keep them all in synch.
  vtkSMSourceProxy *pdUS = this->PreDistributorSuppressor;
  vtkSMSourceProxy *uS = this->UpdateSuppressor;

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;
  this->CopyPieceList(&stream, this->PreCollectUpdateSuppressor, pdUS);
  this->CopyPieceList(&stream, this->PreCollectUpdateSuppressor, uS);

  //now gather list from server to client
  vtkClientServerStream s2c;
  s2c << vtkClientServerStream::Invoke
      << this->PreCollectUpdateSuppressor->GetID()
      << "SerializePriorities" 
      << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::DATA_SERVER_ROOT,
                 s2c);
  //TODO: Find another way to get this. As I recall the info helper has
  //limited length.
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->PreCollectUpdateSuppressor->GetProperty("SerializedList"));
  this->PreCollectUpdateSuppressor->UpdatePropertyInformation(dvp);
  int np = dvp->GetNumberOfElements();
  double *elems = dvp->GetElements();
  vtkClientServerStream s3c;
  s3c << vtkClientServerStream::Invoke
      << this->UpdateSuppressor->GetID()
      << "UnSerializePriorities"
      << vtkClientServerStream::InsertArray(elems, np)
      << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::CLIENT,
                 s3c);  

  //now copy to the LOD pipeline
  vtkSMSourceProxy *pcUSLOD = this->PreCollectUpdateSuppressorLOD;
  vtkSMSourceProxy *pdUSLOD = this->PreDistributorSuppressorLOD;
  vtkSMSourceProxy *uSLOD = this->UpdateSuppressorLOD;
  //False means don't do a shallow copy. 
  //Relic from when cached dataobjects were in the piecelist. Might be 
  //removable now.
  this->CopyPieceList(&stream, this->PreCollectUpdateSuppressor, pcUSLOD);
  this->CopyPieceList(&stream, pcUSLOD, pdUSLOD);
  this->CopyPieceList(&stream, pcUSLOD, uSLOD);

  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::SERVERS,
                 stream);

  return ret;
}

//----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::ClearStreamCache()
{
  vtkSMProperty *cc = this->PieceCache->GetProperty("EmptyCache");
  cc->Modified();
  this->PieceCache->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::CopyPieceList(
  vtkClientServerStream *stream,
  vtkSMSourceProxy *src, vtkSMSourceProxy *dest)
{
  if (src && dest)
    {
    (*stream) 
      << vtkClientServerStream::Invoke
      << src->GetID()
      << "GetPieceList"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << dest->GetID()
      << "SetPieceList"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    }
}

//----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::SharePieceList(
   vtkSMRepresentationStrategy *destination)
{
  vtkSMSUnstructuredGridParallelStrategy *dest = 
    vtkSMSUnstructuredGridParallelStrategy::SafeDownCast(destination);

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();

  vtkSMSourceProxy *US1 = this->UpdateSuppressor;

  vtkSMSourceProxy *US2 =
    vtkSMSourceProxy::SafeDownCast(
      dest->GetSubProxy("UpdateSuppressor"));

  vtkClientServerStream s2c;
  s2c << vtkClientServerStream::Invoke
      << US1->GetID()
      << "SerializePriorities" 
      << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::DATA_SERVER_ROOT,
                 s2c);
  
  //TODO: Find another way to get this. As I recall the info helper has
  //limited length.
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    US1->GetProperty("SerializedList"));
  US1->UpdatePropertyInformation(dvp);
  int np = dvp->GetNumberOfElements();
  
  //cerr << "US1 " << US1 << " ";
  //cerr << "US2 " << US2 << " ";
  //cerr << "NP " << np << endl;

  if (!np)
    {
    return;
    }
  double *elems = dvp->GetElements();
  vtkClientServerStream s3c;
  s3c << vtkClientServerStream::Invoke
      << US2->GetID()
      << "UnSerializePriorities"
      << vtkClientServerStream::InsertArray(elems, np)
      << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::CLIENT,
                 s3c);  
}

//----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::GatherInformation(vtkPVInformation* info)
{
  //gather information in multiple passes so as never to request
  //everything at once.

  vtkSMIntVectorProperty* ivp;

  int doPrints = vtkSMStreamingHelperProxy::GetHelper()->GetEnableStreamMessages();
  if (doPrints)
    {
    cerr << "SParStrat(" << this << ") Gather Info" << endl;
    }

  //put diagnostic setting transfer here because this happens early
  int cacheLimit = vtkSMStreamingHelperProxy::GetHelper()->GetPieceCacheLimit();
  //int useCulling = vtkSMStreamingHelperProxy::GetHelper()->GetUseCulling();
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PieceCache->GetProperty("EnableStreamMessages"));
  ivp->SetElement(0, doPrints);
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PieceCache->GetProperty("SetCacheSize"));
  ivp->SetElement(0, cacheLimit);
  this->PieceCache->UpdateVTKObjects();
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("EnableStreamMessages"));
  ivp->SetElement(0, doPrints);
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("UseCulling"));
  ivp->SetElement(0, 0);//useCulling);

  //let US know NumberOfPasses for CP
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("SetNumberOfPasses"));
  int nPasses = vtkSMStreamingHelperProxy::GetHelper()->GetStreamedPasses();
  ivp->SetElement(0, nPasses); 

  this->UpdateSuppressor->UpdateVTKObjects();
  vtkSMProperty *p = this->UpdateSuppressor->GetProperty("ComputePriorities");
  p->Modified();
  this->UpdateSuppressor->UpdateVTKObjects();

  for (int i = 0; i < 1; i++)
    {
    vtkPVInformation *sinfo = 
      vtkPVInformation::SafeDownCast(info->NewInstance());
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->PreCollectUpdateSuppressor->GetProperty("PassNumber"));
    ivp->SetElement(0, i);
    ivp->SetElement(1, nPasses);

    this->PreCollectUpdateSuppressor->UpdateVTKObjects();
    this->PreCollectUpdateSuppressor->InvokeCommand("ForceUpdate");

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->GatherInformation(this->ConnectionID,
                          vtkProcessModule::DATA_SERVER_ROOT,
                          sinfo,
                          this->PreCollectUpdateSuppressor->GetID());
    info->AddInformation(sinfo);
    sinfo->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::GatherLODInformation(vtkPVInformation* info)
{
  //gather information in multiple passes so as never to request
  //everything at once.
  int nPasses = vtkSMStreamingHelperProxy::GetHelper()->GetStreamedPasses();
  int doPrints = vtkSMStreamingHelperProxy::GetHelper()->GetEnableStreamMessages();
  if (doPrints)
    {
    cerr << "SParStrat(" << this << ") Gather LOD Info" << endl;
    }

  for (int i = 0; i < 1; i++)
    {
    vtkPVInformation *sinfo = 
      vtkPVInformation::SafeDownCast(info->NewInstance());
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->PreCollectUpdateSuppressorLOD->GetProperty("PieceNumber"));
    ivp->SetElement(0, i);
    ivp->SetElement(1, nPasses);

    this->PreCollectUpdateSuppressorLOD->UpdateVTKObjects();
    this->PreCollectUpdateSuppressorLOD->InvokeCommand("ForceUpdate");

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->GatherInformation(this->ConnectionID,
                          vtkProcessModule::DATA_SERVER_ROOT,
                          sinfo,
                          this->PreCollectUpdateSuppressorLOD->GetID());
    info->AddInformation(sinfo);
    sinfo->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::InvalidatePipeline()
{
  // Cache is cleaned up whenever something changes and caching is not currently
  // enabled.
  if (this->UpdateSuppressor)
    {
    this->UpdateSuppressor->InvokeCommand("ClearPriorities");
    }
  this->Superclass::InvalidatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMSUnstructuredGridParallelStrategy::SetViewState(double *camera, double *frustum)
{
  if (!camera || !frustum)
    {
    return;
    }

  vtkSMDoubleVectorProperty* dvp;
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ViewSorter->GetProperty("SetCamera"));
  dvp->SetElements(camera);
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->ViewSorter->GetProperty("SetFrustum"));
  dvp->SetElements(frustum);
  this->ViewSorter->UpdateVTKObjects();      
}
