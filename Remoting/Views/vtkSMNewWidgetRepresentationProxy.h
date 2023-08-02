// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMNewWidgetRepresentationProxy
 * @brief   proxy for 3D widgets and
 * their representations in ParaView.
 *
 * vtkSMNewWidgetRepresentationProxy is a proxy for 3D widgets and their
 * representations. It has several responsibilities.
 * \li Sets up the link between the Widget and its representation on VTK side.
 * \li Sets up event handlers to ensure that the representation proxy's info
 * properties are updated any time the widget fires interaction events.
 * \li Provides API to perform tasks typical with 3DWidgets in ParaView e.g.
 * picking, placing widget on data bounds.
 */

#ifndef vtkSMNewWidgetRepresentationProxy_h
#define vtkSMNewWidgetRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMNewWidgetRepresentationProxyAbstract.h"
class vtkSMViewProxy;
class vtkAbstractWidget;

class VTKREMOTINGVIEWS_EXPORT vtkSMNewWidgetRepresentationProxy
  : public vtkSMNewWidgetRepresentationProxyAbstract
{
public:
  static vtkSMNewWidgetRepresentationProxy* New();
  vtkTypeMacro(vtkSMNewWidgetRepresentationProxy, vtkSMNewWidgetRepresentationProxyAbstract);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get the widget for the representation.
   */
  vtkGetObjectMacro(Widget, vtkAbstractWidget);
  ///@}

  ///@{
  /**
   * Get Representation Proxy.
   */
  vtkGetObjectMacro(RepresentationProxy, vtkSMProxy);
  ///@}

protected:
  vtkSMNewWidgetRepresentationProxy();
  ~vtkSMNewWidgetRepresentationProxy() override;

  /**
   * Overridden from vtkSMProxy to call BeginCreateVTKObjects() and
   * EndCreateVTKObjects().
   */
  void CreateVTKObjects() override;

  vtkSMProxy* RepresentationProxy = nullptr;
  vtkSMProxy* WidgetProxy = nullptr;
  vtkAbstractWidget* Widget = nullptr;

  /**
   * Called every time the user interacts with the widget.
   */
  void ExecuteEvent(unsigned long event) override;

private:
  vtkSMNewWidgetRepresentationProxy(const vtkSMNewWidgetRepresentationProxy&) = delete;
  void operator=(const vtkSMNewWidgetRepresentationProxy&) = delete;
};

#endif
