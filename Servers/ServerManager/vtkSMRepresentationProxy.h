/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRepresentationProxy - Abstract superclass for all representation
// proxies.
// .SECTION Description
// vtkSMRepresentationProxy is an abstract superclass of all representation
// proxies. A representation proxy is a representation of something in a view.
// That something can be data (vtkSMPipelineRepresentationProxy and subclasses) 
// or widgets (those that have no data inputs). 

#ifndef __vtkSMRepresentationProxy_h
#define __vtkSMRepresentationProxy_h

#include "vtkSMProxy.h"

class vtkSMViewProxy;
class vtkPVDataInformation;

class VTK_EXPORT vtkSMRepresentationProxy : public vtkSMProxy
{
public:
  vtkTypeRevisionMacro(vtkSMRepresentationProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called to update the Representation. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  virtual void Update() { this->Update(0); };
  virtual void Update(vtkSMViewProxy*) { };

  // Description:
  // Returns if the representation's input has changed since most recent
  // Update(). 
  virtual bool UpdateRequired()
    { return false; }

  // Description:
  // Returns true if this representation is visible.
  // Default implementation returns the state of "Visibility" property, if any.
  virtual bool GetVisibility();

  // Description:
  // Get the data information for the represented data.
  // Representations that do not have significatant data representations such as
  // 3D widgets, text annotations may return NULL. Default implementation
  // returns NULL.
  virtual vtkPVDataInformation* GetDataInformation()
    { return 0; }

//BTX
protected:
  vtkSMRepresentationProxy();
  ~vtkSMRepresentationProxy();

  friend class vtkSMViewProxy;

  // Description:
  // Overridden from vtkSMProxy to call BeginCreateVTKObjects() and
  // EndCreateVTKObjects().
  virtual void CreateVTKObjects(int numObjects);

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool AddToView(vtkSMViewProxy* view)=0;

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success/
  // Currently a representation can be added to only one view.
  virtual bool RemoveFromView(vtkSMViewProxy* view)=0;

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects(int vtkNotUsed(numObjects)) {return true;}

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects(int vtkNotUsed(numObjects)) {return true;}

  // Description:
  // Creates a connection between the producer and the consumer
  // using "Input" property. Subclasses can use this to build
  // pipelines.
  void Connect(vtkSMProxy* producer, vtkSMProxy* consumer,
    const char* propertyname="Input");

private:
  vtkSMRepresentationProxy(const vtkSMRepresentationProxy&); // Not implemented
  void operator=(const vtkSMRepresentationProxy&); // Not implemented
//ETX
};

#endif

