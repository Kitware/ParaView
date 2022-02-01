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
#include "vtkSMTrace.h"     // needed for vtkSMTrace::TraceItem
#include "vtkWeakPointer.h" // For Proxies

#include <unordered_map> // For Proxies

class vtkPVArrayInformation;
class vtkSMProxy;
class vtkSMPVRepresentationProxy;
class vtkSMViewProxy;

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

  /**
   * Add range to current scalar bar for a given representation proxy.
   * This allows to display the combined range
   * of multiple data sets that hold the same array.
   */
  void AddRange(vtkSMPVRepresentationProxy* proxy);

  /**
   * Remove range to current scalar bar for a given representation proxy.
   * This allows to display the combined range
   * of multiple data sets that hold the same array.
   */
  void RemoveRange(vtkSMPVRepresentationProxy* proxy);

  /**
   * Get the current range of the scalar bar.
   */
  void GetRange(double range[2]);

  /**
   * Clears all data held by the scalar bar concerning range.
   */
  void ClearRange();

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

  ///@{
  /**
   * Storing a list of proxies linked to this scalar bar.
   *
   * @note
   * We need 2 instances of `vtkSMPVRepresentation*` per item, as the proxy can become unused before
   * this class is aware of it. The weak pointer helps not interfere with the life span of this
   * proxy, and the raw pointer is used as key because of its constness: the weak pointer can become
   * `nullptr` and would invalidate the container is used as key.
   */
  std::unordered_map<vtkSMPVRepresentationProxy*, vtkWeakPointer<vtkSMPVRepresentationProxy>>
    Proxies;
  ///@}

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

  vtkSMScalarBarWidgetRepresentationProxy(const vtkSMScalarBarWidgetRepresentationProxy&) = delete;
  void operator=(const vtkSMScalarBarWidgetRepresentationProxy&) = delete;
};

#endif
