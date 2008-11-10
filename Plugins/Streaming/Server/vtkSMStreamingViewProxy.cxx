/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStreamingViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMStreamingViewProxy.h"

#include "vtkSMStreamingHelperProxy.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStreamingRepresentation.h"
#include "vtkSMStreamingViewHelper.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkSMPropertyHelper.h"



#include <vtkstd/vector>
#include <vtkstd/string>
#include <vtksys/ios/sstream>

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSMStreamingViewProxy, "1.2");
vtkStandardNewMacro(vtkSMStreamingViewProxy);

//-----------------------------------------------------------------------------
class vtkSMStreamingViewProxy::vtkInternals
{
public:
  vtkInternals()
  {
    for (int i =0; i < 9; i++)
      {
      this->CamState[i] = 0.0;
      }
  }

  vtkstd::vector<vtkSMStreamingRepresentation*> Representations;
  vtkSmartPointer<vtkSMRenderViewProxy> RootView;
  double CamState[9];
  double Frustum[32];
  vtkstd::string SuggestedViewType;
};

//-----------------------------------------------------------------------------
vtkSMStreamingViewProxy::vtkSMStreamingViewProxy()
{
  this->Internals = new vtkInternals();

  this->DisplayDone = 1;
  this->MaxPass = -1;
  this->PixelArray = NULL;
  this->RenderViewHelper = vtkSMStreamingViewHelper::New();
  this->RenderViewHelper->SetStreamingView(this); //not reference counted.

  this->IsSerial = true;
  this->Pass = 0;

  // Make sure the helper proxy exists
  this->GetStreamingHelperProxy();
}

//-----------------------------------------------------------------------------
vtkSMStreamingViewProxy::~vtkSMStreamingViewProxy()
{
  this->RenderViewHelper->SetStreamingView(0);
  this->RenderViewHelper->Delete();
  if (this->PixelArray)
    {
    this->PixelArray->Delete();
    }
  delete this->Internals;
}

//-----------------------------------------------------------------------------
vtkSMStreamingHelperProxy* vtkSMStreamingViewProxy::GetStreamingHelperProxy()
{
  return vtkSMStreamingHelperProxy::GetHelper();
}

