/*=========================================================================

   Program: ParaView
   Module:  pqInteractivePropertyWidget.h

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
#ifndef pqInteractivePropertyWidget_h
#define pqInteractivePropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqInteractivePropertyWidgetAbstract.h"
#include "pqSMProxy.h"
#include "vtkBoundingBox.h"
#include "vtkSMNewWidgetRepresentationProxy.h"

#include <QScopedPointer>

class vtkObject;
class vtkSMPropertyGroup;

/**
 * pqInteractivePropertyWidget is an abstract pqPropertyWidget subclass
 * designed to serve as the superclass for all pqPropertyWidget types that have
 * interactive widget (also called 3D Widgets) associated with them.
 *
 * pqInteractivePropertyWidget is intended to provide a Qt widget (along with an
 * interactive widget in the active view) for controlling properties on the \c
 * proxy identified by a vtkSMPropertyGroup passed to the constructor.
 * Subclasses are free to determine which interactive widget to create and how
 * to setup the UI for it.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqInteractivePropertyWidget
  : public pqInteractivePropertyWidgetAbstract
{
  Q_OBJECT
  typedef pqInteractivePropertyWidgetAbstract Superclass;

public:
  pqInteractivePropertyWidget(const char* widget_smgroup, const char* widget_smname,
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqInteractivePropertyWidget() override;

  /**
   * Returns the proxy for the 3D interactive widget.
   */
  vtkSMNewWidgetRepresentationProxy* widgetProxy() const { return this->WidgetProxy; };

protected:
  /**
   * Get the internal instance of the widget proxy.
   */
  vtkSMNewWidgetRepresentationProxyAbstract* internalWidgetProxy() final
  {
    return this->WidgetProxy;
  };

private:
  Q_DISABLE_COPY(pqInteractivePropertyWidget)

  vtkSmartPointer<vtkSMNewWidgetRepresentationProxy> WidgetProxy;
};

#endif
