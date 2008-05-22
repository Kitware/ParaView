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
  virtual void AddInput(unsigned int inputPort,
                        vtkSMSourceProxy* input,
                        unsigned int outputPort,
                        const char* method);
  virtual void AddInput(vtkSMSourceProxy* input,
                        const char* method)
  {
    this->AddInput(0, input, 0, method);
  }

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
  // Overridden to make the Strategy modified as well.
  // The strategy is not marked dirty if the modifiedProxy == this, 
  // thus if the changes to representation itself invalidates the data pipelines
  // it must explicity mark the strategy invalid.
  virtual void MarkDirty(vtkSMProxy* modifiedProxy);

// BTX
protected:
  vtkSMTextSourceRepresentationProxy();
  ~vtkSMTextSourceRepresentationProxy();

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

  // Description:
  // Pass the actual update time to use to all strategies.
  // When not using strategies, make sure that this method is overridden to pass
  // the update time correctly.
  virtual void SetUpdateTimeInternal(double time);

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

