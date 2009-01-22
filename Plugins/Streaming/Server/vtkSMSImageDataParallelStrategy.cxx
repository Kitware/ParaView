/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSImageDataParallelStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSImageDataParallelStrategy.h"

#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIceTMultiDisplayRenderViewProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVInformation.h"
#include "vtkSMProxyProperty.h"
#include "vtkStreamingOptions.h"

vtkStandardNewMacro(vtkSMSImageDataParallelStrategy);
vtkCxxRevisionMacro(vtkSMSImageDataParallelStrategy, "1.8");
//----------------------------------------------------------------------------
vtkSMSImageDataParallelStrategy::vtkSMSImageDataParallelStrategy()
{
}

//----------------------------------------------------------------------------
vtkSMSImageDataParallelStrategy::~vtkSMSImageDataParallelStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMSImageDataParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
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
void vtkSMSImageDataParallelStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  //replace all of UpdateSuppressorProxies with StreamingUpdateSuppressorProxies
  ReplaceSubProxy(this->UpdateSuppressor,"StreamingUpdateSuppressor");
  ReplaceSubProxy(this->UpdateSuppressorLOD,"StreamingUpdateSuppressorLOD");
  ReplaceSubProxy(this->PostCollectUpdateSuppressor,"StreamingPostCollectUpdateSuppressor");
  ReplaceSubProxy(this->PostCollectUpdateSuppressorLOD,"StreamingPostCollectUpdateSuppressorLOD");

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
void vtkSMSImageDataParallelStrategy::CreatePipeline(vtkSMSourceProxy* input, int outputport)
{
  //turn off caching for animation it will interfere with streaming
  vtkSMSourceProxy *cacher =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("CacheKeeper"));
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    cacher->GetProperty("CachingEnabled"));
  ivp->SetElement(0, 0);

  this->Connect(input, this->ViewSorter);//, "Input", outputport);
  this->Connect(this->ViewSorter, this->PieceCache);
  this->Superclass::CreatePipeline(this->PieceCache, outputport);
  //input->VS->PCache->Collect->US
  //                 \>PostCollectUS

  //use streams instead of a proxy property here 
  //so that proxyproperty dependencies are not invoked 
  //otherwise they end up with an artificial loop
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << this->PostCollectUpdateSuppressor->GetID()
         << "SetMPIMoveData" 
         << this->Collect->GetID()
         << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::CLIENT_AND_SERVERS,
                 stream);

  // Do not supress any updates in the intermediate US's between
  // data and display. We need them to get piece selection back.
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("Enabled"));
  ivp->SetElement(0, 0);
  this->UpdateSuppressor->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMSImageDataParallelStrategy::CreateLODPipeline(vtkSMSourceProxy* input, int outputport)
{
  this->Connect(input, this->ViewSorter);
  this->Connect(this->ViewSorter, this->PieceCache);
  this->Superclass::CreateLODPipeline(this->PieceCache, outputport);
  //input->VS->PCache->LODDec->CollectLOD->USLOD
  //                         \>PostCollectUSLOD
}

//----------------------------------------------------------------------------
void vtkSMSImageDataParallelStrategy::SetPassNumber(int val, int force)
{
  int nPasses = vtkStreamingOptions::GetStreamedPasses();
  vtkSMIntVectorProperty* ivp;
  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PostCollectUpdateSuppressor->GetProperty("PassNumber"));
  ivp->SetElement(0, val);
  ivp->SetElement(1, nPasses);
  if (force)
    {
    ivp->Modified();
    this->PostCollectUpdateSuppressor->UpdateVTKObjects(); 
    vtkSMProperty *p = this->PostCollectUpdateSuppressor->GetProperty("ForceUpdate");
    p->Modified();
    this->PostCollectUpdateSuppressor->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
int vtkSMSImageDataParallelStrategy::ComputePriorities()
{
  int nPasses = vtkStreamingOptions::GetStreamedPasses();
  int ret = nPasses;

  vtkSMIntVectorProperty* ivp;

  //put diagnostic settings transfer here in case info not gathered yet
  int cacheLimit = vtkStreamingOptions::GetPieceCacheLimit();
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PieceCache->GetProperty("SetCacheSize"));
  ivp->SetElement(0, cacheLimit);
  this->PieceCache->UpdateVTKObjects();

  //Note: Parallel Strategy has to use the PostCollectUS, because that
  //is has access to the data server's pipeline, which can compute the
  //priority.

  //let US know NumberOfPasses for CP
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("SetNumberOfPasses"));
  ivp->SetElement(0, nPasses); 

  this->UpdateSuppressor->UpdateVTKObjects();

  //ask it to compute the priorities
  vtkSMProperty* cp = 
    this->UpdateSuppressor->GetProperty("ComputePriorities");
  vtkSMIntVectorProperty* rp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("GetMaxPass"));
  cp->Modified();
  this->UpdateSuppressor->UpdateVTKObjects();      
  //get the result
  this->UpdateSuppressor->UpdatePropertyInformation(rp);
  ret = rp->GetElement(0);

  //now that we've computed the priority and piece ordering, share that
  //with the other UpdateSuppressors to keep them all in synch.
  vtkSMSourceProxy *pcUS = this->PostCollectUpdateSuppressor;

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;
  this->CopyPieceList(&stream, this->UpdateSuppressor, pcUS);

  //now gather list from server to client
  vtkClientServerStream s2c;
  s2c << vtkClientServerStream::Invoke
      << this->UpdateSuppressor->GetID()
      << "SerializePriorities" 
      << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::DATA_SERVER_ROOT,
                 s2c);
  //TODO: Find another way to get this. As I recall the info helper has
  //limited length.
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("SerializedList"));
  this->UpdateSuppressor->UpdatePropertyInformation(dvp);
  int np = dvp->GetNumberOfElements();
  double *elems = dvp->GetElements();
  vtkClientServerStream s3c;
  s3c << vtkClientServerStream::Invoke
      << this->PostCollectUpdateSuppressor->GetID()
      << "UnSerializePriorities"
      << vtkClientServerStream::InsertArray(elems, np)
      << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::CLIENT,
                 s3c);  

  //now copy to the LOD pipeline
  vtkSMSourceProxy *pcUSLOD = this->PostCollectUpdateSuppressorLOD;
  vtkSMSourceProxy *uSLOD = this->UpdateSuppressorLOD;
  //False means don't do a shallow copy. 
  //Relic from when cached dataobjects were in the piecelist. Might be 
  //removable now.
  this->CopyPieceList(&stream, this->UpdateSuppressor, pcUSLOD);
  this->CopyPieceList(&stream, pcUSLOD, uSLOD);

  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::SERVERS,
                 stream);

  return ret;
}

