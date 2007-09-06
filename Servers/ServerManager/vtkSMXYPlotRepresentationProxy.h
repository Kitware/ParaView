/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXYPlotRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMXYPlotRepresentationProxy
// .SECTION Description
//

#ifndef __vtkSMXYPlotRepresentationProxy_h
#define __vtkSMXYPlotRepresentationProxy_h

#include "vtkSMClientDeliveryRepresentationProxy.h"

class VTK_EXPORT vtkSMXYPlotRepresentationProxy : 
  public vtkSMClientDeliveryRepresentationProxy
{
public:
  static vtkSMXYPlotRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMXYPlotRepresentationProxy, 
    vtkSMClientDeliveryRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called to update the Display. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  // Overridden to update the domains on the DummyConsumer proxy.
  virtual void Update() { this->Update(0); };
  virtual void Update(vtkSMViewProxy* view);

//BTX
protected:
  vtkSMXYPlotRepresentationProxy();
  ~vtkSMXYPlotRepresentationProxy();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();

private:
  vtkSMXYPlotRepresentationProxy(const vtkSMXYPlotRepresentationProxy&); // Not implemented
  void operator=(const vtkSMXYPlotRepresentationProxy&); // Not implemented
//ETX
};

#endif