//STUFF TO MAKE THIS VIEW PRETEND TO BE A RENDERVIEW
//-----------------------------------------------------------------------------
bool vtkSMStreamingViewProxy::BeginCreateVTKObjects()
{
  this->Internals->RootView = vtkSMRenderViewProxy::SafeDownCast(
    this->GetSubProxy("RootView"));
  if (!this->Internals->RootView)
    {
    vtkErrorMacro("Subproxy \"Root\" must be defined in the xml configuration.");
    return false;
    }

  if (!strcmp("StreamingRenderView", this->GetXMLName()))
    {
    this->IsSerial = true;
    }
  else
    {
    this->IsSerial = false;
    }

  return this->Superclass::BeginCreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  //replace the real view's interactor, which points to the real view
  //with one that points to this so that mouse events will result in streaming
  //renders
  vtkPVGenericRenderWindowInteractor *iren = 
    this->Internals->RootView->GetInteractor();
  iren->SetPVRenderView(this->RenderViewHelper);
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy *vtkSMStreamingViewProxy::GetRootView()
{
  return this->Internals->RootView;
}

//----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::AddRepresentation(vtkSMRepresentationProxy* rep)
{
  vtkSMStreamingRepresentation *repr = 
    vtkSMStreamingRepresentation::SafeDownCast(rep);
  if (!repr)
    {
    //quietly ignore
    return;
    }

  vtkSMViewProxy* RVP = this->GetRootView();
  if (repr && !RVP->Representations->IsItemPresent(repr))
    {
    //There is magic inside AddToView, such that it actually adds rep to RVP, 
    //but uses this to to create repr's streaming strategy.
    if (repr->AddToView(this)) 
      {
      RVP->AddRepresentationInternal(repr);
      }
    else
      {
      vtkErrorMacro(<< repr->GetClassName() << " cannot be added to view "
        << "of type " << this->GetClassName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::RemoveRepresentation(vtkSMRepresentationProxy* repr)
{
  this->GetRootView()->RemoveRepresentation(repr);
}

//----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::RemoveAllRepresentations()
{
  this->GetRootView()->RemoveAllRepresentations();
}

//----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::StillRender()
{
  static bool in_still_render = false;
  if (in_still_render)
    {
    return;
    }

  in_still_render = true;

  this->BeginStillRender();
  this->GetRootView()->BeginStillRender();

  this->PrepareRenderPass();
  this->UpdateAllRepresentations();
  this->PerformRender();
  this->FinalizeRenderPass();

  this->GetRootView()->EndStillRender();
  this->EndStillRender();
  in_still_render = false;
}

//----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::InteractiveRender()
{
  static bool in_interactive_render = false;
  if (in_interactive_render)
    {
    return;
    }

  in_interactive_render = true;

  this->BeginInteractiveRender();
  this->GetRootView()->BeginInteractiveRender();

  this->PrepareRenderPass();
  this->UpdateAllRepresentations();
  this->PerformRender();
  this->FinalizeRenderPass();

  this->GetRootView()->EndInteractiveRender();
  this->EndInteractiveRender();
  in_interactive_render = false;
}

//STUFF TO MAKE A PLUGIN VIEW WITH SPECIALIZE STREAMING REPS and STRATS
//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMStreamingViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  int doPrints = vtkSMStreamingHelperProxy::GetHelper()->GetEnableStreamMessages();
  if (doPrints)
    {
    cerr << "SV(" << this << ") CreateDefaultRepresentation" << endl;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    sproxy->UpdatePipeline(this->GetViewUpdateTime());
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations", 
    "StreamingUnstructuredGridRepresentation");

  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool usg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (usg)
    {
    if (doPrints)
      {
      cerr << "SV(" << this << ") Created StreamingUnstructuredGridRepresentation" << endl;
      }
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "StreamingUnstructuredGridRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "StreamingUniformGridRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    if (doPrints)
      {
      cerr << "SV(" << this << ") Created StreamingUniformGridRepresentation" << endl;
      }
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "StreamingUniformGridRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "StreamingGeometryRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool g = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (g)
    {
    if (doPrints)
      {
      cerr << "SV(" << this << ") Created StreamingGeometryRepresentation" << endl;
      }
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "StreamingGeometryRepresentation"));
    }

  return 0;
}

//-----------------------------------------------------------------------------
vtkSMRepresentationStrategy* vtkSMStreamingViewProxy::NewStrategyInternal(
  int dataType)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMRepresentationStrategy* strategy = 0;
    
  int doPrints = vtkSMStreamingHelperProxy::GetHelper()->GetEnableStreamMessages();
  if (this->IsSerial)
    {
    if (dataType == VTK_POLY_DATA || dataType == VTK_UNIFORM_GRID || 
        dataType == VTK_IMAGE_DATA)
      {
      if (doPrints)
        {    
        cerr << "SV(" << this << ") Creating StreamingPolyDataStrategy" << endl;
        }
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
        pxm->NewProxy("strategies", "StreamingPolyDataStrategy"));
      }
    else if (dataType == VTK_UNSTRUCTURED_GRID)
      {
      if (doPrints)
        {    
        cerr << "SV(" << this << ") Creating StreamingUnstructuredGridStrategy" << endl;
        }
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
        pxm->NewProxy("strategies", "StreamingUnstructuredGridStrategy"));
      }
    else
      {
      vtkWarningMacro("This view does not provide a suitable strategy for "
                      << dataType);
      }
    }
  else
    {
    if (dataType == VTK_POLY_DATA)
      {
      if (doPrints)
        {    
        cerr << "SV(" << this << ") Creating StreamingPolyDataParallelStrategy" << endl;
        }
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
         pxm->NewProxy("strategies", "StreamingPolyDataParallelStrategy"));
      }
    else if (dataType == VTK_UNIFORM_GRID)
      {
      if (doPrints)
        {    
        cerr << "SV(" << this << ") Creating StreamingUniformGridParallelStrategy" << endl;
        }
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
        pxm->NewProxy("strategies", "StreamingUniformGridParallelStrategy"));
      }
    else if (dataType == VTK_UNSTRUCTURED_GRID)
      {
      if (doPrints)
        {    
        cerr << "SV(" << this << ") Creating StreamingUnstructuredGridParallelStrategy" << endl;
        }
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
        pxm->NewProxy("strategies", "StreamingUnstructuredGridParallelStrategy"));
      }
    else if (dataType == VTK_IMAGE_DATA)
      {
      if (doPrints)
        {    
        cerr << "SV(" << this << ") Creating StreamingImageDataParallelStrategy" << endl;
        }
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
        pxm->NewProxy("strategies", "StreamingImageDataParallelStrategy"));
      }
    else
      {
      vtkWarningMacro("This view does not provide a suitable strategy for "
                      << dataType);
      }
    }  

  return strategy;
}

