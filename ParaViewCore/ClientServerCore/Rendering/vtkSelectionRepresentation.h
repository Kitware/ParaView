/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSelectionRepresentation
 *
 * vtkSelectionRepresentation is a representation to show the extracted
 * cells. It uses vtkGeometryRepresentation and vtkPVDataRepresentation
 * internally.
 * @par Thanks:
 * The addition of a transformation matrix was supported by CEA/DIF
 * Commissariat a l'Energie Atomique, Centre DAM Ile-De-France, Arpajon, France.
*/

#ifndef vtkSelectionRepresentation_h
#define vtkSelectionRepresentation_h

#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports
#include "vtkPVDataRepresentation.h"

class vtkGeometryRepresentation;
class vtkDataLabelRepresentation;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkSelectionRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkSelectionRepresentation* New();
  vtkTypeMacro(vtkSelectionRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * One must change the internal representations only before the representation
   * is added to a view, after that it should not be touched.
   */
  void SetLabelRepresentation(vtkDataLabelRepresentation*);

  //@{
  /**
   * Overridden to simply pass the input to the internal representations. We
   * won't need this if vtkPVDataRepresentation correctly respected in the
   * arguments passed to it during ProcessRequest() etc.
   */
  virtual void SetInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void SetInputConnection(vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void AddInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void AddInputConnection(vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void RemoveInputConnection(int port, vtkAlgorithmOutput* input) VTK_OVERRIDE;
  virtual void RemoveInputConnection(int port, int idx) VTK_OVERRIDE;
  //@}

  /**
   * This needs to be called on all instances of vtkSelectionRepresentation when
   * the input is modified. This is essential since the geometry filter does not
   * have any real-input on the client side which messes with the Update
   * requests.
   */
  virtual void MarkModified() VTK_OVERRIDE;

  //@{
  /**
   * Passed on to internal representations as well.
   */
  virtual void SetUpdateTime(double time) VTK_OVERRIDE;
  virtual void SetForceUseCache(bool val) VTK_OVERRIDE;
  virtual void SetForcedCacheKey(double val) VTK_OVERRIDE;
  //@}

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   * Overridden to propagate to the active representation.
   */
  virtual void SetVisibility(bool val) VTK_OVERRIDE;

  //@{
  /**
   * Forwarded to GeometryRepresentation.
   */
  void SetColor(double r, double g, double b);
  void SetLineWidth(double val);
  void SetOpacity(double val);
  void SetPointSize(double val);
  void SetRepresentation(int val);
  void SetUseOutline(int);
  //@}

  //@{
  /**
   * Forwarded to GeometryRepresentation and LabelRepresentation
   */
  void SetOrientation(double, double, double);
  void SetOrigin(double, double, double);
  void SetPosition(double, double, double);
  void SetScale(double, double, double);
  void SetUserTransform(const double[16]);
  //@}

  //@{
  /**
   * Forwarded to vtkDataLabelRepresentation.
   */
  virtual void SetPointFieldDataArrayName(const char* val);
  virtual void SetCellFieldDataArrayName(const char* val);
  //@}

  /**
   * Override because of internal composite representations that need to be
   * initilized as well.
   */
  virtual unsigned int Initialize(
    unsigned int minIdAvailable, unsigned int maxIdAvailable) VTK_OVERRIDE;

protected:
  vtkSelectionRepresentation();
  ~vtkSelectionRepresentation();

  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

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

  /**
   * Fires UpdateDataEvent
   */
  void TriggerUpdateDataEvent();

  vtkGeometryRepresentation* GeometryRepresentation;
  vtkDataLabelRepresentation* LabelRepresentation;

private:
  vtkSelectionRepresentation(const vtkSelectionRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSelectionRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
