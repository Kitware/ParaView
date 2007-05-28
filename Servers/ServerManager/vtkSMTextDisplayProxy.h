/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTextDisplayProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTextDisplayProxy
// .SECTION Description
//

#ifndef __vtkSMTextDisplayProxy_h
#define __vtkSMTextDisplayProxy_h

#include "vtkSMConsumerDisplayProxy.h"

class vtkSMTextWidgetDisplayProxy;

class VTK_EXPORT vtkSMTextDisplayProxy : public vtkSMConsumerDisplayProxy
{
public:
  static vtkSMTextDisplayProxy* New();
  vtkTypeRevisionMacro(vtkSMTextDisplayProxy, vtkSMConsumerDisplayProxy);
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
  virtual int UpdateRequired() { return this->Dirty? 1 : 0; }

  // Description:
  // Called to update the Display. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  virtual void Update() { this->Update(0); };
  virtual void Update(vtkSMAbstractViewModuleProxy*);

  // Description:
  // Called when the display is added/removed to/from a RenderModule.
  // Default implementation searches for a subproxies with name
  // Prop/Prop2D. If found, they are added/removed to/from the 
  // Renderer/2DRenderer respectively. 
  // If such subproxies are not found no error is raised.
  virtual void AddToRenderModule(vtkSMRenderModuleProxy*);
  virtual void RemoveFromRenderModule(vtkSMRenderModuleProxy*);

  // Description:
  // Set the update time passed on to the update suppressor.
  void SetUpdateTime(double time);

// BTX
protected:
  vtkSMTextDisplayProxy();
  ~vtkSMTextDisplayProxy();

  // Invalidate geometry. If useCache is true, do not invalidate
  // cached geometry
  virtual void InvalidateGeometryInternal(int /*useCache*/)
    { this->Dirty = true; }

  virtual void CreateVTKObjects();

  vtkSMTextWidgetDisplayProxy* TextWidgetProxy;

  vtkSMSourceProxy* UpdateSuppressorProxy;
  vtkSMSourceProxy* CollectProxy;
  vtkSMSourceProxy* Input;
  void SetInput(vtkSMSourceProxy*);
  bool Dirty;
private:
  vtkSMTextDisplayProxy(const vtkSMTextDisplayProxy&); // Not implemented
  void operator=(const vtkSMTextDisplayProxy&); // Not implemented
//ETX
};

#endif

