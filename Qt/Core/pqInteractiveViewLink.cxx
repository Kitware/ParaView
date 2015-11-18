/*=========================================================================

  Program: ParaView
  Module:    pqInteractiveViewLink.cxx

  Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
  All rights reserved.

  ParaView is a free software; you can redistribute it and/or modify it
  under the terms of the ParaView license version 1.2.

  See License_v1.2.txt for the full ParaView license.
  A copy of this license can be obtained by contacting
  Kitware Inc.
  28 Corporate Drive
  Clifton Park, NY 12065
  USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "pqInteractiveViewLink.h"

// Qt Include
#include <QDebug>
#include <QPointer>
#include <QTimer>

// VTK Include
#include "vtkCommand.h"
#include "vtkImageData.h"
#include "vtkLogoWidget.h"
#include "vtkPointData.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkUnsignedCharArray.h"
#include "vtkWeakPointer.h"

// ParaView Include
#include "pqQVTKWidget.h"
#include "pqRenderView.h"
#include "vtkPVInteractiveViewLinkRepresentation.h"
#include "vtkPVRenderView.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"

static const int LAZY_DRAW_INTERVAL = 30;

class pqInteractiveViewLink::pqInternal
{
public:
  pqInternal():
    LinkWidget(vtkLogoWidget::New()),
    ViewLinkRepresentation(vtkPVInteractiveViewLinkRepresentation::New()),
    Rendering(false),
    LinkedWindowRendered(false),
    DisplayUsedCache(false)
  {
    this->ViewLinkRepresentation->ProportionalResizeOff();
    this->LazyFinalDrawTimer.setSingleShot(true);
    this->LazyFinalDrawTimer.setInterval(LAZY_DRAW_INTERVAL);
  }

  ~pqInternal()
  {
    if (this->LinkWidget != NULL)
      {
      this->LinkWidget->Delete();
      }
    if (this->ViewLinkRepresentation != NULL)
      {
      this->ViewLinkRepresentation->Delete();
      }
  }

  vtkLogoWidget* LinkWidget;
  vtkPVInteractiveViewLinkRepresentation* ViewLinkRepresentation;

  QPointer<pqRenderView> DisplayView;
  QPointer<pqQVTKWidget> DisplayWidget;
  vtkPVRenderView* DisplayPVView;
  vtkWeakPointer<vtkRenderWindow> DisplayWindow;

  QPointer<pqRenderView> LinkedView;
  vtkPVRenderView* LinkedPVView;
  vtkWeakPointer<vtkRenderWindow> LinkedWindow;

  QTimer LazyFinalDrawTimer;
  unsigned long int ObserverTag;
  unsigned long int RenderedTag;
  bool Rendering;
  bool LinkedWindowRendered;
  bool DisplayUsedCache;
};

//-----------------------------------------------------------------------------
pqInteractiveViewLink::pqInteractiveViewLink(pqRenderView* displayView,
  pqRenderView* linkedView, double xPos, double yPos, double xSize, double ySize)
{
  this->Internal = new pqInternal;

  // Initialize Display View Pointers
  this->Internal->DisplayView = displayView;
  this->Internal->DisplayWidget = qobject_cast<pqQVTKWidget*>(
    this->Internal->DisplayView->widget());
  this->Internal->DisplayPVView = vtkPVRenderView::SafeDownCast(
    displayView->getViewProxy()->GetClientSideView());
  this->Internal->DisplayWindow = displayView->getRenderViewProxy()->GetRenderWindow();

  // Initialize Linked View Pointers
  this->Internal->LinkedView = linkedView;
  this->Internal->LinkedPVView = vtkPVRenderView::SafeDownCast(
    linkedView->getViewProxy()->GetClientSideView());
  this->Internal->LinkedWindow = linkedView->getRenderViewProxy()->GetRenderWindow();

  // Sanity check
  if (!this->Internal->DisplayWidget || !this->Internal->LinkedView->widget())
    {
    qCritical() << "Cannot Create pqInteractiveViewLink without view widgets";
    return;
    }

  // Disable cache on display
  this->Internal->DisplayUsedCache =
    this->Internal->DisplayWidget->isAutomaticImageCacheEnabled();
  this->Internal->DisplayWidget->setAutomaticImageCacheEnabled(false);

  // Initialize link widget interactor and renderer
  this->Internal->LinkWidget->SetInteractor(vtkSMRenderViewProxy::SafeDownCast(
    displayView->getViewProxy())->GetInteractor());
  this->Internal->LinkWidget->SetCurrentRenderer(
    vtkPVRenderView::SafeDownCast(displayView->getViewProxy()->GetClientSideView())
    ->GetNonCompositedRenderer());

  this->Internal->ViewLinkRepresentation->SetPosition(xPos, yPos);
  this->Internal->ViewLinkRepresentation->SetPosition2(xSize, ySize);
  this->Internal->ViewLinkRepresentation->GetImageProperty()->SetOpacity(0);

  vtkNew<vtkImageData> image;
  image->SetExtent(0, 0, 0, 0, 0, 0);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  this->Internal->ViewLinkRepresentation->SetImage(image.Get());
  this->Internal->LinkWidget->SetRepresentation(this->Internal->ViewLinkRepresentation);
  this->Internal->LinkWidget->On();

  // linked render end Observer, in order to be sure there is pixels to get in linked 
  // render window
  this->Internal->ObserverTag =
    this->Internal->LinkedWindow->AddObserver(vtkCommand::EndEvent, this,
      &pqInteractiveViewLink::linkedWindowRendered);

  // render Observer, draw on the frame just before it is rendered
  this->Internal->RenderedTag =
    this->Internal->DisplayWindow->AddObserver(vtkCommand::RenderEvent, this,
      &pqInteractiveViewLink::drawViewLink);

  // We need to use lazy draw to ensure the correct frame is finally rendered
  QObject::connect(&this->Internal->LazyFinalDrawTimer, SIGNAL(timeout()), this,
                   SLOT(finalDrawViewLink()));

  // Initial Render
  this->Internal->DisplayView->render();
}

//-----------------------------------------------------------------------------
pqInteractiveViewLink::~pqInteractiveViewLink()
{
  if (this->Internal->DisplayWindow)
    {
    this->Internal->DisplayWindow->RemoveObserver(this->Internal->ObserverTag);
    }

  if (this->Internal->DisplayWidget)
    {
    this->Internal->DisplayWidget->setAutomaticImageCacheEnabled(
      this->Internal->DisplayUsedCache);
    }

  if (this->Internal->DisplayView)
    {
    this->Internal->DisplayView->render();
    }
  if (this->Internal->LinkedView)
    {
    this->Internal->LinkedView->render();
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqInteractiveViewLink::saveXMLState(vtkPVXMLElement* xml)
{
  xml->AddAttribute("DisplayViewProxy", vtkSMRenderViewProxy::SafeDownCast(
    this->Internal->DisplayView->getViewProxy())->GetGlobalID());
  xml->AddAttribute("LinkedViewProxy", vtkSMRenderViewProxy::SafeDownCast(
    this->Internal->LinkedView->getViewProxy())->GetGlobalID());

  double* position = this->Internal->ViewLinkRepresentation->GetPosition();
  double* size = this->Internal->ViewLinkRepresentation->GetPosition2();
  xml->AddAttribute("positionX", position[0]);
  xml->AddAttribute("positionY", position[1]);
  xml->AddAttribute("sizeX", size[0]);
  xml->AddAttribute("sizeY", size[1]);
}

//-----------------------------------------------------------------------------
void pqInteractiveViewLink::renderLinkedView()
{
  if (!this->Internal->Rendering &&
      this->Internal->LinkedView != NULL &&
      this->Internal->LinkedPVView != NULL &&
      this->Internal->DisplayPVView != NULL)
    {
    if (this->Internal->LinkedPVView->GetUseDistributedRenderingForStillRender() ||
      this->Internal->DisplayPVView->GetUseDistributedRenderingForStillRender())
      {
      qCritical() << "Something went wrong, remote rendering should not use "
        "pqInteractiveViewLink::renderLinkedView method";
      return;
      }

    this->Internal->Rendering = true;
    // We need to render the hidden view here
    // The good practice would be to call LinkedView->forceRender()
    // in order to make sure there is no tag mismatch between server and client
    // in remote rendering. But this using a LinkedView->Render cann trigger 
    // a render loop if multiple interactive view link are used, because of the camera
    // link transmitting render from on view to the next, and then back, causing the render
    // loop.
    // Calling LinkedWindow->Render which does not trigger camera link does not cause any problem
    // considering that this render is only necessary and done when the linked view is hidden in LOCAL
    // rendering only. 
    this->Internal->LinkedPVView->SetUpdateAnnotation(false);
    this->Internal->LinkedWindow->Render();
    this->Internal->LinkedPVView->SetUpdateAnnotation(true);
    this->Internal->Rendering = false;
    }
}

//-----------------------------------------------------------------------------
void pqInteractiveViewLink::drawViewLink()
{
  this->drawViewLink(0);

  // Trigger/Reset timer for the final draw
  this->Internal->LazyFinalDrawTimer.start();
}

//-----------------------------------------------------------------------------
void pqInteractiveViewLink::finalDrawViewLink()
{
  this->drawViewLink(1);
}

//-----------------------------------------------------------------------------
void pqInteractiveViewLink::drawViewLink(int setFront)
{
  if (!this->Internal->LinkedWindow ||
      !this->Internal->DisplayWindow ||
      !this->Internal->LinkedWindowRendered)
    {
    return;
    }
 
  bool visible = this->Internal->LinkedView->widget()->isVisible();
  bool remoteRendering =
    this->Internal->LinkedPVView->GetUseDistributedRenderingForStillRender() ||
    this->Internal->DisplayPVView->GetUseDistributedRenderingForStillRender();
    
  // getFront is true when remoteRendering a non-visible linked view, false otherwise
  bool getFront = !remoteRendering || visible;
 
  // Render the linked view offscreen is necessary 
  // only when non-remote-rendering a non-visible linked view
  bool render = !visible && !remoteRendering;

  // Recover window sizes 
  int* linkedSize = this->Internal->LinkedWindow->GetActualSize();
  int* displaySize = this->Internal->DisplayWindow->GetActualSize();

  // Recover ViewLink representation position and size
  double* pos = this->Internal->ViewLinkRepresentation->GetPosition();
  double* pos2 = this->Internal->ViewLinkRepresentation->GetPosition2();

  // Compute the position and size in pixels of the ViewLink representation
  // on the display window
  int displayPos[2];
  int displayPos2[2];
  displayPos[0] = pos[0] * (displaySize[0] - 1) + 1;
  displayPos[1] = pos[1] * (displaySize[1] - 1) + 1;
  displayPos2[0] = pos2[0] * (displaySize[0] - 1) - 2;
  displayPos2[1] = pos2[1] * (displaySize[1] - 1) - 2;

  // Check size are bigger than 1
  if (displayPos2[0] <= 1 || displayPos2[1] <= 1)
    {
    return;
    }

  // Switch to offscreen buffer if rendering
  if (render)
    {
    this->Internal->LinkedWindow->SetUseOffScreenBuffers(true);
    }

  // Compute adapted position and size of the pixel needed on the
  // linked window
  int linkedPos[2];
  int linkedPos2[2];
  linkedPos[0] = displayPos[0] * (linkedSize[0] - 1) / (displaySize[0] - 1);
  linkedPos[1] = displayPos[1] * (linkedSize[1] - 1) / (displaySize[1] - 1);
  linkedPos2[0] = displayPos2[0] * (linkedSize[0] - 1) / (displaySize[0] - 1);
  linkedPos2[1] = displayPos2[1] * (linkedSize[1] - 1) / (displaySize[1] - 1);

  // Allocate pixels data to get
  unsigned char * linkedPixels = new unsigned char[linkedSize[0] * linkedSize[1] * 3];

  // Render image if necessary
  if (render)
    {
    this->renderLinkedView();
    }

  // Get pixels from buffer
  linkedPixels = this->Internal->LinkedWindow->GetPixelData(0, 0,
    linkedSize[0] - 1, linkedSize[1] - 1, getFront);

  // Allocate pixels data to display
  int nPixels = displayPos2[0] * displayPos2[1];
  unsigned char * displayPixels = new unsigned char[nPixels * 3];

  // Fill each pixel to display using a pixel from linked view
  for (int i = 0; i < nPixels; i++)
    {
    // Actual position of pixel in the linked view
    int linkPixIdxX = ((i % displayPos2[0]) * linkedPos2[0]) / displayPos2[0];
    int linkPixIdxY = ((i / displayPos2[0]) * linkedPos2[1]) / displayPos2[1];
    int linkedPixIdx = ((linkPixIdxY + linkedPos[1]) * linkedSize[0] + (linkedPos[0] + linkPixIdxX))*3;

    // Copy pixel
    displayPixels[i * 3] = linkedPixels[linkedPixIdx];
    displayPixels[i * 3 + 1] = linkedPixels[linkedPixIdx + 1];
    displayPixels[i * 3 + 2] = linkedPixels[linkedPixIdx + 2];
    }

  // Set the pixel data on display window
  this->Internal->DisplayWindow->SetPixelData(displayPos[0], displayPos[1], 
    displayPos[0] + displayPos2[0] - 1, displayPos[1] + displayPos2[1] - 1, 
    displayPixels, setFront);

  delete [] linkedPixels;
  delete [] displayPixels;

  // Set offscreen buffers back
  if (render)
    {
    this->Internal->LinkedWindow->SetUseOffScreenBuffers(false);
    }
}

//-----------------------------------------------------------------------------
void pqInteractiveViewLink::linkedWindowRendered()
{
  this->Internal->LinkedWindowRendered = true;
  this->Internal->LinkedWindow->RemoveObserver(this->Internal->RenderedTag);
}
