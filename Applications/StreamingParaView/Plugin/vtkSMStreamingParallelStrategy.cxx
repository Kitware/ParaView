/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingParallelStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStreamingParallelStrategy.h"
#include "vtkStreamingOptions.h"

#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkMPIMoveData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVInformation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIceTMultiDisplayRenderViewProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMStreamingParallelStrategy);
vtkCxxRevisionMacro(vtkSMStreamingParallelStrategy, "1.2");

//----------------------------------------------------------------------------
vtkSMStreamingParallelStrategy::vtkSMStreamingParallelStrategy()
{
  this->EnableCaching = false; //don't try to use animation caching
  this->SetEnableLOD(false); //don't try to have LOD while interacting either
}

//----------------------------------------------------------------------------
vtkSMStreamingParallelStrategy::~vtkSMStreamingParallelStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMStreamingParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMStreamingParallelStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

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
void vtkSMStreamingParallelStrategy::CreatePipeline(vtkSMSourceProxy* input, int outputport)
{
  vtkSMIntVectorProperty *ivp;

  //turn off caching for animation it will interfere with streaming
  vtkSMSourceProxy *cacher =
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("CacheKeeper"));
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    cacher->GetProperty("CachingEnabled"));
  ivp->SetElement(0, 0);

  this->Connect(input, this->ViewSorter, "Input", outputport);
  this->Connect(this->ViewSorter, this->PieceCache);
  this->Superclass::CreatePipeline(this->PieceCache, 0);
  //input->VS->PC->US->Collect->PostCollectUS->Distr->pdUS

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if (pm->GetNumberOfPartitions(this->GetConnectionID())>1)
    {
    //use streams instead of a proxy property here 
    //so that proxyproperty dependencies are not invoked 
    //otherwise they end up with an artificial loop
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
           << this->PostDistributorSuppressor->GetID()
           << "SetMPIMoveData" 
           << this->Collect->GetID()
           << vtkClientServerStream::End;
    pm->SendStream(this->GetConnectionID(),
                   vtkProcessModule::CLIENT_AND_SERVERS,
                   stream);
    }

  // Do not supress any updates in the intermediate US's between
  // data and display. We need them to get piece selection back.
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PostCollectUpdateSuppressor->GetProperty("Enabled"));
  ivp->SetElement(0, 0);
  this->PostCollectUpdateSuppressor->UpdateVTKObjects();
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("Enabled"));
  ivp->SetElement(0, 0);
  this->UpdateSuppressor->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMStreamingParallelStrategy::SetPassNumber(int val, int force)
{
  int nPasses = vtkStreamingOptions::GetStreamedPasses();
//  cerr << "SPS(" << this << ") SetPassNumber " << val << "/" << nPasses << " " << (force?"FORCE":"LAZY") << endl;

  vtkSMIntVectorProperty* ivp;
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->PostDistributorSuppressor->GetProperty("PassNumber"));
  ivp->SetElement(0, val);
  ivp->SetElement(1, nPasses);
  if (force)
    {
    ivp->Modified();
    this->PostDistributorSuppressor->UpdateVTKObjects(); 
    vtkSMProperty *p = this->PostDistributorSuppressor->GetProperty("ForceUpdate");
    p->Modified();
    this->PostDistributorSuppressor->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
int vtkSMStreamingParallelStrategy::ComputePriorities()
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
  vtkSMSourceProxy *pdUS = this->PostDistributorSuppressor;


  vtkClientServerStream stream;
  this->CopyPieceList(&stream, this->UpdateSuppressor, pcUS);
  this->CopyPieceList(&stream, this->UpdateSuppressor, pdUS);

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::SERVERS,
                 stream);

  //now gather list from server to client
  vtkClientServerStream s2c;
  s2c << vtkClientServerStream::Invoke
      << this->UpdateSuppressor->GetID()
      << "SerializePriorities" 
      << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::DATA_SERVER_ROOT,
                 s2c);
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->UpdateSuppressor->GetProperty("SerializedList"));
  this->UpdateSuppressor->UpdatePropertyInformation(dvp);
  int np = dvp->GetNumberOfElements();
  double *elems = dvp->GetElements();
/*
  cerr << "SPS(" << this << ") Obtained list " << np << ":";
  for (int i = 0; i < np; i++)
    {
    cerr << elems[i] << " ";
    }
  cerr << endl;
*/
  vtkClientServerStream s3c;
  s3c << vtkClientServerStream::Invoke
      << this->PostDistributorSuppressor->GetID()
      << "UnSerializePriorities"
      << vtkClientServerStream::InsertArray(elems, np)
      << vtkClientServerStream::End;
  pm->SendStream(this->GetConnectionID(),
                 vtkProcessModule::CLIENT,
                 s3c);  

  return ret;
}

