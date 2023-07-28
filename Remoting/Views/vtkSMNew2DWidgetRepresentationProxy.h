// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkSMNew2DWidgetRepresentationProxy_h
#define vtkSMNew2DWidgetRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMNewWidgetRepresentationProxyAbstract.h"

/**
 * @class   vtkSMNew2DWidgetRepresentationProxy
 * @brief   proxy for 2D widgets and
 * their representations in ParaView.
 *
 * vtkSMNew2DWidgetRepresentationProxy is a proxy for 2D widgets and their
 * representations.
 **/

class vtkContextItem;

class VTKREMOTINGVIEWS_EXPORT vtkSMNew2DWidgetRepresentationProxy
  : public vtkSMNewWidgetRepresentationProxyAbstract
{
public:
  static vtkSMNew2DWidgetRepresentationProxy* New();
  vtkTypeMacro(vtkSMNew2DWidgetRepresentationProxy, vtkSMNewWidgetRepresentationProxyAbstract);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get Representation Proxy.
   */
  vtkGetObjectMacro(ContextItemProxy, vtkSMProxy);

protected:
  vtkSMNew2DWidgetRepresentationProxy();
  ~vtkSMNew2DWidgetRepresentationProxy() override;

  /**
   * Called every time the user interacts with the widget.
   */
  void ExecuteEvent(unsigned long event) override;

  /**
   * Overridden from vtkSMProxy to call BeginCreateVTKObjects() and
   * EndCreateVTKObjects().
   */
  void CreateVTKObjects() override;

  vtkWeakPointer<vtkContextItem> ContextItem;
  vtkSMProxy* ContextItemProxy = nullptr;

private:
  vtkSMNew2DWidgetRepresentationProxy(const vtkSMNew2DWidgetRepresentationProxy&) = delete;
  void operator=(const vtkSMNew2DWidgetRepresentationProxy&) = delete;
};

#endif
