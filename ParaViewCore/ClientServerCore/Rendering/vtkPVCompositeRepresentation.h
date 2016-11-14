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
/**
 * @class   vtkPVCompositeRepresentation
 * @brief   a data-representation used by ParaView.
 *
 * vtkPVCompositeRepresentation is a data-representation used by ParaView for showing
 * a type of data-set in the render view. It is a composite-representation with
 * some fixed representations for showing things like selection and polar axes.
 * This representation has 1 input port and it ensures that that input is passed
 * on to the internal representations
 * (except SelectionRepresentation and PolarAxesRepresentation) properly.
 * For SelectionRepresentation and PolarAxesRepresentation the client is expected
 * to setup the input (look at vtkSMPVRepresentationProxy).
*/

#ifndef vtkPVCompositeRepresentation_h
#define vtkPVCompositeRepresentation_h

#include "vtkCompositeRepresentation.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class vtkPolarAxesRepresentation;
class vtkSelectionRepresentation;
class vtkPVGridAxes3DRepresentation;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkPVCompositeRepresentation
  : public vtkCompositeRepresentation
{
public:
  static vtkPVCompositeRepresentation* New();
  vtkTypeMacro(vtkPVCompositeRepresentation, vtkCompositeRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * These must only be set during initialization before adding the
   * representation to any views or calling Update().
   */
  void SetSelectionRepresentation(vtkSelectionRepresentation*);
  void SetGridAxesRepresentation(vtkPVGridAxes3DRepresentation*);

  /**
   * This must only be set during initialization before adding the
   * representation to any views or calling Update().
   */
  void SetPolarAxesRepresentation(vtkPolarAxesRepresentation*);

  /**
   * Propagate the modification to all internal representations.
   */
  virtual void MarkModified() VTK_OVERRIDE;

  /**
   * Set visibility of the representation.
   * Overridden to update the cube-axes and selection visibilities.
   */
  virtual void SetVisibility(bool visible) VTK_OVERRIDE;

  /**
   * Set the selection visibility.
   */
  virtual void SetSelectionVisibility(bool visible);

  /**
   * Set the polar axes visibility.
   */
  virtual void SetPolarAxesVisibility(bool visible);

  //@{
  /**
   * Passed on to internal representations as well.
   */
  void SetUpdateTime(double time) VTK_OVERRIDE;
  void SetForceUseCache(bool val) VTK_OVERRIDE;
  void SetForcedCacheKey(double val) VTK_OVERRIDE;
  void SetInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  void SetInputConnection(vtkAlgorithmOutput* input) VTK_OVERRIDE;
  void AddInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  void AddInputConnection(vtkAlgorithmOutput* input) VTK_OVERRIDE;
  void RemoveInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  void RemoveInputConnection(int port, int idx) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Forwarded to vtkSelectionRepresentation.
   */
  virtual void SetPointFieldDataArrayName(const char*);
  virtual void SetCellFieldDataArrayName(const char*);
  //@}

  /**
   * Override because of internal composite representations that need to be
   * initilized as well.
   */
  virtual unsigned int Initialize(
    unsigned int minIdAvailable, unsigned int maxIdAvailable) VTK_OVERRIDE;

protected:
  vtkPVCompositeRepresentation();
  ~vtkPVCompositeRepresentation();

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

  vtkSelectionRepresentation* SelectionRepresentation;
  vtkPVGridAxes3DRepresentation* GridAxesRepresentation;
  vtkPolarAxesRepresentation* PolarAxesRepresentation;

  bool SelectionVisibility;

private:
  vtkPVCompositeRepresentation(const vtkPVCompositeRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPVCompositeRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
