/*=========================================================================

  Program:   ParaView
  Module:    vtkSMViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMViewProxy - Superclass for all view proxies.
// .SECTION Description
// vtkSMViewProxy is a superclass for all view proxies. A view proxy
// abstracts the logic to take one or more representation proxies and show then
// in some viewport such as vtkRenderWindow.
// This class may directly be used as the view proxy for views that do all the
// rendering work at the GUI level.
// .SECTION Events
// \li vtkCommand::StartEvent(callData: int:0) -- start of StillRender().
// \li vtkCommand::EndEvent(callData: int:0) -- end of StillRender().
// \li vtkCommand::StartEvent(callData: int:1) -- start of InteractiveRender().
// \li vtkCommand::EndEvent(callData: int:1) -- end of InteractiveRender().

#ifndef __vtkSMViewProxy_h
#define __vtkSMViewProxy_h

#include "vtkSMProxy.h"

class vtkCollection;
class vtkCommand;
class vtkSMRepresentationProxy;
class vtkSMRepresentationStrategy;

class VTK_EXPORT vtkSMViewProxy : public vtkSMProxy
{
public:
  static vtkSMViewProxy* New();
  vtkTypeRevisionMacro(vtkSMViewProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Adds a representation proxy to this view. 
  virtual void AddRepresentation(vtkSMRepresentationProxy*);

  // Description:
  // Removes a representation proxy from this view.
  virtual void RemoveRepresentation(vtkSMRepresentationProxy*);

  // Description:
  // Removes all added representations from this view.
  // Simply calls RemoveRepresentation() on all added representations 
  // one by one.
  void RemoveAllRepresentations();

  // Description:
  // Updates the data pipelines for all visible representations.
  virtual void UpdateAllRepresentations();

  // Description:
  // Renders the view using full resolution.
  // Internally calls 
  // \li UpdateAllDisplays()
  // \li BeginStillRender()
  // \li PerformRender()
  // \li EndStillRender() 
  // in that order.
  void StillRender();

  // Description:
  // Renders the view using lower resolution is possible.
  // Internally calls 
  // \li UpdateAllDisplays()
  // \li BeginInteractiveRender()
  // \li PerformRender()
  // \li EndInteractiveRender() 
  // in that order.
  void InteractiveRender();
 
  // Description:
  // Creates a new vtkSMRepresentationStrategy subclass based on the type
  // requested. A view subclass can define a set of strategies that are
  // registered with it which the representation can ask. A representation
  // strategy encapsulates the pipelines for managing
  // level-of-detail/parallelism. Views that do not support parallelism or LOD
  // may not provide any strategy at all.
  virtual vtkSMRepresentationStrategy* NewStrategy(int dataType, int type)
    { return 0; }

//BTX
protected:
  vtkSMViewProxy();
  ~vtkSMViewProxy();

  // Description:
  // Method called at the start of StillRender().
  virtual void BeginStillRender();

  // Description:
  // Method called at the end of StillRender().
  virtual void EndStillRender();

  // Description:
  // Method called at the start of InteractiveRender().
  virtual void BeginInteractiveRender();
  
  // Description:
  // Method called at the end of InteractiveRender().
  virtual void EndInteractiveRender();

  // Description:
  // Performs the actual rendering. This method is called by
  // both InteractiveRender() and StillRender(). 
  // Default implementation is empty.
  virtual void PerformRender() {};

  // Description:
  // Called by AddRepresentation(). Subclasses can override to add 
  // observers etc.
  virtual void AddRepresentationInternal(vtkSMRepresentationProxy* rep);

  // Description:
  // Called by RemoveRepresentation(). Subclasses can override to remove 
  // observers etc.
  virtual void RemoveRepresentationInternal(vtkSMRepresentationProxy* rep);

  // Description:
  // Called to process events.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);

  // Description:
  // Returns the observer that the subclasses can use to listen to additional
  // events. Additionally these subclasses should override
  // ProcessEvents() to handle these events.
  vtkCommand* GetObserver();

  // Collection of representation objects added to this view.
  vtkCollection* Representations;

private:
  vtkSMViewProxy(const vtkSMViewProxy&); // Not implemented
  void operator=(const vtkSMViewProxy&); // Not implemented

  class Command;
  friend class Command;
  Command* Observer;
//ETX
};

#endif

