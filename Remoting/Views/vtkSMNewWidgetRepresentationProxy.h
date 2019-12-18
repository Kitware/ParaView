/*=========================================================================

  Program:   ParaView
  Module:    vtkSMNewWidgetRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkSMProxy.h"
class vtkSMViewProxy;
class vtkSMNewWidgetRepresentationObserver;
class vtkAbstractWidget;

struct vtkSMNewWidgetRepresentationInternals;

class VTKREMOTINGVIEWS_EXPORT vtkSMNewWidgetRepresentationProxy : public vtkSMProxy
{
public:
  static vtkSMNewWidgetRepresentationProxy* New();
  vtkTypeMacro(vtkSMNewWidgetRepresentationProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the widget for the representation.
   */
  vtkGetObjectMacro(Widget, vtkAbstractWidget);
  //@}

  //@{
  /**
   * Get Representation Proxy.
   */
  vtkGetObjectMacro(RepresentationProxy, vtkSMProxy);
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
  vtkSMNewWidgetRepresentationProxy();
  ~vtkSMNewWidgetRepresentationProxy() override;

  /**
   * Overridden from vtkSMProxy to call BeginCreateVTKObjects() and
   * EndCreateVTKObjects().
   */
  void CreateVTKObjects() override;

  vtkSMProxy* RepresentationProxy;
  vtkSMProxy* WidgetProxy;
  vtkAbstractWidget* Widget;
  vtkSMNewWidgetRepresentationObserver* Observer;
  vtkSMNewWidgetRepresentationInternals* Internal;

  friend class vtkSMNewWidgetRepresentationObserver;

  /**
   * Called every time the user interacts with the widget.
   */
  virtual void ExecuteEvent(unsigned long event);

  /**
   * Called everytime a controlled property's unchecked values change.
   */
  void ProcessLinkedPropertyEvent(vtkSMProperty* controlledProperty, unsigned long event);

private:
  vtkSMNewWidgetRepresentationProxy(const vtkSMNewWidgetRepresentationProxy&) = delete;
  void operator=(const vtkSMNewWidgetRepresentationProxy&) = delete;
};

#endif
