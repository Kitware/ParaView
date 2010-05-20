/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveSerialStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAdaptiveSerialStrategy.h"
#include "vtkAdaptiveOptions.h"
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
  if (vtkAdaptiveOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

vtkStandardNewMacro(vtkSMAdaptiveSerialStrategy);
//----------------------------------------------------------------------------
vtkSMAdaptiveSerialStrategy::vtkSMAdaptiveSerialStrategy()
{
}

//----------------------------------------------------------------------------
vtkSMAdaptiveSerialStrategy::~vtkSMAdaptiveSerialStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::PrintSelf(ostream& os, vtkIndent indent)
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
void vtkSMAdaptiveSerialStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  //replace all of UpdateSuppressorProxies with AdaptiveUpdateSuppressorProxies
  ReplaceSubProxy(this->UpdateSuppressor, "AdaptiveUpdateSuppressor");
  ReplaceSubProxy(this->UpdateSuppressorLOD, "AdaptiveUpdateSuppressorLOD");

  //Get hold of the caching filter proxy
  this->PieceCache = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("PieceCache"));
  this->PieceCache->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);

  //Get hold of the filter that does view dependent prioritization
  this->ViewSorter = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("ViewSorter"));
  this->ViewSorter->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::CreatePipeline(vtkSMSourceProxy* input, int outputport)
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

  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("SetCacheFilter"));
  if (pp)
    {
    pp->AddProxy(this->PieceCache);
    }
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::CreateLODPipeline(vtkSMSourceProxy* input, int outputport)
{
  this->Connect(input, this->ViewSorter, "Input", outputport);
  this->Connect(this->ViewSorter, this->PieceCache);
  this->Superclass::CreateLODPipeline(this->PieceCache, 0);
  //input->ViewSorter->PieceCache->LODDec->USLOD
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::InvalidatePipeline()
{
  // Cache is cleaned up whenever something changes and caching is not currently
  // enabled.
  this->UpdateSuppressor->InvokeCommand("ClearPriorities");
  this->Superclass::InvalidatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::EmptyCache()
{
  this->PieceCache->InvokeCommand("EmptyCache");
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::ClearStreamCache()
{
  this->PieceCache->InvokeCommand("EmptyCache");
  this->UpdateSuppressor->InvokeCommand("ClearPriorities");
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::GatherInformation(vtkPVInformation* info)
{
  //gather information without requesting the whole data  
  vtkSMIntVectorProperty* ivp;
  DEBUGPRINT_STRATEGY(
    cerr << "SSS(" << this << ") Gather Info" << endl;
    );

  int cacheLimit = vtkAdaptiveOptions::GetPieceCacheLimit();
  //int useCulling = vtkAdaptiveOptions::GetUseCulling();
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PieceCache->GetProperty("SetCacheSize"));
  ivp->SetElement(0, cacheLimit);
  this->PieceCache->UpdateVTKObjects();

  vtkSMProperty *p = this->UpdateSuppressor->GetProperty("PrepareFirstPass");
  p->Modified();
  this->UpdateSuppressor->UpdateVTKObjects();
  this->UpdatePipeline();
    
  // For simple strategy information sub-pipline is same as the full pipeline
  // so no data movements are involved.

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID,
                        vtkProcessModule::DATA_SERVER_ROOT,
                        info,
                        this->UpdateSuppressor->GetID());
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::GatherLODInformation(vtkPVInformation* info)
{
  //gather information in multiple passes so as never to request the whole data  
  DEBUGPRINT_STRATEGY(
    cerr << "SSS(" << this << ") Gather LOD Info" << endl;
    );

  vtkSMProperty *p = this->UpdateSuppressor->GetProperty("PrepareFirstPass");
  p->Modified();
  this->UpdateSuppressorLOD->UpdateVTKObjects();
  this->UpdateLODPipeline();
    
  // For simple strategy information sub-pipline is same as the full pipeline
  // so no data movements are involved.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID,
                        vtkProcessModule::DATA_SERVER_ROOT,
                        info,
                        this->UpdateSuppressorLOD->GetID());
}


//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::SetViewState(double *camera, double *frustum)
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

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::PrepareFirstPass()
{
  DEBUGPRINT_STRATEGY(
    cerr << "SSS("<<this<<")::PrepareFirstPass" << endl;
    );
  int cacheLimit = vtkAdaptiveOptions::GetPieceCacheLimit();
  vtkSMIntVectorProperty* ivp;
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PieceCache->GetProperty("SetCacheSize"));
  ivp->SetElement(0, cacheLimit);
  this->PieceCache->UpdateVTKObjects();

  vtkSMProperty* cp = this->UpdateSuppressor->GetProperty("PrepareFirstPass");
  cp->Modified();
  this->UpdateSuppressor->UpdateVTKObjects();      
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::PrepareAnotherPass()
{
  DEBUGPRINT_STRATEGY(
    cerr << "SSS("<<this<<")::PrepareAnotherPass" << endl;
    );

  vtkSMProperty* cp = this->UpdateSuppressor->GetProperty("PrepareAnotherPass");
  cp->Modified();
  this->UpdateSuppressor->UpdateVTKObjects();      
}


//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::ChooseNextPiece()
{
  DEBUGPRINT_STRATEGY(
    cerr << "SSS("<<this<<")::ChooseNextPiece" << endl;
    );
  vtkSMProperty* cp = this->UpdateSuppressor->GetProperty("ChooseNextPiece");
  cp->Modified();
  this->UpdateSuppressor->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::GetPieceInfo(
  int *P, int *NP, double *R, double *PRIORITY, bool *HIT, bool *APPEND)
{
  vtkSMDoubleVectorProperty* dp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("GetPieceInfo"));
  //get the result
  this->UpdateSuppressor->UpdatePropertyInformation(dp);

  *P = (int)dp->GetElement(0);
  *NP = (int)dp->GetElement(1);
  *R = dp->GetElement(2);
  *PRIORITY = dp->GetElement(3);
  *HIT = dp->GetElement(4)!=0.0;
  *APPEND = dp->GetElement(5)!=0.0;
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::SetNextPiece(int N, int NP, double R, 
                                               double pri, bool hit, bool append)
{
  DEBUGPRINT_STRATEGY(
    cerr << "SSS("<<this<<")::SetNextPiece" << endl;
    );
  vtkSMProperty* cp = this->PieceCache->GetProperty("Silence");
  cp->Modified();
  this->PieceCache->UpdateVTKObjects();

  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("SetPieceInfo"));
  dvp->SetElement(0, (double)N);
  dvp->SetElement(1, (double)NP);
  dvp->SetElement(2, R);
  dvp->SetElement(3, pri);
  dvp->SetElement(4, (hit?1.0:0.0));
  dvp->SetElement(5, (append?1.0:0.0));

  this->UpdateSuppressor->UpdateVTKObjects();
  this->UpdatePipeline();
  this->DataValid = false;
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::GetStateInfo(
  bool *ALLDONE, bool *WENDDONE)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("GetStateInfo"));
  //get the result
  this->UpdateSuppressor->UpdatePropertyInformation(ivp);
  *ALLDONE = (ivp->GetElement(0)!=0);
  *WENDDONE = (ivp->GetElement(1)!=0);
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::FinishPass()
{
  vtkSMProperty *prop;
  prop = this->UpdateSuppressor->GetProperty("FinishPass");
  prop->Modified();
  this->UpdateSuppressor->UpdateVTKObjects();  
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::Refine()
{
  vtkSMProperty *prop;
  prop = this->UpdateSuppressor->GetProperty("Refine");
  prop->Modified();
  this->UpdateSuppressor->UpdateVTKObjects();  
}


//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::Coarsen()
{
  vtkSMProperty *prop;
  prop = this->UpdateSuppressor->GetProperty("Coarsen");
  prop->Modified();
  this->UpdateSuppressor->UpdateVTKObjects();  
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::SetMaxDepth(int maxD)
{
  vtkSMIntVectorProperty* vp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("SetMaxDepth"));
  vp->SetElement(0, maxD);
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveSerialStrategy::GetMaxDepth(int *maxD)
{
  vtkSMIntVectorProperty* vp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("SetMaxDepth"));
  *maxD = vp->GetElement(0);
}

