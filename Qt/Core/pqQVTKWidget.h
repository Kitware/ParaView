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

#include "pqCoreModule.h"
#include "pqQVTKWidgetBase.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"
#include <QPointer>

class vtkSMProxy;
class vtkSMSession;

/**
* pqQVTKWidget extends pqQVTKWidgetBase to add awareness for view proxies. The
* advantage of doing that is that pqQVTKWidget can automatically update the
* "ViewSize" property on the view proxy whenever the
* widget's size/position changes.
*
* This class also enables image-caching by default (image caching support is
* provided by the superclass).
*/
class PQCORE_EXPORT pqQVTKWidget : public pqQVTKWidgetBase
{
  Q_OBJECT
  typedef pqQVTKWidgetBase Superclass;

public:
  pqQVTKWidget(QWidget* parent = NULL, Qt::WindowFlags f = 0);
  virtual ~pqQVTKWidget();

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
  * Retrun the Proxy ID if any, otherwise return 0
  */
  vtkTypeUInt32 getProxyId();

  /**
  * Set/Get the name of the property to use to update the size of the widget
  * on the proxy. By default "ViewSize" is used.
  */
  void setSizePropertyName(const QString& pname) { this->SizePropertyName = pname; }
  const QString& sizePropertyName() const { return this->SizePropertyName; }

public slots:
  void paintMousePointer(int x, int y);

protected:
  /**
  * overloaded resize handler
  */
  virtual void resizeEvent(QResizeEvent* event);

  //@{
  /**
   * methods that manage skipping of rendering if ParaView is not ready for it.
   */
  virtual void doDeferredRender();
  virtual bool renderVTK();
  bool canRender();
  //@}
private slots:
  void updateSizeProperties();
  void handleViewSizeForModifiedQt4();

private:
  Q_DISABLE_COPY(pqQVTKWidget)
  vtkSmartPointer<vtkSMProxy> ViewProxy;
  vtkWeakPointer<vtkSMSession> Session;
  QImage MousePointerToDraw;
  QString SizePropertyName;

  vtkNew<vtkEventQtSlotConnect> VTKConnect;
  bool SkipHandleViewSizeForModifiedQt4;
};

#endif
