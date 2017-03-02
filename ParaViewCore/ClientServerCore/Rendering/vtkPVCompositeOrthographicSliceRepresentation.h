/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeOrthographicSliceRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVCompositeOrthographicSliceRepresentation
 *
 * vtkPVCompositeOrthographicSliceRepresentation is designed for use with
 * vtkPVOrthographicSliceView. We add 3 vtkGeometrySliceRepresentation
 * instances to render the 3 slices in vtkPVOrthographicSliceView's orthographic
 * views.
*/

#ifndef vtkPVCompositeOrthographicSliceRepresentation_h
#define vtkPVCompositeOrthographicSliceRepresentation_h

#include "vtkPVCompositeRepresentation.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer.

class vtkGeometrySliceRepresentation;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVCompositeOrthographicSliceRepresentation
  : public vtkPVCompositeRepresentation
{
public:
  static vtkPVCompositeOrthographicSliceRepresentation* New();
  vtkTypeMacro(vtkPVCompositeOrthographicSliceRepresentation, vtkPVCompositeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  void SetSliceRepresentation(int index, vtkGeometrySliceRepresentation*);
  void SetSliceRepresentation0(vtkGeometrySliceRepresentation* repr)
  {
    this->SetSliceRepresentation(0, repr);
  }
  void SetSliceRepresentation1(vtkGeometrySliceRepresentation* repr)
  {
    this->SetSliceRepresentation(1, repr);
  }
  void SetSliceRepresentation2(vtkGeometrySliceRepresentation* repr)
  {
    this->SetSliceRepresentation(2, repr);
  }

  /**
   * Set visibility of the representation.
   * Overridden to update the cube-axes and selection visibilities.
   */
  virtual void SetVisibility(bool visible) VTK_OVERRIDE;

  //@{
  /**
   * Overridden to simply pass the input to the internal representations. We
   * won't need this if vtkDataRepresentation correctly respected in the
   * arguments passed to it during ProcessRequest() etc.
   */
  virtual void SetInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void SetInputConnection(vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void AddInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void AddInputConnection(vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void RemoveInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void RemoveInputConnection(int port, int index) VTK_OVERRIDE;
  //@}

  /**
   * Propagate the modification to all internal representations.
   */
  virtual void MarkModified() VTK_OVERRIDE;

  /**
   * Override because of internal composite representations that need to be
   * initilized as well.
   */
  virtual unsigned int Initialize(
    unsigned int minIdAvailable, unsigned int maxIdAvailable) VTK_OVERRIDE;

protected:
  vtkPVCompositeOrthographicSliceRepresentation();
  ~vtkPVCompositeOrthographicSliceRepresentation();

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  virtual bool AddToView(vtkView* view) VTK_OVERRIDE;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  virtual bool RemoveFromView(vtkView* view) VTK_OVERRIDE;

  vtkSmartPointer<vtkGeometrySliceRepresentation> SliceRepresentations[3];

private:
  vtkPVCompositeOrthographicSliceRepresentation(
    const vtkPVCompositeOrthographicSliceRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVCompositeOrthographicSliceRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
