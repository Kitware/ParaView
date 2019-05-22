/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqQVTKWidget.h"

#include <QResizeEvent>

#include "pqApplicationCore.h"
#include "pqEventDispatcher.h"
#include "pqOptions.h"
#include "pqUndoStack.h"
#include "vtkPVLogger.h"
#include "vtkRenderWindow.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"
#include "vtkSMViewProxy.h"
#include "vtksys/SystemTools.hxx"

#include <chrono>
#include <numeric>

//----------------------------------------------------------------------------
pqQVTKWidget::pqQVTKWidget(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f)
  , SizePropertyName("ViewSize")
{
  // disable HiDPI if we are running tests
  this->setEnableHiDPI(vtksys::SystemTools::GetEnv("DASHBOARD_TEST_FROM_CTEST") ? false : true);

  // if active stereo requested, then we need to request appropriate context.
  auto options = pqApplicationCore::instance()->getOptions();
  if (options->GetStereoType() && strcmp(options->GetStereoType(), "Crystal Eyes") == 0)
  {
#if PARAVIEW_USING_QVTKOPENGLWIDGET
    auto fmt = this->defaultFormat(/*supports_stereo =*/true);
    this->setFormat(fmt);
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "requesting stereo-capable context.");
#else
    vtkLogF(WARNING,
      "we do not support stereo capable context on this platform; stereo request ignored.");
#endif
  }
}

//----------------------------------------------------------------------------
pqQVTKWidget::~pqQVTKWidget()
{
}

//----------------------------------------------------------------------------
void pqQVTKWidget::setViewProxy(vtkSMProxy* view)
{
  if (view != this->ViewProxy)
  {
    this->VTKConnect->Disconnect();
    this->ViewProxy = view;
    if (view)
    {
      this->VTKConnect->Connect(
        view, vtkSMViewProxy::PrepareContextForRendering, this, SLOT(prepareContextForRendering()));
    }
  }
}

//----------------------------------------------------------------------------
void pqQVTKWidget::setSession(vtkSMSession* session)
{
  this->Session = session;
}

//----------------------------------------------------------------------------
bool pqQVTKWidget::renderVTK()
{
  return false;
}

//----------------------------------------------------------------------------
bool pqQVTKWidget::canRender()
{
  // despite our best efforts, it's possible that the paint event happens while
  // the server manager is busy processing some other request that yields
  // progress (e.g. pvcrs.UndoRedo2 test).
  // Triggering renders in that case is hazardous. So we skip calling
  // rendering in those cases.
  if (this->ViewProxy && this->ViewProxy->GetSession()->GetPendingProgress())
  {
    return false;
  }

  if (this->Session && this->Session->GetPendingProgress())
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
vtkTypeUInt32 pqQVTKWidget::getProxyId()
{
  if (this->ViewProxy)
  {
    return this->ViewProxy->GetGlobalID();
  }
  return 0;
}

//----------------------------------------------------------------------------
#if PARAVIEW_USING_QVTKOPENGLWIDGET
void pqQVTKWidget::resizeEvent(QResizeEvent* evt)
{
  this->Superclass::resizeEvent(evt);

  if (this->ViewProxy)
  {
    BEGIN_UNDO_EXCLUDE();
    // this is necessary only because resizeEvent are not propagated immediately
    // from the QWidget to an embedded QOpenGLWindow and hence the vtkRenderWindow
    // will not see updated size until Qt's event processing catches up. This can
    // cause issues with test playback since the RenderWIndow size may not be
    // correct just yet. So we manually update the render window size.
    const QSize newsize = evt->size() * this->devicePixelRatioF();
    const int inewsize[2] = { newsize.width(), newsize.height() };
    vtkSMPropertyHelper(this->ViewProxy, "ViewSize").Set(inewsize, 2);
    this->ViewProxy->UpdateVTKObjects();
    END_UNDO_EXCLUDE();
  }
}
#endif

//----------------------------------------------------------------------------
void pqQVTKWidget::prepareContextForRendering()
{
  if (!this->isVisible())
  {
    // if not visible, waiting for a bit isn't going to help with the context
    // creation at all.
    return;
  }

  auto start = std::chrono::system_clock::now();
  while (!this->isValid())
  {
    pqEventDispatcher::processEventsAndWait(10);
    auto end = std::chrono::system_clock::now();
    const std::chrono::duration<double> diff = end - start;
    if (diff.count() >= 5)
    {
      break;
    }
  }

  if (!this->isValid())
  {
    qCritical("Failed to create a valid OpenGL context for 5 seconds....giving up!");
  }
}

//----------------------------------------------------------------------------
void pqQVTKWidget::paintMousePointer(int xLocation, int yLocation)
{
  Q_UNUSED(xLocation);
  Q_UNUSED(yLocation);
  // TODO: need to add support to paint collaboration mouse pointer in Qt 5.
}