//----------------------------------------------------------------------------
void vtkSMSImageDataParallelStrategy::ClearStreamCache()
{
  vtkSMProperty *cc = this->PieceCache->GetProperty("EmptyCache");
  cc->Modified();
  this->PieceCache->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMSImageDataParallelStrategy::CopyPieceList(
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
void vtkSMSImageDataParallelStrategy::SharePieceList(
   vtkSMRepresentationStrategy *destination)
{
  vtkSMSImageDataParallelStrategy *dest = 
    vtkSMSImageDataParallelStrategy::SafeDownCast(destination);

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();

  vtkSMSourceProxy *US1 = this->PostCollectUpdateSuppressor;

  vtkSMSourceProxy *US2 =
    vtkSMSourceProxy::SafeDownCast(
      dest->GetSubProxy("PostCollectUpdateSuppressor"));

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
void vtkSMSImageDataParallelStrategy::GatherInformation(vtkPVInformation* info)
{
  //gather information in multiple passes so as never to request
  //everything at once.

  vtkSMIntVectorProperty* ivp;

  //put diagnostic setting transfer here because this happens early
  int cacheLimit = vtkStreamingOptions::GetPieceCacheLimit();
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PieceCache->GetProperty("SetCacheSize"));
  ivp->SetElement(0, cacheLimit);
  this->PieceCache->UpdateVTKObjects();

  //let US know NumberOfPasses for CP
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("SetNumberOfPasses"));
  int nPasses = vtkStreamingOptions::GetStreamedPasses();
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
      this->PostCollectUpdateSuppressor->GetProperty("PassNumber"));
    ivp->SetElement(0, i);
    ivp->SetElement(1, nPasses);

    this->UpdateSuppressor->UpdateVTKObjects();
    this->UpdateSuppressor->InvokeCommand("ForceUpdate");

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->GatherInformation(this->ConnectionID,
                          vtkProcessModule::DATA_SERVER_ROOT,
                          sinfo,
                          this->PostCollectUpdateSuppressor->GetID());
    info->AddInformation(sinfo);
    sinfo->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMSImageDataParallelStrategy::GatherLODInformation(vtkPVInformation* info)
{
  //gather information in multiple passes so as never to request
  //everything at once.
  int nPasses = vtkStreamingOptions::GetStreamedPasses();

  for (int i = 0; i < 1; i++)
    {
    vtkPVInformation *sinfo = 
      vtkPVInformation::SafeDownCast(info->NewInstance());
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->UpdateSuppressorLOD->GetProperty("PieceNumber"));
    ivp->SetElement(0, i);
    ivp->SetElement(1, nPasses);

    this->UpdateSuppressorLOD->UpdateVTKObjects();
    this->UpdateSuppressorLOD->InvokeCommand("ForceUpdate");

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->GatherInformation(this->ConnectionID,
                          vtkProcessModule::DATA_SERVER_ROOT,
                          sinfo,
                          this->UpdateSuppressorLOD->GetID());
    info->AddInformation(sinfo);
    sinfo->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMSImageDataParallelStrategy::InvalidatePipeline()
{
  // Cache is cleaned up whenever something changes and caching is not currently
  // enabled.
  if (this->PostCollectUpdateSuppressor)
    {
    this->PostCollectUpdateSuppressor->InvokeCommand("ClearPriorities");
    }
  this->Superclass::InvalidatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMSImageDataParallelStrategy::SetViewState(double *camera, double *frustum)
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
