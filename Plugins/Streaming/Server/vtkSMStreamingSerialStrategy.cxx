/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingSerialStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStreamingSerialStrategy.h"
#include "vtkStreamingOptions.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataSizeInformation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyProperty.h"

#define DEBUGPRINT_STRATEGY(arg)\
  if (vtkStreamingOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

vtkStandardNewMacro(vtkSMStreamingSerialStrategy);
vtkCxxRevisionMacro(vtkSMStreamingSerialStrategy, "1.4");
//----------------------------------------------------------------------------
vtkSMStreamingSerialStrategy::vtkSMStreamingSerialStrategy()
{
}

//----------------------------------------------------------------------------
vtkSMStreamingSerialStrategy::~vtkSMStreamingSerialStrategy()
{
}

//----------------------------------------------------------------------------
#define ReplaceSubProxy(orig, name) \
{\
  vtkTypeUInt32 servers = orig->GetServers(); \
  orig = vtkSMSourceProxy::SafeDownCast(this->GetSubProxy(name));\
  orig->SetServers(servers);\
}

//----------------------------------------------------------------------------
void vtkSMStreamingSerialStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  //replace all of UpdateSuppressorProxies with StreamingUpdateSuppressorProxies
  ReplaceSubProxy(this->UpdateSuppressor, "StreamingUpdateSuppressor");
  ReplaceSubProxy(this->UpdateSuppressorLOD, "StreamingUpdateSuppressorLOD");

  //Get hold of the caching filter proxy
  this->PieceCache = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PieceCache"));

  //Get hold of the filter that does view dependent prioritization
  this->ViewSorter = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("ViewSorter"));
  this->ViewSorter->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
void vtkSMStreamingSerialStrategy::CreatePipeline(vtkSMSourceProxy* input, int outputport)
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
  //input->ViewSorter->PieceCache->US
}

//----------------------------------------------------------------------------
void vtkSMStreamingSerialStrategy::CreateLODPipeline(vtkSMSourceProxy* input, int outputport)
{
  this->Connect(input, this->ViewSorter);
  this->Connect(this->ViewSorter, this->PieceCache);
  this->Superclass::CreateLODPipeline(this->PieceCache, outputport);
  //input->ViewSorter->PieceCache->LODDec->USLOD
}

//----------------------------------------------------------------------------
void vtkSMStreamingSerialStrategy::GatherInformation(vtkPVInformation* info)
{
  //gather information without requesting the whole data  
  vtkSMIntVectorProperty* ivp;
  DEBUGPRINT_STRATEGY(
    cerr << "SSS(" << this << ") Gather Info" << endl;
    );

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
    vtkPVInformation *sinfo = vtkPVInformation::SafeDownCast(info->NewInstance());
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->UpdateSuppressor->GetProperty("PassNumber"));
    ivp->SetElement(0, i);
    ivp->SetElement(1, nPasses);

    this->UpdateSuppressor->UpdateVTKObjects();
    this->UpdatePipeline();
    
    // For simple strategy information sub-pipline is same as the full pipeline
    // so no data movements are involved.
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->GatherInformation(this->ConnectionID,
                          vtkProcessModule::DATA_SERVER_ROOT,
                          sinfo,
                          this->UpdateSuppressor->GetID());
    info->AddInformation(sinfo);
    sinfo->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMStreamingSerialStrategy::GatherLODInformation(vtkPVInformation* info)
{
  //gather information in multiple passes so as never to request the whole data  
  DEBUGPRINT_STRATEGY(
    cerr << "SSS(" << this << ") Gather LOD Info" << endl;
    );

  vtkSMIntVectorProperty* ivp;
  int nPasses = vtkStreamingOptions::GetStreamedPasses();
  for (int i = 0; i < 1; i++)
    {
    vtkPVInformation *sinfo = 
      vtkPVInformation::SafeDownCast(info->NewInstance());
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->UpdateSuppressorLOD->GetProperty("PassNumber"));
    ivp->SetElement(0, i);
    ivp->SetElement(1, nPasses);

    this->UpdateSuppressorLOD->UpdateVTKObjects();
    this->UpdateLODPipeline();
    
    // For simple strategy information sub-pipline is same as the full pipeline
    // so no data movements are involved.
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
void vtkSMStreamingSerialStrategy::InvalidatePipeline()
{
  // Cache is cleaned up whenever something changes and caching is not currently
  // enabled.
  this->UpdateSuppressor->InvokeCommand("ClearPriorities");
  this->Superclass::InvalidatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMStreamingSerialStrategy::SetPassNumber(int val, int force)
{
  int nPasses = vtkStreamingOptions::GetStreamedPasses();
  vtkSMIntVectorProperty* ivp;
  DEBUGPRINT_STRATEGY(
    cerr << "SSS(" << this << ") SetPassNumber(" << val << "/" << nPasses << (force?"FORCE":"LAZY") << ")" << endl;
                      );
  
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
int vtkSMStreamingSerialStrategy::ComputePriorities()
{
  int nPasses = vtkStreamingOptions::GetStreamedPasses();
  int ret = nPasses;

  vtkSMIntVectorProperty* ivp;
  
  //put diagnostic settings transfer here in case info not gathered yet
  int cacheLimit = vtkStreamingOptions::GetPieceCacheLimit();

  DEBUGPRINT_STRATEGY(
    cerr << "SSS(" << this << ") ComputePriorities" << endl;
                      )  
  ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->PieceCache->GetProperty("SetCacheSize"));
  ivp->SetElement(0, cacheLimit);
  this->PieceCache->UpdateVTKObjects();

  //let US know NumberOfPasses for CP
  ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->UpdateSuppressor->GetProperty("SetNumberOfPasses"));
  ivp->SetElement(0, nPasses); 

  this->UpdateSuppressor->UpdateVTKObjects();

  //ask it to compute the priorities
  vtkSMProperty* cp = this->UpdateSuppressor->GetProperty("ComputePriorities");
  vtkSMIntVectorProperty* rp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("GetMaxPass"));
  cp->Modified();
  this->UpdateSuppressor->UpdateVTKObjects();      
  //get the result
  this->UpdateSuppressor->UpdatePropertyInformation(rp);
  ret = rp->GetElement(0);

  vtkClientServerStream stream;
  this->CopyPieceList(&stream, this->UpdateSuppressor, this->UpdateSuppressorLOD);

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::SERVERS,
                 stream);

  return ret;
}

//----------------------------------------------------------------------------
void vtkSMStreamingSerialStrategy::ClearStreamCache()
{
  vtkSMProperty *cc = this->PieceCache->GetProperty("EmptyCache");
  cc->Modified();
  this->PieceCache->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMStreamingSerialStrategy::CopyPieceList(
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
void vtkSMStreamingSerialStrategy::SharePieceList(
   vtkSMRepresentationStrategy *destination)
{
  vtkSMStreamingSerialStrategy *dest = 
    vtkSMStreamingSerialStrategy::SafeDownCast(destination);
  if (!dest)
    {
    vtkErrorMacro("Can't copy my piecelist to that");
    return;
    }

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();

  vtkSMSourceProxy *US1 =
    this->UpdateSuppressor;

  vtkSMSourceProxy *US2 =
    vtkSMSourceProxy::SafeDownCast(
      dest->GetSubProxy("StreamingUpdateSuppressor"));

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
void vtkSMStreamingSerialStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMStreamingSerialStrategy::SetViewState(double *camera, double *frustum)
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
