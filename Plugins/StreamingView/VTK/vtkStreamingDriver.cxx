/*=========================================================================

  Program:   ParaView
  Module:    vtkStreamingDriver.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamingDriver.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkInteractorStyle.h"
#include "vtkObjectFactory.h"
#include "vtkParallelStreamHelper.h"
#include "vtkPieceCacheFilter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkStreamingHarness.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVisibilityPrioritizer.h"

class vtkStreamingDriver::Internals
{
public:
  Internals(vtkStreamingDriver *owner)
  {
    this->Owner = owner;
    this->Renderer = NULL;
    this->RenderWindow = NULL;
    this->WindowWatcher = NULL;
    this->Harnesses = vtkCollection::New();
    this->RenderLaterFunction = NULL;
    this->RenderLaterArgument = NULL;
    //auxilliary functionality, that help view sorting sublasses
    this->ViewSorter = vtkVisibilityPrioritizer::New();
    this->CameraTime = 0;
    this->PixelArray = NULL;
    this->ParallelHelper = NULL;
  }
  ~Internals()
  {
    this->Owner->SetRenderer(NULL);
    this->Owner->SetRenderWindow(NULL);
    if (this->WindowWatcher)
      {
      this->WindowWatcher->Delete();
      }
    this->Harnesses->Delete();
    this->ViewSorter->Delete();
    if (this->PixelArray)
      {
      this->PixelArray->Delete();
      }
    if (this->ParallelHelper)
      {
      this->ParallelHelper->Delete();
      }
  }

  vtkStreamingDriver *Owner;
  vtkRenderer *Renderer;
  vtkRenderWindow *RenderWindow;
  vtkCallbackCommand *WindowWatcher;
  vtkCollection *Harnesses;
  void (*RenderLaterFunction) (void *);
  void *RenderLaterArgument;
  vtkUnsignedCharArray *PixelArray;
  vtkParallelStreamHelper *ParallelHelper;
  //auxilliary functionality, that help view sorting sublasses
  vtkVisibilityPrioritizer *ViewSorter;
  unsigned long CameraTime;
};

static void VTKSD_RenderEvent(vtkObject *vtkNotUsed(caller),
                              unsigned long eventid,
                              void *who,
                              void *)
{
  vtkStreamingDriver *self = reinterpret_cast<vtkStreamingDriver*>(who);
  if (eventid == vtkCommand::StartEvent)
    {
    self->StartRenderEvent();
    }
  if (eventid == vtkCommand::EndEvent)
    {
    self->EndRenderEvent();
    }
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::AssignRenderLaterFunction(void (*foo)(void *),
                                                   void*bar)
{
  this->Internal->RenderLaterFunction = foo;
  this->Internal->RenderLaterArgument = bar;
}

//----------------------------------------------------------------------------
vtkStreamingDriver::vtkStreamingDriver()
{
  this->Internal = new Internals(this);
  this->ManualStart = false;
  this->ManualFinish = false;

  this->DisplayFrequency = 0;
  this->CacheSize = 32;
}

//----------------------------------------------------------------------------
vtkStreamingDriver::~vtkStreamingDriver()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::SetRenderWindow(vtkRenderWindow *rw)
{
  if (this->Internal->RenderWindow)
    {
    this->Internal->RenderWindow->Delete();
    }
  this->Internal->RenderWindow = rw;
  if (!rw)
    {
    return;
    }
  rw->Register(this);
  vtkRenderWindowInteractor *iren = rw->GetInteractor();
  if(iren)
    {
    vtkInteractorStyle *istyle = vtkInteractorStyle::SafeDownCast
      (iren->GetInteractorStyle());
    if (istyle)
      {
      istyle->AutoAdjustCameraClippingRangeOff();
      }
    }

  if (this->Internal->WindowWatcher)
    {
    this->Internal->WindowWatcher->Delete();
    }
  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetCallback(VTKSD_RenderEvent);
  cbc->SetClientData((void*)this);
  if (!this->ManualStart)
    {
    rw->AddObserver(vtkCommand::StartEvent,cbc);
    }
  if (!this->ManualFinish)
    {
    rw->AddObserver(vtkCommand::EndEvent,cbc);
    }
  this->Internal->WindowWatcher = cbc;
}

//----------------------------------------------------------------------------
vtkRenderWindow *vtkStreamingDriver::GetRenderWindow()
{
  return this->Internal->RenderWindow;
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::SetRenderer(vtkRenderer *ren)
{
  if (this->Internal->Renderer)
    {
    this->Internal->Renderer->Delete();
    }
  if (!ren)
    {
    return;
    }

  ren->Register(this);
  this->Internal->Renderer = ren;
}

//----------------------------------------------------------------------------
vtkRenderer *vtkStreamingDriver::GetRenderer()
{
  return this->Internal->Renderer;
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::AddHarness(vtkStreamingHarness *harness)
{
  if (!harness)
    {
    return;
    }
  if (this->Internal->Harnesses->IsItemPresent(harness))
    {
    return;
    }
  this->AddHarnessInternal(harness);
  this->Internal->Harnesses->AddItem(harness);
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::RemoveHarness(vtkStreamingHarness *harness)
{
  if (!harness)
    {
    return;
    }
  this->Internal->Harnesses->RemoveItem(harness);
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::RemoveAllHarnesses()
{
  this->Internal->Harnesses->RemoveAllItems();
}

//----------------------------------------------------------------------------
vtkCollection * vtkStreamingDriver::GetHarnesses()
{
  return this->Internal->Harnesses;
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::RenderEventually()
{
  if (this->Internal->RenderLaterFunction)
    {
    this->Internal->RenderLaterFunction
      (this->Internal->RenderLaterArgument);
    return;
    }

  if (this->Internal->RenderWindow)
    {
    this->Internal->RenderWindow->Render();
    }
}

//----------------------------------------------------------------------------
vtkParallelStreamHelper *vtkStreamingDriver::GetParallelHelper()
{
  return this->Internal->ParallelHelper;
}

//----------------------------------------------------------------------------
void vtkStreamingDriver::SetParallelHelper(vtkParallelStreamHelper *hlp)
{
  if (this->Internal->ParallelHelper)
    {
    this->Internal->ParallelHelper->Delete();
    }
  if (!hlp)
    {
    return;
    }

  hlp->Register(this);
  this->Internal->ParallelHelper = hlp;
}

//----------------------------------------------------------------------------
bool vtkStreamingDriver::HasCameraMoved()
{
  vtkRenderer *ren = this->GetRenderer();
  if (!ren)
    {
    return false;
    }

  vtkCamera *cam = ren->GetActiveCamera();
  if (!cam)
    {
    return false;
    }

  unsigned long mtime = cam->GetMTime();
  if (mtime > this->Internal->CameraTime)
    {
    this->Internal->CameraTime = mtime;

    double camState[9];
    cam->GetPosition(&camState[0]);
    cam->GetViewUp(&camState[3]);
    cam->GetFocalPoint(&camState[6]);

    //convert screen rectangle to world frustum
    const double HALFEXT=1.0; //1.0 means all way to edge of screen
    const double XMAX=HALFEXT;
    const double XMIN=-HALFEXT;
    const double YMAX=HALFEXT;
    const double YMIN=-HALFEXT;
    const double viewP[32] = {
      XMIN, YMIN,  0.0, 1.0,
      XMIN, YMIN,  1.0, 1.0,
      XMIN, YMAX,  0.0, 1.0,
      XMIN, YMAX,  1.0, 1.0,
      XMAX, YMIN,  0.0, 1.0,
      XMAX, YMIN,  1.0, 1.0,
      XMAX, YMAX,  0.0, 1.0,
      XMAX, YMAX,  1.0, 1.0
    };
    double frust[32];
    memcpy(frust, viewP, 32*sizeof(double));
    for (int index=0; index<8; index++)
      {
      ren->ViewToWorld(frust[index*4+0],
                       frust[index*4+1],
                       frust[index*4+2]);
      }

    this->Internal->ViewSorter->SetCameraState(camState);
    this->Internal->ViewSorter->SetFrustum(frust);
    return true;
    }

  return false;
}

//------------------------------------------------------------------------------
double vtkStreamingDriver::CalculateViewPriority(double *pbbox)
{
  return this->Internal->ViewSorter->CalculatePriority(pbbox);
}

//------------------------------------------------------------------------------
void vtkStreamingDriver::SetCacheSize(int nv)
{
  if (this->CacheSize == nv)
    {
    return;
    }
  this->CacheSize = nv;
  vtkCollection *harnesses = this->GetHarnesses();
  if (harnesses)
    {
    vtkCollectionIterator *iter = harnesses->NewIterator();
    iter->InitTraversal();
    while(!iter->IsDoneWithTraversal())
      {
      vtkStreamingHarness *harness = vtkStreamingHarness::SafeDownCast
        (iter->GetCurrentObject());
      iter->GoToNextItem();
      vtkPieceCacheFilter *pcf = harness->GetCacheFilter();
      if (pcf)
        {
        pcf->SetCacheSize(nv);
        }
      }
    iter->Delete();
    }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkStreamingDriver::CopyBackBufferToFront()
{
  vtkRenderWindow *rw = this->GetRenderWindow();
  if (!rw || rw->GetNeverRendered())
    {
    return;
    }

  //allocate pixel storage
  int *size = rw->GetSize();
  if (!this->Internal->PixelArray)
    {
    this->Internal->PixelArray = vtkUnsignedCharArray::New();
    this->Internal->PixelArray->SetNumberOfComponents(4);
    }
  if(size[0]*size[1] != this->Internal->PixelArray->GetNumberOfTuples())
    {
    this->Internal->PixelArray->SetNumberOfTuples(size[0]*size[1]);
    }

  //capture back buffer
  rw->GetRGBACharPixelData
    (0, 0, size[0]-1, size[1]-1, 0, this->Internal->PixelArray);

  //copy into the front buffer
  rw->SetRGBACharPixelData
    (0, 0, size[0]-1, size[1]-1, this->Internal->PixelArray, 1);
}
