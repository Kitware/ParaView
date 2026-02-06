// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqInteractivePropertyWidget_h
#define pqInteractivePropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqInteractivePropertyWidgetAbstract.h"
#include "vtkSMNewWidgetRepresentationProxy.h"

#include <QScopedPointer>

class vtkObject;
class vtkSMPropertyGroup;
class vtkRenderer;
class vtkSMRenderViewProxy;
class vtkVector3d;

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

  /**
   * Convert the given display space coordinates into world space coordinates.
   * If there is no renderer or render view proxy, it returns an empty vector.
   */
  std::vector<vtkVector3d> displayToWorldCoordinates(
    const std::vector<vtkVector3d>& displayCoordPoints);

  /**
   * Return the Z coordinate of the focal point in display space.
   * If there is no renderer or render view proxy, it returns -1.
   */
  double getFocalPointDepth();

  /**
   * Return the renderer object from the current active view.
   * If the renderer does not exists, it returns a nullptr.
   */
  vtkRenderer* getRenderer();

private:
  Q_DISABLE_COPY(pqInteractivePropertyWidget)

  vtkSmartPointer<vtkSMNewWidgetRepresentationProxy> WidgetProxy;
};

#endif
