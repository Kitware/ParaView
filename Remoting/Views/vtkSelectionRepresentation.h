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

#include "vtkPVDataRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class vtkGeometryRepresentation;
class vtkDataLabelRepresentation;

class VTKREMOTINGVIEWS_EXPORT vtkSelectionRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkSelectionRepresentation* New();
  vtkTypeMacro(vtkSelectionRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

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
  void SetInputConnection(int port, vtkAlgorithmOutput* input) override;
  void SetInputConnection(vtkAlgorithmOutput* input) override;
  void AddInputConnection(int port, vtkAlgorithmOutput* input) override;
  void AddInputConnection(vtkAlgorithmOutput* input) override;
  void RemoveInputConnection(int port, vtkAlgorithmOutput* input) override;
  void RemoveInputConnection(int port, int idx) override;
  //@}

  /**
   * This needs to be called on all instances of vtkSelectionRepresentation when
   * the input is modified. This is essential since the geometry filter does not
   * have any real-input on the client side which messes with the Update
   * requests.
   */
  void MarkModified() override;

  //@{
  /**
   * Passed on to internal representations as well.
   */
  void SetUpdateTime(double time) override;
  void SetForceUseCache(bool val) override;
  void SetForcedCacheKey(double val) override;
  //@}

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   * Overridden to propagate to the active representation.
   */
  void SetVisibility(bool val) override;

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
  void SetRenderPointsAsSpheres(bool);
  void SetRenderLinesAsTubes(bool);
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
   * initialized as well.
   */
  unsigned int Initialize(unsigned int minIdAvailable, unsigned int maxIdAvailable) override;

  /**
   * Overridden to pass logname to internal GeometryRepresentation.
   */
  void SetLogName(const std::string&) override;

protected:
  vtkSelectionRepresentation();
  ~vtkSelectionRepresentation() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation().  Subclasses should override this method.
   * Returns true if the addition succeeds.
   */
  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation().  Subclasses should override this method.
   * Returns true if the removal succeeds.
   */
  bool RemoveFromView(vtkView* view) override;

  /**
   * Fires UpdateDataEvent
   */
  void TriggerUpdateDataEvent();

  vtkGeometryRepresentation* GeometryRepresentation;
  vtkDataLabelRepresentation* LabelRepresentation;

private:
  vtkSelectionRepresentation(const vtkSelectionRepresentation&) = delete;
  void operator=(const vtkSelectionRepresentation&) = delete;
};

#endif
