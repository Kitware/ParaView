/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAdaptiveViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMAdaptiveViewProxy.h"
#include "vtkSMAdaptiveOptionsProxy.h"
#include "vtkAdaptiveOptions.h"
#include "vtkCamera.h"
#include "vtkClientServerStream.h"
#include "vtkCollectionIterator.h"
#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMAdaptiveOutlineRepresentation.h"
#include "vtkSMAdaptiveRepresentation.h"
#include "vtkSMAdaptiveViewHelper.h"
#include "vtkSMUtilities.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWindowToImageFilter.h"

#include <vtkstd/vector>
#include <vtkstd/string>
#include <vtksys/ios/sstream>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMAdaptiveViewProxy);

#define DEBUGPRINT_VIEW(arg)\
  if (vtkAdaptiveOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

#define DEBUGPRINT_VIEW2(arg) \
  if (vtkAdaptiveOptions::GetEnableStreamMessages()) \
    { \
      arg;\
    }

//-----------------------------------------------------------------------------
class vtkSMAdaptiveViewProxy::vtkInternals
{
public:
  vtkInternals()
  {
    for (int i =0; i < 9; i++)
      {
      this->CamState[i] = 0.0;
      }
  }

  vtkSmartPointer<vtkSMRenderViewProxy> RootView;
  double CamState[9];
  double Frustum[32];
  vtkstd::string SuggestedViewType;
};

//-----------------------------------------------------------------------------
vtkSMAdaptiveViewProxy::vtkSMAdaptiveViewProxy()
{
  this->Internals = new vtkInternals();

  //link up interaction properly
  this->RenderViewHelper = vtkSMAdaptiveViewHelper::New();
  this->RenderViewHelper->SetAdaptiveView(this); //not reference counted.

  //informs decision about types of strategies to create
  this->IsSerial = true;

  // Make sure the proxy that manager adaptive view options exists
  this->GetAdaptiveOptionsProxy();

  // this pixel storage is used in adding in next pieces contribution
  this->PixelArray = NULL;

  // input state controls
  //controls for when refinement happens
  this->RefinementMode = MANUAL;
  this->AdvanceCommand = STAY;

  //view clears this when it is ready to stop streaming
  this->DisplayDone = 1;

  //view sets this when it comes to the end of a wend
  this->WendDone = 1;

  this->ProgrammaticRestart = false;
}

//-----------------------------------------------------------------------------
vtkSMAdaptiveViewProxy::~vtkSMAdaptiveViewProxy()
{
  this->RenderViewHelper->SetAdaptiveView(0);
  this->RenderViewHelper->Delete();
  if (this->PixelArray)
    {
    this->PixelArray->Delete();
    }
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
vtkSMAdaptiveOptionsProxy* vtkSMAdaptiveViewProxy::GetAdaptiveOptionsProxy()
{
  return vtkSMAdaptiveOptionsProxy::GetProxy();
}

//STUFF TO MAKE THIS VIEW PRETEND TO BE A RENDERVIEW
//-----------------------------------------------------------------------------
bool vtkSMAdaptiveViewProxy::BeginCreateVTKObjects()
{
  this->Internals->RootView = vtkSMRenderViewProxy::SafeDownCast(
    this->GetSubProxy("RootView"));
  if (!this->Internals->RootView)
    {
    vtkErrorMacro("Subproxy \"Root\" must be defined in the xml configuration.");
    return false;
    }

  if (!strcmp("AdaptiveRenderView", this->GetXMLName()))
    {
    DEBUGPRINT_VIEW(cerr << "SV(" << this << ") Created serial view" << endl;);
    this->IsSerial = true;
    }
  else
    {
    DEBUGPRINT_VIEW(cerr << "SV(" << this << ") Created parallel view type " << this->GetXMLName() << endl;);
    this->IsSerial = false;
    }

  return this->Superclass::BeginCreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  //replace the real view's interactor, which points to the real view
  //with one that points to this. Then mouse events result in streaming
  //renders.
  vtkPVGenericRenderWindowInteractor *iren = 
    this->Internals->RootView->GetInteractor();
  iren->SetPVRenderView(this->RenderViewHelper);

  //turn off the axes widgets by default
  vtkSMIntVectorProperty *vis;
  vtkSMProxy *annotation;
  annotation = this->Internals->RootView->GetOrientationWidgetProxy();
  vis = vtkSMIntVectorProperty::SafeDownCast(annotation->GetProperty("Visibility"));
  vis->SetElement(0,0);
  annotation = this->Internals->RootView->GetCenterAxesProxy();
  vis = vtkSMIntVectorProperty::SafeDownCast(annotation->GetProperty("Visibility"));
  vis->SetElement(0,0);

  this->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy *vtkSMAdaptiveViewProxy::GetRootView()
{
  return this->Internals->RootView;
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::AddRepresentation(vtkSMRepresentationProxy* rep)
{
  vtkSMAdaptiveRepresentation *srep = 
    vtkSMAdaptiveRepresentation::SafeDownCast(rep);
  vtkSMAdaptiveOutlineRepresentation *orep = 
    vtkSMAdaptiveOutlineRepresentation::SafeDownCast(rep);
  vtkSMViewProxy* RVP = this->GetRootView();
  if (!srep && !orep)
    {
    //representation is not specialized for streaming, add it to child 
    //view and hope for the best
    RVP->AddRepresentation(rep);
    return;
    }

  if (rep && !RVP->Representations->IsItemPresent(rep))
    {
    //There is magic inside AddToView, such that it actually adds rep to RVP, 
    //but uses this to to create repr's streaming strategy.
    if (rep->AddToView(this)) 
      {
      RVP->AddRepresentationInternal(rep);
      }
    else
      {
      vtkErrorMacro(<< rep->GetClassName() << " cannot be added to view "
        << "of type " << this->GetClassName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::RemoveRepresentation(vtkSMRepresentationProxy* rep)
{
  this->GetRootView()->RemoveRepresentation(rep);
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::RemoveAllRepresentations()
{
  this->GetRootView()->RemoveAllRepresentations();
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::StillRender()
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
void vtkSMAdaptiveViewProxy::InteractiveRender()
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

//-----------------------------------------------------------------------------
vtkImageData* vtkSMAdaptiveViewProxy::CaptureWindow(int magnification)
{
  vtkRenderWindow *renWin = this->GetRootView()->GetRenderWindow();

  vtkWindowToImageFilter* w2i = vtkWindowToImageFilter::New();
  w2i->SetInput(renWin);
  w2i->SetMagnification(magnification);
  w2i->ReadFrontBufferOn();
  w2i->ShouldRerenderOff();
  w2i->Update();

  vtkImageData* capture = vtkImageData::New();
  capture->ShallowCopy(w2i->GetOutput());
  w2i->Delete();

  // Update image extents based on ViewPosition
  int extents[6];
  capture->GetExtent(extents);
  for (int cc=0; cc < 4; cc++)
    {
    extents[cc] += this->ViewPosition[cc/2]*magnification;
    }
  capture->SetExtent(extents);

  return capture;
}

//-----------------------------------------------------------------------------
int vtkSMAdaptiveViewProxy::WriteImage(const char* filename, int magnification)
{
  vtkSmartPointer<vtkImageData> shot;
  shot.TakeReference(this->CaptureWindow(magnification));
  return vtkSMUtilities::SaveImage(shot, filename);
}

//-----------------------------------------------------------------------------
int vtkSMAdaptiveViewProxy::WriteImage(const char* filename,
  const char* writerName, int magnification)
{
  if (!filename || !writerName)
    {
    return vtkErrorCode::UnknownError;
    }

  vtkSmartPointer<vtkImageData> shot;
  shot.TakeReference(this->CaptureWindow(magnification));
  return vtkSMUtilities::SaveImageOnProcessZero(shot, filename, writerName);
}

//STUFF TO MAKE A PLUGIN VIEW WITH SPECIALIZED STREAMING REPS and STRATS
//----------------------------------------------------------------------------
const char* vtkSMAdaptiveViewProxy::GetSuggestedViewType(vtkIdType connectionID)
{
  vtkSMViewProxy* rootView = vtkSMViewProxy::SafeDownCast(this->GetSubProxy("RootView"));
  if (rootView)
    {
    vtksys_ios::ostringstream stream;
    stream << "Adaptive" << rootView->GetSuggestedViewType(connectionID);
    this->Internals->SuggestedViewType = stream.str();
    return this->Internals->SuggestedViewType.c_str();
    }

  return this->Superclass::GetSuggestedViewType(connectionID);
}

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMAdaptiveViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  DEBUGPRINT_VIEW(
    cerr << "SV(" << this << ") CreateDefaultRepresentation" << endl;
    );

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    sproxy->UpdatePipeline(this->GetViewUpdateTime());
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations", 
    "AdaptiveUnstructuredGridRepresentation");

  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool usg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (usg)
    {
    DEBUGPRINT_VIEW(
      cerr << "SV(" << this << ") Created AdaptiveUnstructuredGridRepresentation" << endl;
      );
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "AdaptiveUnstructuredGridRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "AdaptiveUniformGridRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    DEBUGPRINT_VIEW(
      cerr << "SV(" << this << ") Created AdaptiveUniformGridRepresentation" << endl;
      );
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "AdaptiveUniformGridRepresentation"));
    }

  prototype = pxm->GetPrototypeProxy("representations",
    "AdaptiveGeometryRepresentation");
  pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool g = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (g)
    {
    DEBUGPRINT_VIEW(
      cerr << "SV(" << this << ") Created AdaptiveGeometryRepresentation" << endl;
      );
    return vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "AdaptiveGeometryRepresentation"));
    }

  return 0;
}

//-----------------------------------------------------------------------------
vtkSMRepresentationStrategy* vtkSMAdaptiveViewProxy::NewStrategyInternal(
  int dataType)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMRepresentationStrategy* strategy = 0;
    
  if (this->IsSerial)
    {
    if (dataType == VTK_POLY_DATA || dataType == VTK_UNIFORM_GRID || 
        dataType == VTK_IMAGE_DATA)
      {
      DEBUGPRINT_VIEW(
        cerr << "SV(" << this << ") Creating AdaptivePolyDataStrategy" << endl;
        );
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
        pxm->NewProxy("strategies", "AdaptivePolyDataStrategy"));
      }
    else if (dataType == VTK_UNSTRUCTURED_GRID)
      {
      DEBUGPRINT_VIEW(
        cerr << "SV(" << this << ") Creating AdaptiveUnstructuredGridStrategy" << endl;
        );
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
        pxm->NewProxy("strategies", "AdaptiveUnstructuredGridStrategy"));
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
      DEBUGPRINT_VIEW(
        cerr << "SV(" << this << ") Creating AdaptivePolyDataParallelStrategy" << endl;
        );
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
         pxm->NewProxy("strategies", "AdaptivePolyDataParallelStrategy"));
      }
    else if (dataType == VTK_UNIFORM_GRID)
      {
      DEBUGPRINT_VIEW(
        cerr << "SV(" << this << ") Creating AdaptiveUniformGridParallelStrategy" << endl;
        );
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
        pxm->NewProxy("strategies", "AdaptiveUniformGridParallelStrategy"));
      }
    else if (dataType == VTK_UNSTRUCTURED_GRID)
      {
      DEBUGPRINT_VIEW(
        cerr << "SV(" << this << ") Creating AdaptiveUnstructuredGridParallelStrategy" << endl;
        );
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
        pxm->NewProxy("strategies", "AdaptiveUnstructuredGridParallelStrategy"));
      }
    else if (dataType == VTK_IMAGE_DATA)
      {
      DEBUGPRINT_VIEW(
        cerr << "SV(" << this << ") Creating AdaptiveImageDataParallelStrategy" << endl;
        );
      strategy = vtkSMRepresentationStrategy::SafeDownCast(
        pxm->NewProxy("strategies", "AdaptiveImageDataParallelStrategy"));
      }
    else
      {
      vtkWarningMacro("This view does not provide a suitable strategy for "
                      << dataType);
      }
    }  

  vtkSMProperty *prop = strategy->GetProperty("PrepareFirstPass");
  if (prop)
    {
    prop->Modified();
    }

  return strategy;
}

