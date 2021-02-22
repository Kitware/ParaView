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

#include <QApplication>
#include <QResizeEvent>
#include <QVBoxLayout>

#include "QVTKOpenGLNativeWidget.h"
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
  : pqQVTKWidget(
      parentObject, f, pqApplicationCore::instance()->getOptions()->GetUseStereoRendering())
{
}

//----------------------------------------------------------------------------
pqQVTKWidget::pqQVTKWidget(QWidget* parentObject, Qt::WindowFlags f, bool isStereo)
  : Superclass(parentObject, f)
  , SizePropertyName("ViewSize")
  , useStereo(isStereo)
{
  auto options = pqApplicationCore::instance()->getOptions();
  auto* layout = new QVBoxLayout();
  layout->setMargin(0);

  if (useStereo)
  {
    auto ptr = new pqQVTKWidgetBase(parentObject, f);
    baseClass.setValue(ptr);
    layout->addWidget(ptr);

    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "Using stereo-capable context.");
  }
  else
  {
    auto ptr = new QVTKOpenGLNativeWidget(parentObject, f);
    baseClass.setValue(ptr);
    layout->addWidget(ptr);

    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "Using native-capable context.");
  }

  this->setLayout(layout);

  // disable HiDPI if we are running tests
  if (this->useStereo)
  {
    baseClass.value<pqQVTKWidgetBase*>()->setEnableHiDPI(
      vtksys::SystemTools::GetEnv("DASHBOARD_TEST_FROM_CTEST") ? false : true);
  }
  else
  {
    baseClass.value<QVTKOpenGLNativeWidget*>()->setEnableHiDPI(
      vtksys::SystemTools::GetEnv("DASHBOARD_TEST_FROM_CTEST") ? false : true);
  }

  // if active stereo requested, then we need to request appropriate context.
  if (options->GetStereoType() && strcmp(options->GetStereoType(), "Crystal Eyes") == 0)
  {
#if PARAVIEW_USING_QVTKOPENGLSTEREOWIDGET
    auto fmt = baseClass.value<pqQVTKWidgetBase*>()->defaultFormat(/*supports_stereo =*/true);
    baseClass.value<pqQVTKWidgetBase*>()->setFormat(fmt);
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "requesting stereo-capable context.");
#else
    vtkLogF(WARNING,
      "we do not support stereo capable context on this platform; stereo request ignored.");
#endif
  }
}

//----------------------------------------------------------------------------
pqQVTKWidget::~pqQVTKWidget() = default;

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
#if PARAVIEW_USING_QVTKOPENGLSTEREOWIDGET
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
    const QSize newsize = evt->size() * this->effectiveDevicePixelRatio();
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
  bool ret = false;

  if (this->useStereo)
  {
    ret = baseClass.value<pqQVTKWidgetBase*>()->isValid();
  }
  else
  {
    ret = baseClass.value<QVTKOpenGLNativeWidget*>()->isValid();
  }

  while (!ret)
  {
    pqEventDispatcher::processEventsAndWait(10);
    auto end = std::chrono::system_clock::now();
    const std::chrono::duration<double> diff = end - start;
    if (diff.count() >= 5)
    {
      break;
    }

    if (this->useStereo)
    {
      ret = baseClass.value<pqQVTKWidgetBase*>()->isValid();
    }
    else
    {
      ret = baseClass.value<QVTKOpenGLNativeWidget*>()->isValid();
    }
  }

  if (this->useStereo)
  {
    ret = baseClass.value<pqQVTKWidgetBase*>()->isValid();
  }
  else
  {
    ret = baseClass.value<QVTKOpenGLNativeWidget*>()->isValid();
  }

  if (!ret)
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

//----------------------------------------------------------------------------
void pqQVTKWidget::setRenderWindow(vtkRenderWindow* win)
{
  if (this->useStereo)
  {
    baseClass.value<pqQVTKWidgetBase*>()->setUnscaledDPI(win->GetDPI());
    baseClass.value<pqQVTKWidgetBase*>()->setRenderWindow(win);
  }
  else
  {
    baseClass.value<QVTKOpenGLNativeWidget*>()->setUnscaledDPI(win->GetDPI());
    baseClass.value<QVTKOpenGLNativeWidget*>()->setRenderWindow(win);
  }
}

