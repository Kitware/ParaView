/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTextSourceRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTextSourceRepresentationProxy
// .SECTION Description
//

#ifndef __vtkSMTextSourceRepresentationProxy_h
#define __vtkSMTextSourceRepresentationProxy_h

#include "vtkSMDataRepresentationProxy.h"

class vtkSMTextWidgetRepresentationProxy;
class vtkSMViewProxy;

class VTK_EXPORT vtkSMTextSourceRepresentationProxy : public vtkSMDataRepresentationProxy
{
public:
  static vtkSMTextSourceRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMTextSourceRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called when setting input using the Input property.
  // Subclasses must override this method to set the input 
  // to the display pipeline.
  // Typically, none of the displays use method/hasMultipleInputs
  // arguements.
  virtual void AddInput(vtkSMSourceProxy* input, const char* method, 
    int hasMultipleInputs);

  // Description:
  // This method returns if the Update() or UpdateDistributedGeometry()
  // calls will actually lead to an Update. This is used by the render module
  // to decide if it can expect any pipeline updates.
  virtual bool UpdateRequired() { return this->Dirty? true : false; }

  // Description:
  // Called to update the Display. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  virtual void Update() { this->Update(0); };
  virtual void Update(vtkSMViewProxy* view);

  // Description:
  // Set the update time passed on to the update suppressor.
  void SetUpdateTime(double time);

// BTX
protected:
  vtkSMTextSourceRepresentationProxy();
  ~vtkSMTextSourceRepresentationProxy();

  // Invalidate geometry. If useCache is true, do not invalidate
  // cached geometry
  virtual void InvalidateGeometryInternal(int /*useCache*/)
    { this->Dirty = true; }

    // Description:
  // This method is called at the beginning of CreateVTKObjects().
  // This gives the subclasses an opportunity to set the servers flags
  // on the subproxies.
  // If this method returns false, CreateVTKObjects() is aborted.
  virtual bool BeginCreateVTKObjects();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool RemoveFromView(vtkSMViewProxy* view);

  vtkSMTextWidgetRepresentationProxy* TextWidgetProxy;

  vtkSMSourceProxy* UpdateSuppressorProxy;
  vtkSMSourceProxy* CollectProxy;
  bool Dirty;

private:
  vtkSMTextSourceRepresentationProxy(const vtkSMTextSourceRepresentationProxy&); // Not implemented
  void operator=(const vtkSMTextSourceRepresentationProxy&); // Not implemented
//ETX
};

#endif

