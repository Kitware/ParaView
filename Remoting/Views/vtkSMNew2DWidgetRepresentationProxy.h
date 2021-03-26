/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNew2DWidgetRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkSMNew2DWidgetRepresentationProxy_h
#define vtkSMNew2DWidgetRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMProxy.h"

class vtkSMNew2DWidgetRepresentationObserver;

/**
 * @class   vtkSMNew2DWidgetRepresentationProxy
 * @brief   proxy for 2D widgets and
 * their representations in ParaView.
 *
 * vtkSMNew2DWidgetRepresentationProxy is a proxy for 2D widgets and their
 * representations.
 **/

class VTKREMOTINGVIEWS_EXPORT vtkSMNew2DWidgetRepresentationProxy : public vtkSMProxy
{
public:
  static vtkSMNew2DWidgetRepresentationProxy* New();
  vtkTypeMacro(vtkSMNew2DWidgetRepresentationProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get Representation Proxy.
   */
  vtkGetObjectMacro(ContextItemProxy, vtkSMProxy);
  //@}

  //@{
  /**
   * Called to link properties from a Widget to \c controlledProxy i.e. a
   * proxy whose properties are being manipulated using this Widget.
   * Currently, we only support linking with one controlled proxy at a time. One
   * must call UnlinkProperties() before one can call this method on another
   * controlledProxy. The \c controlledPropertyGroup is used to determine the
   * mapping between this widget properties and controlledProxy properties.
   */
  bool LinkProperties(vtkSMProxy* controlledProxy, vtkSMPropertyGroup* controlledPropertyGroup);
  bool UnlinkProperties(vtkSMProxy* controlledProxy);
  //@}
protected:
  vtkSMNew2DWidgetRepresentationProxy();
  ~vtkSMNew2DWidgetRepresentationProxy() override;

  /**
   * Overridden from vtkSMProxy to call BeginCreateVTKObjects() and
   * EndCreateVTKObjects().
   */
  void CreateVTKObjects() override;

  vtkSMProxy* ContextItemProxy;
  vtkSMNew2DWidgetRepresentationObserver* Observer;

  friend class vtkSMNew2DWidgetRepresentationObserver;

  /**
   * Called every time the user interacts with the widget.
   */
  virtual void ExecuteEvent(unsigned long event);

  /**
   * Called everytime a controlled property's unchecked values change.
   */
  void ProcessLinkedPropertyEvent(vtkSMProperty* caller, unsigned long event);

private:
  vtkSMNew2DWidgetRepresentationProxy(const vtkSMNew2DWidgetRepresentationProxy&) = delete;
  void operator=(const vtkSMNew2DWidgetRepresentationProxy&) = delete;

  struct Internals;
  Internals* Internal;
};

#endif
