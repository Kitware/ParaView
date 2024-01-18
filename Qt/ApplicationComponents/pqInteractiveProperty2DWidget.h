// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqInteractiveProperty2DWidget_h
#define pqInteractiveProperty2DWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqInteractivePropertyWidgetAbstract.h"
#include "pqSMProxy.h"
#include "vtkBoundingBox.h"
#include "vtkSMNew2DWidgetRepresentationProxy.h"

#include <QScopedPointer>

class vtkObject;
class vtkSMPropertyGroup;

/**
 * pqInteractiveProperty2DWidget is an abstract pqInteractivePropertyWidgetAbstract subclass
 * designed to serve as the superclass for all pqInteractivePropertyWidgetAbstract types that have
 * interactive widget (also called 2D Widgets) associated with them.
 *
 * pqInteractiveProperty2DWidget is intended to provide a Qt widget (along with an
 * interactive widget in the active view) for controlling properties on the \c
 * proxy identified by a vtkSMPropertyGroup passed to the constructor.
 * Subclasses are free to determine which interactive widget to create and how
 * to setup the UI for it.
 *
 * pqInteractiveProperty2DWidget is basically the same as pqInteractivePropertyWidget
 * but use vtkSMNew2DWidgetRepresentationProxy instead vtkSMNewWidgetRepresentationProxy
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqInteractiveProperty2DWidget
  : public pqInteractivePropertyWidgetAbstract
{
  Q_OBJECT
  typedef pqInteractivePropertyWidgetAbstract Superclass;

public:
  pqInteractiveProperty2DWidget(const char* widget_smgroup, const char* widget_smname,
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqInteractiveProperty2DWidget() override;

  /**
   * Returns the proxy for the interactive 2D widget.
   */
  vtkSMNew2DWidgetRepresentationProxy* widgetProxy() const { return this->WidgetProxy; };

protected:
  /**
   * Get the internal instance of the widget proxy.
   */
  vtkSMNewWidgetRepresentationProxyAbstract* internalWidgetProxy() final
  {
    return this->WidgetProxy;
  };

private:
  Q_DISABLE_COPY(pqInteractiveProperty2DWidget)

  vtkSmartPointer<vtkSMNew2DWidgetRepresentationProxy> WidgetProxy;
};

#endif