//STUFF THAT ACTUALLY MATTERS FOR MULTIPASS RENDERING
//-----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::SetRefinementMode(int i)
{
  if (this->RefinementMode == i)
    {
    return;
    }
  this->RefinementMode = i;
  this->AdvanceCommand = STAY;
  this->ProgrammaticRestart = true;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::Refine()
{
  if (this->RefinementMode != MANUAL ||
      this->AdvanceCommand == REFINE)
    {
    return;
    }
  this->AdvanceCommand = REFINE;
  this->ProgrammaticRestart = true;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::Coarsen()
{
  if (this->RefinementMode != MANUAL ||
      this->AdvanceCommand == COARSEN)
    {
    return;
    }
  this->AdvanceCommand = COARSEN;
  this->ProgrammaticRestart = true;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::Interrupt()
{
  this->SetRefinementMode(MANUAL);
  this->DisplayDone = 1;
}

//-----------------------------------------------------------------------------
int vtkSMAdaptiveViewProxy::GetDisplayDone()
{
  return this->DisplayDone;
}

//-----------------------------------------------------------------------------
bool vtkSMAdaptiveViewProxy::CameraChanged()
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

    if (changed)
      {
      memcpy(this->Internals->CamState, camState, 9*sizeof(double));

      vtkRenderer *renderer = RVP->GetRenderer();
      //convert screen rectangle to world frustum
      const double HALFEXT=1.0; /*1.0 means all way to edge of screen*/
      const double XMAX=HALFEXT;
      const double XMIN=-HALFEXT;
      const double YMAX=HALFEXT;
      const double YMIN=-HALFEXT;
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

//-----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::CopyBackBufferToFrontBuffer()
{
  vtkRenderWindow *renWin = this->GetRootView()->GetRenderWindow();

  //allocate pixel storage
  int *size = renWin->GetSize();
  if (!this->PixelArray)
    {
    this->PixelArray = vtkUnsignedCharArray::New();
    this->PixelArray->SetNumberOfComponents(4);
    }

  //this->PixelArray->Initialize();
  if(size[0] * size[1] != this->PixelArray->GetNumberOfTuples())
    {
    this->PixelArray->SetNumberOfTuples(size[0]*size[1]);  
    }

  //capture back buffer
  renWin->GetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, 0, this->PixelArray);

  //copy into the front buffer
  renWin->SetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, this->PixelArray, 1);

  /*
  //hack, this call is just here to reset glDrawBuffer(BACK)
  renWin->SetRGBACharPixelData(0, 0, size[0]-1, size[1]-1, this->PixelArray, 0);
  */
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::PrepareRenderPass()
{
  static bool firstpass=true;
  vtkRenderWindow *renWin = this->GetRootView()->GetRenderWindow(); 
  vtkRenderer *ren = this->GetRootView()->GetRenderer();
  
  bool CamChanged = this->CameraChanged();

  //prepare for incremental rendering
  if (CamChanged || 
      this->WendDone || this->DisplayDone || 
      this->ProgrammaticRestart)
    {
    if (firstpass) 
      {
      //force a render, just to make sure we have a graphics context
      renWin->Render();
      firstpass = false;

      //Setup to for multi pass render process
      //don't cls automatically whenever renderwindow->render() starts
      renWin->EraseOff();
      ren->EraseOff();
      //don't swap back to front automatically whenever renderwindow->render() finishes
      renWin->SwapBuffersOff();
      }

    this->Pass = 0;
    ren->Clear();

    DEBUGPRINT_VIEW2(cerr << "SV(" << this << ") start pass 0" << endl;);
    }
  else
    {
    this->Pass++;
    DEBUGPRINT_VIEW2(cerr << "SV(" << this << ") start pass " << this->Pass << endl;);
    }
 
  //tell each representation to prepare
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(this->GetRootView()->Representations->NewIterator());
  for (iter->InitTraversal(); 
       !iter->IsDoneWithTraversal(); 
       iter->GoToNextItem())
    {
    vtkSMAdaptiveRepresentation* srep = 
      vtkSMAdaptiveRepresentation::SafeDownCast(iter->GetCurrentObject());
    if (srep && srep->GetVisibility()) 
      {
      if (CamChanged || 
          this->WendDone || this->DisplayDone ||
          this->ProgrammaticRestart)
        {
        //tell it to start domain over
        if (CamChanged || this->ProgrammaticRestart)
          {
          srep->SetViewState(this->Internals->CamState, this->Internals->Frustum);
          }
        if ((this->WendDone && this->RefinementMode == AUTOMATIC_REFINE)||
            this->AdvanceCommand == REFINE)
          {
          srep->Refine();
          }
        if ((this->WendDone && this->RefinementMode == AUTOMATIC_COARSEN)||
            this->AdvanceCommand == COARSEN)
          {
          srep->Coarsen();
          }
        srep->PrepareFirstPass();
        }
      else
        {
        //tell it to get the next piece ready
        if (!srep->GetAllDone() && !srep->GetWendDone())
          {
          srep->PrepareAnotherPass();
          }
        }
      }
    }

  this->WendDone = true;
  this->DisplayDone = true;

  this->AdvanceCommand = STAY;
  this->ProgrammaticRestart = false; 
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::UpdateAllRepresentations()
{
  vtkSMRenderViewProxy *RVP = this->GetRootView();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  DEBUGPRINT_VIEW(
    cerr << "SV(" << this << ")::UpdateAllRepresentations" << endl;
    cerr << "VWDONE = " << this->WendDone << endl;
    );

  //Update pipeline for each representation.
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(RVP->Representations->NewIterator());
  bool enable_progress = false;
  for (iter->InitTraversal(); 
       !iter->IsDoneWithTraversal(); 
       iter->GoToNextItem())
    {
    vtkSMRepresentationProxy* rep = 
      vtkSMRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
    if (!rep->GetVisibility())
      {
      // Invisible representations are not updated.
      continue;
      }

    if (!enable_progress && rep->UpdateRequired())
      {
      // If a representation required an update, than it implies that the
      // update will result in progress events. We don't want to ignore 
      // those progress events, hence we enable progress handling.
      pm->SendPrepareProgress(this->ConnectionID,
        vtkProcessModule::CLIENT | vtkProcessModule::DATA_SERVER);
      enable_progress = true;
      }
    
    vtkSMAdaptiveRepresentation* srep = 
      vtkSMAdaptiveRepresentation::SafeDownCast(rep);
    if (srep)
      {
      if (!srep->GetAllDone() && !srep->GetWendDone())
        {
        srep->ChooseNextPiece();
        //srep->Update(this); //choose updates when it needs to, so this doesn't have to be called
        }
      
      int rWendDone = srep->GetWendDone();

      DEBUGPRINT_VIEW(cerr << "RWDONE = " << rWendDone << endl;);
      
      this->WendDone = this->WendDone && srep->GetWendDone();
      }
    else
      {
      if (this->Pass == 0)
        {
        rep->Update(RVP);
        }
      }
    }

  DEBUGPRINT_VIEW(cerr << "VWDONE = " << this->WendDone << endl;);

  if (enable_progress)
    {
    pm->SendCleanupPendingProgress(this->ConnectionID);
    }
}

//-----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::PerformRender()
{
  DEBUGPRINT_VIEW(
    cerr << "SV(" << this << ")::PerformRender" << endl;
    );

  vtkSMRenderViewProxy *RVP = this->GetRootView();

  if ( RVP->GetMeasurePolygonsPerSecond() )
    {
    this->RenderTimer->StartTimer();
    }

  vtkSMProxy *RWProxy = RVP->GetRenderWindowProxy();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << RWProxy->GetID()
         << "Render"
         << vtkClientServerStream::End;
  vtkProcessModule::GetProcessModule()->SendStream(
    this->ConnectionID,
    vtkProcessModule::CLIENT,
    stream);

  if ( RVP->GetMeasurePolygonsPerSecond() )
    {
    this->RenderTimer->StopTimer();
    RVP->CalculatePolygonsPerSecond(this->RenderTimer->GetElapsedTime());
    }
}

//----------------------------------------------------------------------------
void vtkSMAdaptiveViewProxy::FinalizeRenderPass()
{
  DEBUGPRINT_VIEW(
                   cerr << "SV(" << this << ") FinalizeRenderPass" << endl;
                   cerr << "VADONE = " << this->DisplayDone << endl;
                   );

  //tell each representation to prepare for next frame
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(this->GetRootView()->Representations->NewIterator());
  for (iter->InitTraversal(); 
       !iter->IsDoneWithTraversal(); 
       iter->GoToNextItem())
    {
    vtkSMAdaptiveRepresentation* srep = 
      vtkSMAdaptiveRepresentation::SafeDownCast(iter->GetCurrentObject());
    if (srep && srep->GetVisibility())
      {
      //TODO: think about moving this to before prepare render pass
      if (this->RefinementMode != MANUAL)
        {        
        DEBUGPRINT_VIEW(cerr << "SV(" << this << ") " << srep << " FinishPass " << endl;);
        srep->FinishPass();
        int rAllDone = srep->GetAllDone();      
        DEBUGPRINT_VIEW(cerr << "RADONE = " << rAllDone << endl;);
        this->DisplayDone = this->DisplayDone && rAllDone;
        }
      else
        {
        this->DisplayDone = this->DisplayDone && this->WendDone;
        }
      }
    }
    
  DEBUGPRINT_VIEW(cerr << "VADONE = " << this->DisplayDone << endl;);

  vtkRenderWindow *renWin = this->GetRootView()->GetRenderWindow();
  int showStep = vtkAdaptiveOptions::GetShowOn();
  if (showStep == vtkAdaptiveOptions::PIECE ||
      (showStep == vtkAdaptiveOptions::REFINE && this->WendDone) ||
      this->DisplayDone
    )
    {
    DEBUGPRINT_VIEW(cerr << "SV(" << this << ") Update Front Buffer" << endl;);

    this->CopyBackBufferToFrontBuffer();
    renWin->SwapBuffersOn();
    renWin->Frame();
    renWin->SwapBuffersOff();
    }


//  cerr << "---------------------<any key>----------------------" << endl;
//  vtkstd::string s;
//  cin >> s;
}