//STUFF THAT ACTUALLY MATTERS FOR MULTIPASS RENDERING
//----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::PrepareRenderPass()
{
  vtkRenderWindow *renWin = this->GetRootView()->GetRenderWindow(); 
  vtkRenderer *ren = this->GetRootView()->GetRenderer();

  bool CamChanged = this->CameraChanged();
  if (CamChanged)
    {
    this->Pass = 0;
    }
  //prepare for incremental rendering
  if (this->Pass == 0)
    {
    //cls
    ren->Clear(); 
    //don't cls on following render passes
    renWin->EraseOff();
    ren->EraseOff();
    //render into back buffer and do not swap, so we can keep it intact
    //and add to each each pass
    renWin->SwapBuffersOff();

    if (CamChanged)
      {
      vtkSmartPointer<vtkCollectionIterator> iter;
      iter.TakeReference(this->GetRootView()->Representations->NewIterator());
      for (iter->InitTraversal(); 
           !iter->IsDoneWithTraversal(); 
           iter->GoToNextItem())
        {
        vtkSMStreamingRepresentation* srep = 
          vtkSMStreamingRepresentation::SafeDownCast(iter->GetCurrentObject());
        if (srep && srep->GetVisibility()) 
          {
          srep->SetViewState(this->Internals->CamState, this->Internals->Frustum);
          }
        }
      }    
    }
}

//----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::FinalizeRenderPass()
{
  vtkRenderWindow *renWin = this->GetRootView()->GetRenderWindow();
  vtkRenderer *ren = this->GetRootView()->GetRenderer();

  if (this->DisplayDone)
    {
    renWin->SwapBuffersOn();
    renWin->Frame();
    //reset to normal cls before each render behavior
    renWin->EraseOn();
    ren->EraseOn();
    }
  else
    {
    //take all that we've drawn into back buffer and show it
    this->CopyBackBufferToFrontBuffer();
    }
}

//-----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::CopyBackBufferToFrontBuffer()
{
  vtkRenderWindow *renWin = this->GetRootView()->GetRenderWindow();

  //allocate pixel storage
  int *size = renWin->GetSize();
  if (!this->PixelArray)
    {
    this->PixelArray = vtkUnsignedCharArray::New();
    }
  this->PixelArray->Initialize();
  this->PixelArray->SetNumberOfComponents(4);
  this->PixelArray->SetNumberOfTuples(size[0]*size[1]);  

  //capture back buffer
  renWin->GetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, 0, this->PixelArray);

  //copy into the front buffer
  renWin->SetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, this->PixelArray, 1);

  //hack, this call is just here to reset glDrawBuffer(BACK)
  renWin->SetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, this->PixelArray, 0);
}

//----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::UpdateAllRepresentations()
{
  if (this->Pass == 0)
    {
    this->MaxPass = -1;
    }

  vtkSMRenderViewProxy *RVP = this->GetRootView();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  int nPasses = vtkSMStreamingHelperProxy::GetHelper()->GetStreamedPasses();
  int doPrints = vtkSMStreamingHelperProxy::GetHelper()->GetEnableStreamMessages();
  //int useViewOrdering = vtkSMStreamingHelperProxy::GetHelper()->GetUseViewOrdering();

  if (doPrints)
    {
    cerr << "SV::UpdateAllRepresentations" << endl;
    }

  //Update pipeline for each representation.
  //For representations that allow streaming, compute a piece priority order
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(RVP->Representations->NewIterator());
  bool enable_progress = false;
  for (iter->InitTraversal(); 
       !iter->IsDoneWithTraversal(); 
       iter->GoToNextItem())
    {
    vtkSMRepresentationProxy* repr = 
      vtkSMRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
    if (!repr->GetVisibility())
      {
      // Invisible representations are not updated.
      continue;
      }

    if (!enable_progress && repr->UpdateRequired())
      {
      // If a representation required an update, than it implies that the
      // update will result in progress events. We don't to ignore those
      // progress events, hence we enable progress handling.
      pm->SendPrepareProgress(this->ConnectionID,
        vtkProcessModule::CLIENT | vtkProcessModule::DATA_SERVER);
      enable_progress = true;
      }
    
    vtkSMStreamingRepresentation *drepr = 
      vtkSMStreamingRepresentation::SafeDownCast(repr);
    if (drepr && nPasses > 1)
      {
      if (this->Pass == 0)
        {
        if (doPrints)
          {
          cerr << "SV(" << this << ") Compute priorities on DREP " << drepr << endl;
          }
        int maxpass = drepr->ComputePriorities();
        if (maxpass > this->MaxPass)
          {
          if (doPrints)
            {
            cerr << "SV(" << this << ") MaxPass is now " << maxpass << endl;
            }
          this->MaxPass = maxpass;
          }
        }

      }
    else
      {
      //cerr << "GOT " << repr->GetClassName() << endl;
      }
    if (this->Pass == 0)
      {
      repr->Update(RVP);
      }
    }

  if (enable_progress)
    {
    pm->SendCleanupPendingProgress(this->ConnectionID);
    }
}

