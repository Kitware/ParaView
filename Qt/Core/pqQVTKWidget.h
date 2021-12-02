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
#ifndef pqQVTKWidget_h
#define pqQVTKWidget_h

#include "QVTKInteractor.h"
#include "pqCoreModule.h"
#include "pqQVTKWidgetBase.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"

#include <QVariant>
#include <QWidget>

class vtkSMProxy;
class vtkSMSession;

/**
 * @class pqQVTKWidget
 * @brief QWidget subclass to show rendering results from vtkSMViewProxy.
 *
 * pqQVTKWidget is used by ParaView to show rendering results from a
 * vtkSMViewProxy (or subclass) in QWidget. Internally, it uses a
 * QVTKOpenGLNativeWidget, or QVTKOpenGLStereoWidget based on whether stereo
 * mode is enabled and supported on the platform.
 *
 * This class adds awareness for view proxies to the widget it owns.
 * The advantage of doing so is that pqQVTKWidget can automatically update the
 * "ViewSize" property on the view proxy whenever the widget's size/position changes.
 */
class PQCORE_EXPORT pqQVTKWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqQVTKWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  pqQVTKWidget(QWidget* parentObject, Qt::WindowFlags f, bool isStereo);
  ~pqQVTKWidget() override;

  /**
   * Set the view proxy.
   */
  void setViewProxy(vtkSMProxy*);

  /**
   * Set the session.
   * This is only used when ViewProxy is not set.
   */
  void setSession(vtkSMSession*);

  /**
   * Return the Proxy ID if any, otherwise return 0
   */
  vtkTypeUInt32 getProxyId();

  /**
   * Set/Get the name of the property to use to update the size of the widget
   * on the proxy. By default "ViewSize" is used.
   */
  void setSizePropertyName(const QString& pname) { this->SizePropertyName = pname; }
  const QString& sizePropertyName() const { return this->SizePropertyName; }

  void notifyQApplication(QMouseEvent*);

  /**
   * Methods that decorate QVTKOpenGL*Widget methods
   */
  void setRenderWindow(vtkRenderWindow* win);
  vtkRenderWindow* renderWindow() const;

  QVTKInteractor* interactor() const;
  bool isValid();

  void setEnableHiDPI(bool flag);
  void setCustomDevicePixelRatio(double cdpr);
  double effectiveDevicePixelRatio() const;
  void setViewSize(int width, int height);

  /**
   * Provide access to the internal QWidget subclass used for actual rendering.
   * This may be QVTKOpenGLStereoWidget or QVTKOpenGLNativeWidget
   */
  QWidget* renderWidget() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void paintMousePointer(int x, int y);

private Q_SLOTS:
  void prepareContextForRendering();

protected:
  bool renderVTK();
  bool canRender();

#if PARAVIEW_USING_QVTKOPENGLSTEREOWIDGET
  void resizeEvent(QResizeEvent* evt) override;
#endif

private:
  Q_DISABLE_COPY(pqQVTKWidget)
  vtkSmartPointer<vtkSMProxy> ViewProxy;
  vtkWeakPointer<vtkSMSession> Session;
  QString SizePropertyName;
  vtkNew<vtkEventQtSlotConnect> VTKConnect;

  bool useStereo;
  QVariant baseClass;
};

class vtkGenericOpenGLRenderWindow;
typedef vtkGenericOpenGLRenderWindow pqQVTKWidgetBaseRenderWindowType;

#endif
