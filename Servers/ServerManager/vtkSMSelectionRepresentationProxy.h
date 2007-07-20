/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSelectionRepresentationProxy
// .SECTION Description
// Representation use to show selection. This shows only the selection i.e.
// output of ExtractSelection filter.

#ifndef __vtkSMSelectionRepresentationProxy_h
#define __vtkSMSelectionRepresentationProxy_h

#include "vtkSMDataRepresentationProxy.h"

class vtkSMDataLabelRepresentationProxy;

class VTK_EXPORT vtkSMSelectionRepresentationProxy : public vtkSMDataRepresentationProxy
{
public:
  static vtkSMSelectionRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMSelectionRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to pass the view information to all the internal
  // representations.
  virtual void SetViewInformation(vtkInformation*);

  // Description:
  // Set the input connection to the extract selection filter.
  void SetSelection(vtkSMSourceProxy* selection);
  void CleanSelectionInput();

  // Description:
  // Called to update the Representation. 
  // Overridden to EnableLOD on the prop based on the ViewInformation.
  virtual void Update(vtkSMViewProxy* view);
  virtual void Update() { this->Superclass::Update(); }

  // Description:
  // Overridden to pass to LabelRepresentation.
  virtual void SetUpdateTime(double time);

  // Description:
  // Set the visibility for this representation.
  // Implemented to turn off label representation when visibility is turned off.
  virtual void SetVisibility(int visible);

  // Description:
  // Returns the proxy for the prop.
  vtkGetObjectMacro(Prop3D, vtkSMProxy);

//BTX
protected:
  vtkSMSelectionRepresentationProxy();
  ~vtkSMSelectionRepresentationProxy();

  // Description:
  // This representation needs a surface compositing strategy.
  // Overridden to request the correct type of strategy from the view.
  virtual bool InitializeStrategy(vtkSMViewProxy* view);

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

  // Proxies for the selection pipeline.
  vtkSMSourceProxy* ExtractSelection;
  vtkSMSourceProxy* GeometryFilter;
  vtkSMProxy* Mapper;
  vtkSMProxy* LODMapper;
  vtkSMProxy* Prop3D;
  vtkSMProxy* Property;
  vtkSMProxy* EmptySelectionSource;

  vtkSMDataLabelRepresentationProxy* LabelRepresentation;

private:
  vtkSMSelectionRepresentationProxy(const vtkSMSelectionRepresentationProxy&); // Not implemented
  void operator=(const vtkSMSelectionRepresentationProxy&); // Not implemented
//ETX
};

#endif