//----------------------------------------------------------------------------
QVTKInteractor* pqQVTKWidget::interactor() const
{
  if (this->useStereo)
  {
    return baseClass.value<pqQVTKWidgetBase*>()->interactor();
  }
  else
  {
    return baseClass.value<QVTKOpenGLNativeWidget*>()->interactor();
  }
}

//----------------------------------------------------------------------------
bool pqQVTKWidget::isValid()
{
  if (this->useStereo)
  {
    return baseClass.value<pqQVTKWidgetBase*>()->isValid();
  }
  else
  {
    return baseClass.value<QVTKOpenGLNativeWidget*>()->isValid();
  }
}

//----------------------------------------------------------------------------
vtkRenderWindow* pqQVTKWidget::renderWindow() const
{
  if (this->useStereo)
  {
    return baseClass.value<pqQVTKWidgetBase*>()->renderWindow();
  }
  else
  {
    return baseClass.value<QVTKOpenGLNativeWidget*>()->renderWindow();
  }
}

//----------------------------------------------------------------------------
void pqQVTKWidget::notifyQApplication(QMouseEvent* e)
{
  if (this->useStereo)
  {
// Due to QTBUG-61836 (see QVTKOpenGLStereoWidget::testingEvent()), events should
// be propagated back to the internal QVTKOpenGLWindow when being fired
// explicitly on the widget instance. We have to use a custom event
// callback in this case to ensure that events are passed to the window.

#if defined(__APPLE__)
    qApp->notify(baseClass.value<QVTKOpenGLNativeWidget*>(), e);
#else
    qApp->notify(baseClass.value<pqQVTKWidgetBase*>()->embeddedOpenGLWindow(), e);
#endif
  }
  else
  {
    qApp->notify(baseClass.value<QVTKOpenGLNativeWidget*>(), e);
  }
}
//----------------------------------------------------------------------------
void pqQVTKWidget::setEnableHiDPI(bool flag)
{
  // disable HiDPI if we are running tests
  if (this->useStereo)
  {
    baseClass.value<pqQVTKWidgetBase*>()->setEnableHiDPI(flag);
  }
  else
  {
    baseClass.value<QVTKOpenGLNativeWidget*>()->setEnableHiDPI(flag);
  }
}
//----------------------------------------------------------------------------
void pqQVTKWidget::setCustomDevicePixelRatio(double cdpr)
{
  // disable HiDPI if we are running tests
  if (this->useStereo)
  {
    baseClass.value<pqQVTKWidgetBase*>()->setCustomDevicePixelRatio(cdpr);
  }
  else
  {
    baseClass.value<QVTKOpenGLNativeWidget*>()->setCustomDevicePixelRatio(cdpr);
  }
}
//----------------------------------------------------------------------------
double pqQVTKWidget::effectiveDevicePixelRatio() const
{
  // disable HiDPI if we are running tests
  if (this->useStereo)
  {
    return baseClass.value<pqQVTKWidgetBase*>()->effectiveDevicePixelRatio();
  }
  else
  {
    return baseClass.value<QVTKOpenGLNativeWidget*>()->effectiveDevicePixelRatio();
  }
}
//----------------------------------------------------------------------------
void pqQVTKWidget::setViewSize(int width, int height)
{
  if (this->ViewProxy)
  {
    int targetSize[2] = { width, height };
    int currentSize[2];
    vtkSMPropertyHelper(this->ViewProxy, "ViewSize").Get(currentSize, 2);
    if (currentSize[0] != targetSize[0] || currentSize[1] != targetSize[1])
    {
      vtkSMPropertyHelper(this->ViewProxy, "ViewSize").Set(targetSize, 2);
      this->ViewProxy->UpdateVTKObjects();
    }
  }
}
