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
// .NAME vtkSMScalarBarWidgetRepresentationProxy - is the representation
// corresponding to a scalar bar or color legend in a Render View.
// .SECTION Description

#ifndef __vtkSMScalarBarWidgetRepresentationProxy_h
#define __vtkSMScalarBarWidgetRepresentationProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMNewWidgetRepresentationProxy.h"

class vtkSMViewProxy;
class vtkPVArrayInformation;

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMScalarBarWidgetRepresentationProxy :
  public vtkSMNewWidgetRepresentationProxy
{
public:
  static vtkSMScalarBarWidgetRepresentationProxy* New();
  vtkTypeMacro(vtkSMScalarBarWidgetRepresentationProxy, vtkSMNewWidgetRepresentationProxy);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Updates the scalar bar's component title using the data information to
  // determine component names if possible.
  virtual bool UpdateComponentTitle(vtkPVArrayInformation* dataInfo);
  static bool UpdateComponentTitle(vtkSMProxy* proxy, vtkPVArrayInformation* dataInfo)
    {
    vtkSMScalarBarWidgetRepresentationProxy* self =
      vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(proxy);
    return self? self->UpdateComponentTitle(dataInfo) : false;
    }

  // Description:
  // Attempt to place the scalar bar in the view based on the placement of other
  // currently shown and visible scalar bars.
  virtual bool PlaceInView(vtkSMProxy* view);
  static bool PlaceInView(vtkSMProxy* proxy, vtkSMProxy* view)
    {
    vtkSMScalarBarWidgetRepresentationProxy* self =
      vtkSMScalarBarWidgetRepresentationProxy::SafeDownCast(proxy);
    return self? self->PlaceInView(view) : false;
    }

//BTX
protected:
  vtkSMScalarBarWidgetRepresentationProxy();
  ~vtkSMScalarBarWidgetRepresentationProxy();

  // Description:
  // Overridden from vtkSMProxy to call BeginCreateVTKObjects() and
  // EndCreateVTKObjects().
  virtual void CreateVTKObjects();

  // Description:
  // Called every time the user interacts with the widget.
  virtual void ExecuteEvent(unsigned long event);

  vtkSMProxy* ActorProxy;
private:
  vtkSMScalarBarWidgetRepresentationProxy(const vtkSMScalarBarWidgetRepresentationProxy&); // Not implemented
  void operator=(const vtkSMScalarBarWidgetRepresentationProxy&); // Not implemented
//ETX
};

#endif

