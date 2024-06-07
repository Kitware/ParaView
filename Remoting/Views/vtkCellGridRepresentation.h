// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridRepresentation
 * @brief   representation for rendering surfaces of cell-grid datasets.
 *
 * vtkCellGridRepresentation is a representation for showing
 * surfaces of finite-element cell-grids.
 * It handles volumetric datasets by computing arrays of external-surface
 * (cell-ID, side-ID)-tuples and rendering those.
 *
 * It can also render existing (cell-ID, side-ID) tuple-arrays to
 * illustrate the domains of imposed boundary- and initial-conditions.
 */

#ifndef vtkCellGridRepresentation_h
#define vtkCellGridRepresentation_h

#include "vtkGeometryRepresentation.h"

class VTKREMOTINGVIEWS_EXPORT vtkCellGridRepresentation : public vtkGeometryRepresentation
{

public:
  static vtkCellGridRepresentation* New();
  vtkTypeMacro(vtkCellGridRepresentation, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   */
  void SetVisibility(bool val) override;

  /**
   * Enable/Disable LOD;
   */
  void SetSuppressLOD(bool vtkNotUsed(suppress)) override {} // We do not support LOD yet.

  /**
   * Set which sides of the input to show.
   */
  void SetSidesToShow(int flags);

  /**
   * Set whether to preserve renderable inputs.
   */
  void SetPreserveRenderableInputs(bool shouldPreserve);

  /**
   * Set whether to omit sides for renderable inputs.
   */
  void SetOmitSidesForRenderableInputs(bool shouldOmit);

protected:
  vtkCellGridRepresentation();
  ~vtkCellGridRepresentation() override;

  /**
   * This method is called in the constructor. If the subclasses override any of
   * the iVar vtkObject's of this class e.g. the Mappers, GeometryFilter etc.,
   * they should call this method again in their constructor. It must be totally
   * safe to call this method repeatedly.
   */
  void SetupDefaults() override;

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Subclasses should override this to connect inputs to the internal pipeline
   * as necessary. Since most representations are "meta-filters" (i.e. filters
   * containing other filters), you should create shallow copies of your input
   * before connecting to the internal pipeline. The convenience method
   * GetInternalOutputPort will create a cached shallow copy of a specified
   * input for you. The related helper functions GetInternalAnnotationOutputPort,
   * GetInternalSelectionOutputPort should be used to obtain a selection or
   * annotation port whose selections are localized for a particular input data object.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

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
   * Passes on parameters to vtkProperty and vtkMapper
   */
  void UpdateColoringParameters() override;

  /**
   * Returns true if this representation has translucent geometry. Unlike
   * `vtkActor::HasTranslucentPolygonalGeometry` which cannot be called in
   * `Update`, this method can be called in `Update` i.e. before the mapper has
   * all the data to decide if it is doing translucent rendering.
   */
  bool NeedsOrderedCompositing() override;

#if 0
  /**
   * Used by SetNormalArray, SetTCoordArray and SetTangentArray
   */
  virtual void SetPointArrayToProcess(int p, const char* val);
#endif

private:
  vtkCellGridRepresentation(const vtkCellGridRepresentation&) = delete;
  void operator=(const vtkCellGridRepresentation&) = delete;
};

#endif
