/*=========================================================================

Program:   ParaView
Module:    vtkPVDesktopDeliveryServer.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVDesktopDeliveryServer.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"

#include "vtkImageCompressor.h"
#include "vtkSquirtCompressor.h"
#include "vtkZlibImageCompressor.h"

#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#include <vtkstd/map>

//-----------------------------------------------------------------------------
static void SatelliteStartParallelRender(vtkObject *caller,
                                         unsigned long vtkNotUsed(event),
                                         void *clientData, void *);
static void SatelliteEndParallelRender(vtkObject *caller,
                                       unsigned long vtkNotUsed(event),
                                       void *clientData, void *);

static void UseRendererSet(void *localArg, void *remoteArg, int, int);

//-----------------------------------------------------------------------------

typedef vtkstd::map<int, vtkSmartPointer<vtkRendererCollection> > RendererMapType;
class vtkPVDesktopDeliveryServerRendererMap
{
public:
  RendererMapType Renderers;
};

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkPVDesktopDeliveryServer);

//----------------------------------------------------------------------------
vtkPVDesktopDeliveryServer::vtkPVDesktopDeliveryServer()
{
  this->ParallelRenderManager = NULL;
  this->RemoteDisplay = 1;

  this->RendererMap = new vtkPVDesktopDeliveryServerRendererMap;

  this->SendImageBuffer = vtkUnsignedCharArray::New();

  this->Renderers->Delete();
  this->Renderers = NULL;

  this->WindowIdRMIId = 0;
  this->ReducedZBuffer = 0;
  this->AnnotationLayerVisible = 1;

  // The other process is the root process.
  this->RootProcessId = 1;
}

//----------------------------------------------------------------------------
vtkPVDesktopDeliveryServer::~vtkPVDesktopDeliveryServer()
{
  this->SetParallelRenderManager(NULL);

  delete this->RendererMap;

  this->SendImageBuffer->Delete();

  this->Renderers = NULL;

  if (this->Controller && this->AddedRMIs)
    {
    this->Controller->RemoveRMI(this->WindowIdRMIId);
    this->WindowIdRMIId = 0;
    }
  if (this->ReducedZBuffer)
    {
    this->ReducedZBuffer->Delete();
    this->ReducedZBuffer=0;
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::SetParallelRenderManager(
                                                  vtkParallelRenderManager *prm)
{
  if (this->ParallelRenderManager == prm)
    {
    return;
    }

  if (this->ParallelRenderManager)
    {
    // Remove all observers.
    this->ParallelRenderManager->RemoveObserver(this->StartParallelRenderTag);
    this->ParallelRenderManager->RemoveObserver(this->EndParallelRenderTag);
    this->StartParallelRenderTag = 0;
    this->EndParallelRenderTag = 0;
    }

  vtkSetObjectBodyMacro(ParallelRenderManager, vtkParallelRenderManager, prm);

  if (this->ParallelRenderManager)
    {
    if (this->RemoteDisplay)
      {
      // No need to write the image back on the render server.
      this->ParallelRenderManager->WriteBackImagesOff();
      }
    else
      {
      // Presumably someone is viewing the remote screen, perhaps in a
      // tile display.
      this->ParallelRenderManager->WriteBackImagesOn();
      }

    // Attach observers.
    vtkCallbackCommand *cbc;

    cbc = vtkCallbackCommand::New();
    cbc->SetCallback(::SatelliteStartParallelRender);
    cbc->SetClientData(this);
    this->StartParallelRenderTag
      = this->ParallelRenderManager->AddObserver(vtkCommand::StartEvent, cbc);
    // ParallelRenderManager will really delete cbc when observer is removed.
    cbc->Delete();

    cbc = vtkCallbackCommand::New();
    cbc->SetCallback(::SatelliteEndParallelRender);
    cbc->SetClientData(this);
    this->EndParallelRenderTag
      = this->ParallelRenderManager->AddObserver(vtkCommand::EndEvent, cbc);
    // ParallelRenderManager will really delete cbc when observer is removed.
    cbc->Delete();

    // Remove observers to RenderWindow.  We use the prm instead.
    this->RemoveRenderWindowEventHandlers();
    }
  else
    {
    // Apparently we added and then removed a ParallelRenderManager.
    // Restore RenderWindow observers.
    this->AddRenderWindowEventHandlers();
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::SetRenderWindow(vtkRenderWindow *renWin)
{
  this->Superclass::SetRenderWindow(renWin);
  if (this->ParallelRenderManager)
    {
    // If ParallelRenderManager is present, we don't want to be listening to
    // render events from the render window.
    this->RemoveRenderWindowEventHandlers(); 
    }
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::InitializeRMIs()
{
  if (!this->AddedRMIs)
    {
    this->Superclass::InitializeRMIs();

    this->WindowIdRMIId = this->Controller->AddRMI(
                             ::UseRendererSet, this,
                             vtkPVDesktopDeliveryServer::WINDOW_ID_RMI_TAG);
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::SetRemoteDisplay(int flag)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting RemoteDisplay to " << flag);
  if (this->RemoteDisplay != flag)
    {
    this->RemoteDisplay = flag;
    this->Modified();

    if (this->ParallelRenderManager)
      {
      if (this->RemoteDisplay)
        {
        // No need to write the image back on the render server.
        this->ParallelRenderManager->WriteBackImagesOff();
        }
      else
        {
        // Presumably someone is viewing the remote screen, perhaps in a
        // tile display.
        this->ParallelRenderManager->WriteBackImagesOn();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::UseRendererSet(int id)
{
  if (!this->SyncRenderWindowRenderers)
    {
    this->Renderers = this->RendererMap->Renderers[id];

    vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
    vtkCollectionSimpleIterator cookie;
    vtkRenderer *ren;
    for (rens->InitTraversal(cookie);
         (ren = rens->GetNextRenderer(cookie)) != NULL; )
      {
      // Turn off everything that is not annotation.  The superclass will
      // later turn on any renderers that the client requested.
      if (ren->GetLayer() >= this->AnnotationLayer && 
        this->AnnotationLayerVisible)
        {
        ren->DrawOn();
        }
      else
        {
        ren->DrawOff();
        }
      }
    }
}

//----------------------------------------------------------------------------
bool vtkPVDesktopDeliveryServer::ProcessWindowInformation(
  vtkMultiProcessStream& stream)
{
  if (!this->Superclass::ProcessWindowInformation(stream))
    {
    return false;
    }

  vtkPVDesktopDeliveryServer::WindowGeometry winGeoInfo;
  if (!winGeoInfo.Restore(stream))
    {
    vtkErrorMacro("Failed to read WindowGeometry info.");
    return false;
    }

  // Correct window size.
  this->ClientWindowSize[0] = this->FullImageSize[0];
  this->ClientWindowSize[1] = this->FullImageSize[1];
  if (winGeoInfo.ViewSize[0] > 0 && winGeoInfo.ViewSize[1] > 0)
    {
    // The ViewSize is non-zero only in tile-display mode. In that case use the
    // ViewSize as that's the size after the window borders have been compacted.
    this->ClientWindowSize[0] = winGeoInfo.ViewSize[0];
    this->ClientWindowSize[1] = winGeoInfo.ViewSize[1];
    }
  this->ClientRequestedImageSize[0] = this->ReducedImageSize[0];
  this->ClientRequestedImageSize[1] = this->ReducedImageSize[1];
  this->FullImageSize[0] = winGeoInfo.GUISize[0];
  this->FullImageSize[1] = winGeoInfo.GUISize[1];
  this->ReducedImageSize[0]
    = (int)(this->FullImageSize[0]/this->ImageReductionFactor);
  this->ReducedImageSize[1]
    = (int)(this->FullImageSize[1]/this->ImageReductionFactor);
  this->ClientWindowPosition[0] = winGeoInfo.Position[0];
  this->ClientWindowPosition[1] = winGeoInfo.Position[1];
  this->ClientGUISize[0] = winGeoInfo.GUISize[0];
  this->ClientGUISize[1] = winGeoInfo.GUISize[1];

  this->AnnotationLayer = winGeoInfo.AnnotationLayer;

  this->UseRendererSet(winGeoInfo.Id);

  return true;
}

//-----------------------------------------------------------------------------
bool vtkPVDesktopDeliveryServer::ProcessRendererInformation(vtkRenderer *ren,
  vtkMultiProcessStream& stream)
{
  if (!this->Superclass::ProcessRendererInformation(ren, stream))
    {
    return false;
    }

  // Get the original viewport from the client.
  double viewport[4];
  stream >> viewport[0] >> viewport[1] >> viewport[2] >> viewport[3];

  double scaleX = (double)this->ClientWindowSize[0]/this->ClientGUISize[0];
  double scaleY = (double)this->ClientWindowSize[1]/this->ClientGUISize[1];
  viewport[0] *= scaleX;
  viewport[1] *= scaleY;
  viewport[2] *= scaleX;
  viewport[3] *= scaleY;

  double offsetX = (double)this->ClientWindowPosition[0]/this->ClientGUISize[0];
  double offsetY = (double)this->ClientWindowPosition[1]/this->ClientGUISize[1];
  if (!this->ParallelRenderManager && (this->ImageReductionFactor > 1.0))
    {
    offsetX /= this->ImageReductionFactor;
    offsetY /= this->ImageReductionFactor;
    }
  viewport[0] += offsetX;
  viewport[1] += offsetY;
  viewport[2] += offsetX;
  viewport[3] += offsetY;

  ren->SetViewport(viewport);
  return true;
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::PreRenderProcessing()
{
  vtkDebugMacro("PreRenderProcessing");

  // Send remote display flag.
  this->Controller->Send(&this->RemoteDisplay, 1, this->RootProcessId,
                         vtkPVDesktopDeliveryServer::REMOTE_DISPLAY_TAG);

  if (this->ParallelRenderManager)
    {
    // Make sure the prm has the correct image reduction factor.
    if (  this->ParallelRenderManager->GetMaxImageReductionFactor()
          < this->ImageReductionFactor)
      {
      this->ParallelRenderManager
        ->SetMaxImageReductionFactor(this->ImageReductionFactor);
      }
    this->ParallelRenderManager
      ->SetImageReductionFactor(this->ImageReductionFactor);
    this->ParallelRenderManager->AutoImageReductionFactorOff();

    // Pass UseCompositing flag.
    this->ParallelRenderManager->SetUseCompositing(this->UseCompositing);
    }

  this->ImageResized = 0;
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::PostRenderProcessing()
{
  vtkDebugMacro("PostRenderProcessing");
  vtkTimerLog::MarkStartEvent("Sending");

  vtkPVDesktopDeliveryServer::ImageParams ip;
  ip.RemoteDisplay = this->RemoteDisplay;

  if (ip.RemoteDisplay)
    {
    this->ReadReducedImage();
    ip.NumberOfComponents = this->ReducedImage->GetNumberOfComponents();
    if (   (this->ClientWindowSize[0] == this->ClientGUISize[0])
        && (this->ClientWindowSize[1] == this->ClientGUISize[1]) )
      {
      ip.ImageSize[0] = this->ReducedImageSize[0];
      ip.ImageSize[1] = this->ReducedImageSize[1];
      this->SendImageBuffer->SetArray(this->ReducedImage->GetPointer(0),
                                      ip.ImageSize[0]*ip.ImageSize[1]
                                      *ip.NumberOfComponents, 1);
      this->SendImageBuffer->SetNumberOfComponents(ip.NumberOfComponents);
      this->SendImageBuffer->SetNumberOfTuples(ip.ImageSize[0]*ip.ImageSize[1]);
      }
    else
      {
      // Grab a subset of the server's window that corresponds to the window on
      // the client.
      if (   (this->ClientGUISize[0] == this->FullImageSize[0])
          && !this->ImageResized )
        {
        // Window was not resized.  Return image size requested.
        ip.ImageSize[0] = this->ClientRequestedImageSize[0];
        ip.ImageSize[1] = this->ClientRequestedImageSize[1];
        }
      else
        {
        ip.ImageSize[0]
          = (  (this->ReducedImageSize[0]*this->ClientWindowSize[0])
             / this->ClientGUISize[0] );
        ip.ImageSize[1]
          = (  (this->ReducedImageSize[1]*this->ClientWindowSize[1])
             / this->ClientGUISize[1] );
        }
      int left   = (  (this->ReducedImageSize[0]*this->ClientWindowPosition[0])
                    / this->ClientGUISize[0] );
      int bottom = (  (this->ReducedImageSize[1]*this->ClientWindowPosition[1])
                    / this->ClientGUISize[1] );

      this->SendImageBuffer->Initialize();
      this->SendImageBuffer->SetNumberOfComponents(ip.NumberOfComponents);
      this->SendImageBuffer->SetNumberOfTuples(ip.ImageSize[0]*ip.ImageSize[1]);
      for (int i = 0; i < ip.ImageSize[1]; i++)
        {
        int destPos = i*ip.ImageSize[0]*ip.NumberOfComponents;
        int srcPos
          = (left + (bottom+i)*this->ReducedImageSize[0])*ip.NumberOfComponents;
        memcpy(this->SendImageBuffer->GetPointer(destPos),
               this->ReducedImage->GetPointer(srcPos),
               ip.ImageSize[0]*ip.NumberOfComponents);
        }
      }

    // ip.SquirtCompressed = this->Squirt && (ip.NumberOfComponents == 4);
    // if (ip.SquirtCompressed)
    if (this->CompressionEnabled)
      {
      this->Compressor->SetLossLessMode(this->LossLessCompression);
      this->Compressor->SetInput(this->SendImageBuffer);
      this->Compressor->SetOutput(this->CompressorBuffer);
      this->Compressor->Compress();
      this->Compressor->SetInput(0);
      this->Compressor->SetOutput(0);

      ip.NumberOfComponents=this->SendImageBuffer->GetNumberOfComponents();
      ip.BufferSize=this->CompressorBuffer->GetNumberOfTuples();

      this->Controller->Send(reinterpret_cast<int *>(&ip),
                             IMAGE_PARAMS_SIZE,
                             this->RootProcessId,
                             IMAGE_PARAMS_TAG);

      this->Controller->Send(this->CompressorBuffer->GetPointer(0),
                             ip.BufferSize,
                             this->RootProcessId,
                             IMAGE_TAG);
      }
    else
      {
      ip.BufferSize
        = ip.NumberOfComponents*this->SendImageBuffer->GetNumberOfTuples();
      this->Controller->Send(reinterpret_cast<int *>(&ip),
                             vtkPVDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
                             this->RootProcessId,
                             vtkPVDesktopDeliveryServer::IMAGE_PARAMS_TAG);
      this->Controller->Send(this->SendImageBuffer->GetPointer(0), ip.BufferSize,
                             this->RootProcessId,
                             vtkPVDesktopDeliveryServer::IMAGE_TAG);
      }
    }
  else
    {
    this->Controller->Send(reinterpret_cast<int *>(&ip),
                           vtkPVDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
                           this->RootProcessId,
                           vtkPVDesktopDeliveryServer::IMAGE_PARAMS_TAG);
    }

  // Send timing metrics
  vtkPVDesktopDeliveryServer::TimingMetrics tm;
  if (this->ParallelRenderManager)
    {
    tm.ImageProcessingTime
      = this->ParallelRenderManager->GetImageProcessingTime();
    }
  else
    {
    tm.ImageProcessingTime = 0.0;
    }
  this->Controller->Send(reinterpret_cast<double *>(&tm),
                         vtkPVDesktopDeliveryServer::TIMING_METRICS_SIZE,
                         this->RootProcessId,
                         vtkPVDesktopDeliveryServer::TIMING_METRICS_TAG);

  // If another parallel render manager has already made an image, don't
  // clober it.
  if (this->ParallelRenderManager)
    {
    this->RenderWindowImageUpToDate = 1;
    }

  vtkTimerLog::MarkEndEvent("Sending");
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::AddRenderer(int id, vtkRenderer *ren)
{
  if (this->RendererMap->Renderers[id].GetPointer() == NULL)
    {
    this->RendererMap->Renderers[id]
      = vtkSmartPointer<vtkRendererCollection>::New();
    }
  this->RendererMap->Renderers[id]->AddItem(ren);
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::RemoveRenderer(int id, vtkRenderer *ren)
{
  RendererMapType::iterator iter = this->RendererMap->Renderers.find(id);
  if (iter != this->RendererMap->Renderers.end())
    {
    iter->second->RemoveItem(ren);
    if (iter->second->GetNumberOfItems() < 1)
      {
      this->RendererMap->Renderers.erase(iter);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::RemoveAllRenderers(int id)
{
  this->RendererMap->Renderers.erase(id);
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::SetRenderWindowSize()
{
  if (this->RemoteDisplay)
    {
    this->Superclass::SetRenderWindowSize();
    // vtkXOpenGLRenderWindow::GetSize() called right after
    // vtkXOpenGLRenderWindow::SetSize() might return the wrong
    // value due to what seems to be a bug in X. Instead of
    // relying on that, set the size explicitly
    if (this->ParallelRenderManager)
      {
      this->ParallelRenderManager->SetForceRenderWindowSize(1);
      this->ParallelRenderManager->SetForcedRenderWindowSize(
        this->FullImageSize[0], this->FullImageSize[1]);
      }
    }
  else
    {
    int *size = this->RenderWindow->GetActualSize();
    this->FullImageSize[0] = size[0];
    this->FullImageSize[1] = size[1];
    this->ReducedImageSize[0] = (int)(size[0]/this->ImageReductionFactor);
    this->ReducedImageSize[1] = (int)(size[1]/this->ImageReductionFactor);
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::ReadReducedImage()
{
  if (this->ParallelRenderManager)
    {
    int *size = this->ParallelRenderManager->GetReducedImageSize();
    if ( this->ReducedImageSize[0] != size[0]
      || this->ReducedImageSize[1] != size[1] )
      {
      vtkDebugMacro(<< "Coupled parallel render manager reports unexpected reduced image size\n"
                    << "Expected size: " << this->ReducedImageSize[0] << " "
                    << this->ReducedImageSize[1] << "\n"
                    << "Reported size: " << size[0] << " " << size[1]);
      if ( this->ReducedImageSize[0] == this->FullImageSize[0]
        && this->ReducedImageSize[1] == this->FullImageSize[1] )
        {
        vtkWarningMacro(<< "The coupled render manager has apparently resized the window.\n"
                        << "Operation will still work normally, but the client may waste many cycles\n"
                        << "resizing the resulting window.");
        }
      this->ReducedImageSize[0] = size[0];
      this->ReducedImageSize[1] = size[1];
      }
    this->ParallelRenderManager->GetReducedPixelData(this->ReducedImage);
    this->ReducedImageUpToDate = 1;
    }
  else
    {
    this->Superclass::ReadReducedImage();
    if (this->CaptureZBuffer)
      {
      this->ReducedZBuffer = this->ReducedZBuffer? 
        this->ReducedZBuffer : vtkFloatArray::New();
      this->RenderWindow->GetZbufferData(0, 0, this->ReducedImageSize[0]-1,
        this->ReducedImageSize[1]-1, this->ReducedZBuffer);
      }
    else if (this->ReducedZBuffer)
      {
      this->ReducedZBuffer->Delete();
      this->ReducedZBuffer=0;
      }
    }
}

//-----------------------------------------------------------------------------
float vtkPVDesktopDeliveryServer::GetZBufferValue(int x, int y)
{
  if (this->ParallelRenderManager)
    {
    vtkErrorMacro("When running in parallel, ask the IceTRenderManager "
      "for Z buffer value.");
    return 0.0f;
    }
  
  int width = this->ReducedImageSize[0];
  int height = this->ReducedImageSize[1]; 
  
  if (x>=0 && y>=0 && x <width && y < height)
    {
    int index = y* (width) + x;
    if (index < this->ReducedZBuffer->GetNumberOfTuples())
      {
      return this->ReducedZBuffer->GetValue(index);
      }
    }

  return 1.0f;
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::LocalComputeVisiblePropBounds(vtkRenderer *ren,
                                                             double bounds[6])
{
  if (this->ParallelRenderManager)
    {
    this->ParallelRenderManager->ComputeVisiblePropBounds(ren, bounds);
    }
  else
    {
    this->Superclass::LocalComputeVisiblePropBounds(ren, bounds);
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ParallelRenderManager: "
     << this->ParallelRenderManager << endl;
  os << indent << "RemoteDisplay: "
     << (this->RemoteDisplay ? "on" : "off") << endl;
  os << indent << "AnnotationLayerVisible: " 
    << this->AnnotationLayerVisible << endl;
}

//----------------------------------------------------------------------------
static void SatelliteStartParallelRender(vtkObject *caller,
                                         unsigned long vtkNotUsed(event),
                                         void *clientData, void *)
{
  vtkPVDesktopDeliveryServer *self
    = reinterpret_cast<vtkPVDesktopDeliveryServer *>(clientData);
  if (caller != self->GetParallelRenderManager())
    {
    vtkGenericWarningMacro("vtkPVDesktopDeliveryServer caller mismatch");
    return;
    }
  self->SatelliteStartRender();
}

//----------------------------------------------------------------------------
static void SatelliteEndParallelRender(vtkObject *caller,
                                       unsigned long vtkNotUsed(event),
                                       void *clientData, void *)
{
  vtkPVDesktopDeliveryServer *self
    = reinterpret_cast<vtkPVDesktopDeliveryServer *>(clientData);
  if (caller != self->GetParallelRenderManager())
    {
    vtkGenericWarningMacro("vtkPVDesktopDeliveryServer caller mismatch");
    return;
    }
  self->SatelliteEndRender();
}

//-----------------------------------------------------------------------------
static void UseRendererSet(void *localArg, void *remoteArg, int, int)
{
  vtkPVDesktopDeliveryServer *self
    = reinterpret_cast<vtkPVDesktopDeliveryServer *>(localArg);
  self->UseRendererSet(*reinterpret_cast<int*>(remoteArg));
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryServer::WindowGeometry::Save(vtkMultiProcessStream& stream)
{
  stream << vtkPVDesktopDeliveryServer::WINDOW_GEOMETRY_TAG;
  stream << this->Position[0] << this->Position[1]
    << this->GUISize[0] << this->GUISize[1]
    << this->ViewSize[0] << this->ViewSize[1]
    << this->Id
    << this->AnnotationLayer;
}

//-----------------------------------------------------------------------------
bool vtkPVDesktopDeliveryServer::WindowGeometry::Restore(vtkMultiProcessStream& stream)
{
  int tag;
  stream >> tag;
  if (tag != vtkPVDesktopDeliveryServer::WINDOW_GEOMETRY_TAG)
    {
    return false;
    }
  stream >> this->Position[0] >> this->Position[1]
    >> this->GUISize[0] >> this->GUISize[1]
    >> this->ViewSize[0] >> this->ViewSize[1]
    >> this->Id
    >> this->AnnotationLayer;
  return true;
}