//----------------------------------------------------------------------------
void vtkSMStreamingParallelStrategy::ClearStreamCache()
{
  vtkSMProperty *cc = this->PieceCache->GetProperty("EmptyCache");
  cc->Modified();
  this->PieceCache->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMStreamingParallelStrategy::CopyPieceList(
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
void vtkSMStreamingParallelStrategy::SharePieceList(
   vtkSMRepresentationStrategy *destination)
{
  vtkSMStreamingParallelStrategy *dest = 
    vtkSMStreamingParallelStrategy::SafeDownCast(destination);

  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();

  vtkSMSourceProxy *US1 = this->PostDistributorSuppressor;

  vtkSMSourceProxy *US2 =
    vtkSMSourceProxy::SafeDownCast(
      dest->GetSubProxy("PostDistributorSuppressor"));

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
void vtkSMStreamingParallelStrategy::GatherInformation(vtkPVInformation* info)
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
      this->UpdateSuppressor->GetProperty("PassNumber"));
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
void vtkSMStreamingParallelStrategy::InvalidatePipeline()
{
  // Cache is cleaned up whenever something changes and caching is not currently
  // enabled.
  if (this->PostDistributorSuppressor)
    {
    this->PostDistributorSuppressor->InvokeCommand("ClearPriorities");
    }
  this->Superclass::InvalidatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMStreamingParallelStrategy::SetViewState(double *camera, double *frustum)
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
void vtkSMStreamingParallelStrategy::UpdatePipeline()
{
  //I have inlined the code that normally happens in the parent classes.
  //Each parent class checks locally if data is valid and if not calls 
  //superclass Update before updating the parts of the pipeline it is 
  //responsible for. Doing it that way squeezes the data downward along
  //the intestine, as it were, toward the client.
  //
  //I had to do all this here just to bypass the blockage that
  //streamingoutputport makes on the MPIMoveData filter.

  //this->vtkSMUnstructuredDataParallelStrategy::UpdatePipeline();
    { 
   
    if (this->vtkSMUnstructuredDataParallelStrategy::GetDataValid())
      {
      return;
      }
    
    //this->vtkSMSimpleParallelStrategy::UpdatePipeline();
      {
      if (this->vtkSMSimpleParallelStrategy::GetDataValid())
        {
        return;
        }
      
      //this->vtkSMSimpleStrategy::UpdatePipeline();
        {
        // We check to see if the part of the pipeline that will up updated by this
        // class needs any update. Then alone do we call update.
        if (this->vtkSMSimpleStrategy::GetDataValid())
          {
          return;
          }
        
        //this->vtkSMRepresentationStrategy::UpdatePipeline()
          {
          // Update the CacheKeeper.                    
          this->DataValid = true;
          this->InformationValid = false; 
          }
        
        this->UpdateSuppressor->InvokeCommand("ForceUpdate");
        // This is called for its side-effects; i.e. to force a PostUpdateData()
        this->UpdateSuppressor->UpdatePipeline();        
        }
      
      vtkSMPropertyHelper(this->Collect, "MoveMode").Set(this->GetMoveMode()); 
      this->Collect->UpdateProperty("MoveMode");
      
      // It is essential to mark the Collect filter explicitly modified.
      vtkClientServerStream stream;
      stream  << vtkClientServerStream::Invoke
              << this->Collect->GetID()
              << "Modified"
              << vtkClientServerStream::End;
      vtkProcessModule::GetProcessModule()->SendStream(
              this->ConnectionID, this->Collect->GetServers(), stream);
      
      this->PostCollectUpdateSuppressor->InvokeCommand("ForceUpdate");

      this->CollectedDataValid = true;      
      }
      
    bool usecompositing = this->GetUseCompositing();
    // cout << "usecompositing: " << usecompositing << endl;
    
    // cout << "use ordered compositing: " << (usecompositing && this->UseOrderedCompositing)
    //  << endl;
    vtkSMPropertyHelper(this->Distributor, "PassThrough").Set(
           (usecompositing && this->UseOrderedCompositing)? 0 : 1);
    this->Distributor->UpdateProperty("PassThrough");
    
    this->PostDistributorSuppressor->InvokeCommand("ForceUpdate");
    // This is called for its side-effects; i.e. to force a PostUpdateData()
    this->PostDistributorSuppressor->UpdatePipeline();
    this->DistributedDataValid = true;
    }
}
