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
  vtkTypeMacro(vtkSMSelectionRepresentationProxy, vtkSMDataRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to pass the view information to all the internal
  // representations.
  virtual void SetViewInformation(vtkInformation*);

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
  // Returns true if this representation is visible.
  // This returns the actual representation visibility which may be different
  // from the one set using SetVisibility() based on whether a selection source
  // has been set on the input proxy.
  virtual bool GetVisibility();

  // Description:
  // Set the visibility for point labels. The "Visibility" overrides this flag,
  // i.e. when Visibility is false, point labels are also not shown.
  vtkSetMacro(PointLabelVisibility, int);

  // Description:
  // Set the visibility for cell labels. The "Visibility" overrides this flag,
  // i.e. when Visibility is false, cell labels are also not shown.
  vtkSetMacro(CellLabelVisibility, int);
  
  // Description:
  // Returns the proxy for the prop.
  vtkGetObjectMacro(Prop3D, vtkSMProxy);

  // Description:
  // When set to true, the UpdateTime for this representation is linked to the
  // ViewTime for the view to which this representation is added (default
  // behaviour). Otherwise the update time is independent of the ViewTime.
  // Overridden to pass value to internal representations. 
  virtual void SetUseViewUpdateTime(bool);

  // Description:
  // Called by the view to pass the view's update time to the representation.
  // Overridden to pass value to internal representations. 
  virtual void SetViewUpdateTime(double time);

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

  // Description:
  // The actual visibility of props is function of whether the user enabled the
  // visibility of this representation and whether the selection input is set.
  void UpdateVisibility();

  // Proxies for the selection pipeline.
  vtkSMSourceProxy* GeometryFilter;
  vtkSMProxy* Mapper;
  vtkSMProxy* LODMapper;
  vtkSMProxy* Prop3D;
  vtkSMProxy* Property;

  vtkSMDataLabelRepresentationProxy* LabelRepresentation;

  bool UserRequestedVisibility;
  int PointLabelVisibility;
  int CellLabelVisibility;
private:
  vtkSMSelectionRepresentationProxy(const vtkSMSelectionRepresentationProxy&); // Not implemented
  void operator=(const vtkSMSelectionRepresentationProxy&); // Not implemented
//ETX
};

#endif

