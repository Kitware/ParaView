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
// That something can be data (vtkSMDataRepresentationProxy and subclasses) 
// or widgets (those that have no data inputs). 
//
// A representation additionally has selection obligations i.e. a representation
// may be able to show a selection. Here we define API to query whether the
// representation fulfills selection obligations. For more details look at
// vtkSMDataRepresentationProxy.

#ifndef __vtkSMRepresentationProxy_h
#define __vtkSMRepresentationProxy_h

#include "vtkSMProxy.h"

class vtkSMViewProxy;
class vtkPVDataInformation;
class vtkInformation;

class VTK_EXPORT vtkSMRepresentationProxy : public vtkSMProxy
{
public:
  vtkTypeMacro(vtkSMRepresentationProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called to update the Representation. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  // Typically update should cause any pipeline execution only is
  // UpdateRequired() is true.
  // If Update() is going to result in some execution that changes data or data
  // information, it must fire vtkCommand::StartEvent and vtkCommand::EndEvent
  // to mark the start and end of the update.
  virtual void Update() { this->Update(0); };
  virtual void Update(vtkSMViewProxy*) { this->PostUpdateData(); };

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
  // Get the bounds for the representation.  Returns true if successful.
  // Default implementation returns non-transformed bounds.
  virtual bool GetBounds(double bounds[6]);

  // Description:
  // Get the data information for the represented data.
  // Representations that do not have significatant data representations such as
  // 3D widgets, text annotations may return NULL. Default implementation
  // returns NULL.
  virtual vtkPVDataInformation* GetRepresentedDataInformation(bool update=true)
    { 
    (void)update;
    return 0; 
    }

  // Description:
  // Returns the data size of the display data. When using LOD this is the
  // low-res data size, else it's same as GetFullResMemorySize().
  // This may trigger a pipeline update to obtain correct data sizes.
  virtual unsigned long GetDisplayedMemorySize()
    { return 0; }

  // Description:
  // Returns the data size for the full-res data.
  // This may trigger a pipeline update to obtain correct data sizes.
  virtual unsigned long GetFullResMemorySize()
    { return 0; }

//BTX
  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  // Don't call this directly, it is called by the View.
  virtual bool AddToView(vtkSMViewProxy* view)=0;

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  // Don't call this directly, it is called by the View.
  virtual bool RemoveFromView(vtkSMViewProxy* vtkNotUsed(view)) 
    { return true; }

  // Description:
  // Called to set the view information object.
  // Don't call this directly, it is called by the View.
  virtual void SetViewInformation(vtkInformation*);
  vtkGetObjectMacro(ViewInformation, vtkInformation);

  // Description:
  // Called by the view to pass the view's update time to the representation.
  virtual void SetViewUpdateTime(double time)
    {
    this->ViewUpdateTimeInitialized = true;
    this->ViewUpdateTime = time;
    }
  vtkGetMacro(ViewUpdateTime, double);
protected:
  vtkSMRepresentationProxy();
  ~vtkSMRepresentationProxy();

  // Description:
  // Overridden from vtkSMProxy to call BeginCreateVTKObjects() and
  // EndCreateVTKObjects().
  virtual void CreateVTKObjects();

  // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects() {return true;}

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects() {return true;}

  // Description:
  // Creates a connection between the producer and the consumer
  // using "Input" property. Subclasses can use this to build
  // pipelines. OutputPort is applicable only to vtkSMIntVectorProperty.
  void Connect(vtkSMProxy* producer, vtkSMProxy* consumer,
    const char* propertyname="Input", int outputport=0);

  vtkInformation* ViewInformation;
  double ViewUpdateTime;
  bool ViewUpdateTimeInitialized;

  friend class vtkSMPVRepresentationProxy;
private:
  vtkSMRepresentationProxy(const vtkSMRepresentationProxy&); // Not implemented
  void operator=(const vtkSMRepresentationProxy&); // Not implemented

//ETX
};

#endif

