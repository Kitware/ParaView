/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVCompositeRepresentation - a data-representation used by ParaView.
// .SECTION Description
// vtkPVCompositeRepresentation is a data-representation used by ParaView for showing
// a type of data-set in the render view. It is a composite-representation with
// some fixed representations for showing things like selection and cube-axes.
// This representation has 1 input port and it ensures that that input is passed
// on to the internal representations (except SelectionRepresentation) properly.
// For SelectionRepresentation, the client is expected to setup the input (look
// at vtkSMPVRepresentationProxy).

#ifndef vtkPVCompositeRepresentation_h
#define vtkPVCompositeRepresentation_h

#include "vtkCompositeRepresentation.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class vtkSelectionRepresentation;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVCompositeRepresentation : public vtkCompositeRepresentation
{
public:
  static vtkPVCompositeRepresentation* New();
  vtkTypeMacro(vtkPVCompositeRepresentation, vtkCompositeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These must only be set during initialization before adding the
  // representation to any views or calling Update().
  void SetSelectionRepresentation(vtkSelectionRepresentation*);

  // Description:
  // Propagate the modification to all internal representations.
  virtual void MarkModified();

  // Description:
  // Set visibility of the representation.
  // Overridden to update the cube-axes and selection visibilities.
  virtual void SetVisibility(bool visible);

  // Description:
  // Set the selection visibility.
  virtual void SetSelectionVisibility(bool visible);

  // Description:
  // Passed on to internal representations as well.
  virtual void SetUpdateTime(double time);
  virtual void SetForceUseCache(bool val);
  virtual void SetForcedCacheKey(double val);

  // Description:
  // Forwarded to vtkSelectionRepresentation.
  virtual void SetPointFieldDataArrayName(const char*);
  virtual void SetCellFieldDataArrayName(const char*);

  // Description:
  // Override because of internal composite representations that need to be
  // initilized as well.
  virtual unsigned int Initialize(unsigned int minIdAvailable, unsigned int maxIdAvailable);

protected:
  vtkPVCompositeRepresentation();
  ~vtkPVCompositeRepresentation();

  // Description:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().  Subclasses should override this method.
  // Returns true if the addition succeeds.
  virtual bool AddToView(vtkView* view);

  // Description:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  virtual bool RemoveFromView(vtkView* view);

  vtkSelectionRepresentation* SelectionRepresentation;
  bool SelectionVisibility;

private:
  vtkPVCompositeRepresentation(const vtkPVCompositeRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVCompositeRepresentation&) VTK_DELETE_FUNCTION;

};

#endif