//-----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::PerformRender()
{
  int doPrints = vtkSMStreamingHelperProxy::GetHelper()->GetEnableStreamMessages();
  if (doPrints)
    {
    cerr << "SV:PerformRender" << endl;
    }
  vtkSMRenderViewProxy *RVP = this->GetRootView();

  this->DisplayDone = 1;
  int nPasses = vtkSMStreamingHelperProxy::GetHelper()->GetStreamedPasses(); 
  if (this->MaxPass == -1)
    {
    nPasses = 1;
    }
  if (this->MaxPass > -1 && this->MaxPass < nPasses)
    {
    nPasses = this->MaxPass;
    }  

  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(RVP->Representations->NewIterator());
  for (iter->InitTraversal(); 
       !iter->IsDoneWithTraversal(); 
       iter->GoToNextItem())
    {
    vtkSMRepresentationProxy* repr = 
      vtkSMRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
    if (!repr->GetVisibility())
      {
      // Invisible representations are not updated.
      continue;
      }
    vtkSMStreamingRepresentation *drepr = 
      vtkSMStreamingRepresentation::SafeDownCast(repr);
    if (drepr)
      {
      //if representation supports pieces, choose most important one to render in this pass      
      if (this->Pass < nPasses)
        {
        if (doPrints)
          {
          cerr << "SV(" << this << ") Update Pass " << this->Pass << endl;
          }
        drepr->SetPassNumber(this->Pass, 1);
        //update pipeline to get the geometry for that piece
        drepr->Update(this); //get geometry for next stripe
        }
      }
    }

  if (this->Pass+1 < nPasses)
    {
    if (doPrints)
      {
      cerr << "SV(" << this << ") Need more passes " << endl;
      }    
    this->DisplayDone = 0;
    }
  else
    {
    if (doPrints)
      {
      cerr << "SV(" << this << ") All passes finished " << endl;
      }    
    }
  
  if ( RVP->GetMeasurePolygonsPerSecond() )
    {
    this->RenderTimer->StartTimer();
    }

  vtkRenderWindow *renWin = RVP->GetRenderWindow(); 
  renWin->Render();

  if (this->DisplayDone)
    {
    this->Pass = 0;
    }
  else
    {
    this->Pass++;
    }

  if ( RVP->GetMeasurePolygonsPerSecond() )
    {
    this->RenderTimer->StopTimer();
    RVP->CalculatePolygonsPerSecond(this->RenderTimer->GetElapsedTime());
    }
}

//-----------------------------------------------------------------------------
int vtkSMStreamingViewProxy::GetDisplayDone()
{
  return this->DisplayDone;
}

//-----------------------------------------------------------------------------
void vtkSMStreamingViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


//-----------------------------------------------------------------------------
bool vtkSMStreamingViewProxy::CameraChanged()
{
  bool changed = false;
  vtkSMRenderViewProxy *RVP = this->GetRootView();
  vtkCamera *cam = RVP->GetActiveCamera();
  if (cam)
    {
    double camState[9];
    cam->GetPosition(&camState[0]);
    cam->GetViewUp(&camState[3]);
    cam->GetFocalPoint(&camState[6]);
    for (int i = 0; i < 9; i++)
      {          
      if (camState[i] != this->Internals->CamState[i])
        {
        changed = true;
        break;
        }
      }
    memcpy(this->Internals->CamState, camState, 9*sizeof(double));      

    if (changed)
      {
      vtkRenderer *renderer = RVP->GetRenderer();
      //convert screen rectangle to world frustum
      const double XMAX=1.0;
      const double XMIN=-1.0;
      const double YMAX=1.0;
      const double YMIN=-1.0;
      static double viewP[32] = {
        XMIN, YMIN,  0.0, 1.0,
        XMIN, YMIN,  1.0, 1.0,
        XMIN,  YMAX,  0.0, 1.0,
        XMIN,  YMAX,  1.0, 1.0,
        XMAX, YMIN,  0.0, 1.0,
        XMAX, YMIN,  1.0, 1.0,
        XMAX,  YMAX,  0.0, 1.0,
        XMAX,  YMAX,  1.0, 1.0
        };
      memcpy(this->Internals->Frustum, viewP, 32*sizeof(double));
      for (int index=0; index<8; index++)
        {
        renderer->ViewToWorld(this->Internals->Frustum[index*4+0], 
                              this->Internals->Frustum[index*4+1], 
                              this->Internals->Frustum[index*4+2]);
        }
      }
    }
  return changed;
}

//----------------------------------------------------------------------------
const char* vtkSMStreamingViewProxy::GetSuggestedViewType(vtkIdType connectionID)
{
  vtkSMViewProxy* rootView = vtkSMViewProxy::SafeDownCast(this->GetSubProxy("RootView"));
  if (rootView)
    {
    vtksys_ios::ostringstream stream;
    stream << "Streaming" << rootView->GetSuggestedViewType(connectionID);
    this->Internals->SuggestedViewType = stream.str();
    return this->Internals->SuggestedViewType.c_str();
    }

  return this->Superclass::GetSuggestedViewType(connectionID);
}

