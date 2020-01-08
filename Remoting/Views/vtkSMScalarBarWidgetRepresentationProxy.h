/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScalarBarWidgetRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMScalarBarWidgetRepresentationProxy
 * @brief   is the representation
 * corresponding to a scalar bar or color legend in a Render View.
 *
*/

#ifndef vtkSMScalarBarWidgetRepresentationProxy_h
#define vtkSMScalarBarWidgetRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMTrace.h" // needed for vtkSMTrace::TraceItem

class vtkSMViewProxy;
class vtkPVArrayInformation;

class VTKREMOTINGVIEWS_EXPORT vtkSMScalarBarWidgetRepresentationProxy
  : public vtkSMNewWidgetRepresentationProxy
{
public:
  static vtkSMScalarBarWidgetRepresentationProxy* New();
  vtkTypeMacro(vtkSMScalarBarWidgetRepresentationProxy, vtkSMNewWidgetRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Updates the scalar bar's component title using the data information to
   * determine component names if possible.
   */
  virtual bool UpdateComponentTitle(vtkPVArrayInformation* dataInfo);
  static bool UpdateComponentTitle(vtkSMProxy* proxy, vtkPVArrayInformation* dataInfo)
  {
    vtkSMScalarBarWidgetRepresentationProxy* self =
      vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(proxy);
    return self ? self->UpdateComponentTitle(dataInfo) : false;
  }
  //@}

  //@{
  /**
   * Attempt to place the scalar bar in the view based on the placement of other
   * currently shown and visible scalar bars.
   */
  virtual bool PlaceInView(vtkSMProxy* view);
  static bool PlaceInView(vtkSMProxy* proxy, vtkSMProxy* view)
  {
    vtkSMScalarBarWidgetRepresentationProxy* self =
      vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(proxy);
    return self ? self->PlaceInView(view) : false;
  }
  //@}

protected:
  vtkSMScalarBarWidgetRepresentationProxy();
  ~vtkSMScalarBarWidgetRepresentationProxy() override;

  /**
   * Overridden from vtkSMProxy to call BeginCreateVTKObjects() and
   * EndCreateVTKObjects().
   */
  void CreateVTKObjects() override;

  /**
   * Called every time the user interacts with the widget.
   */
  void ExecuteEvent(unsigned long event) override;

  vtkSMProxy* ActorProxy;

private:
  //@{
  /**
   * Called when user starts/stops interacting with the scalar bar to move it.
   * We handle tracking of the property so that we can trace the changes to its
   * properties in Python.
   */
  void BeginTrackingPropertiesForTrace();
  void EndTrackingPropertiesForTrace();
  //@}

  /**
   * Synchronizes Position2 length definition of the scalar bar widget with the
   * ScalarBarLength property.
   */
  void ScalarBarWidgetPosition2ToScalarBarLength();
  void ScalarBarLengthToScalarBarWidgetPosition2();

  // Used in StartTrackingPropertiesForTrace/EndTrackingPropertiesForTrace.
  vtkSMTrace::TraceItem* TraceItem;

private:
  vtkSMScalarBarWidgetRepresentationProxy(const vtkSMScalarBarWidgetRepresentationProxy&) = delete;
  void operator=(const vtkSMScalarBarWidgetRepresentationProxy&) = delete;
};

#endif
