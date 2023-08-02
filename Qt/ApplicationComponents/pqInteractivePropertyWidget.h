// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
