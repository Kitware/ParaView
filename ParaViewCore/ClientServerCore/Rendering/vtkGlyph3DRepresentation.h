/*=========================================================================

  Program:   ParaView
  Module:    vtkGlyph3DRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGlyph3DRepresentation
 *
 * vtkGlyph3DRepresentation is a representation that uses the vtkGlyph3DMapper
 * for rendering glyphs.
 * Note that vtkGlyph3DRepresentation requires that the "glyph" source data is
 * available on all rendering processes.
*/

#ifndef vtkGlyph3DRepresentation_h
#define vtkGlyph3DRepresentation_h

#include "vtkGeometryRepresentation.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class vtkGlyph3DMapper;
class vtkArrowSource;

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkGlyph3DRepresentation
  : public vtkGeometryRepresentation
{
public:
  static vtkGlyph3DRepresentation* New();
  vtkTypeMacro(vtkGlyph3DRepresentation, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  void MarkModified() override;

  /**
   * Toggle the visibility of the original mesh.
   * If this->GetVisibility() is false, then this has no effect.
   */
  void SetMeshVisibility(bool visible);

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   */
  void SetVisibility(bool) override;

  //**************************************************************************
  // Forwarded to vtkGlyph3DMapper
  void SetMaskArray(const char* val);
  void SetScaleArray(const char* val);
  void SetOrientationArray(const char* val);
  void SetSourceIndexArray(const char* val);
  void SetScaling(bool val);
  void SetScaleMode(int val);
  void SetScaleFactor(double val);
  void SetOrient(bool val);
  void SetOrientationMode(int val);
  void SetMasking(bool val);
  void SetSourceIndexing(bool val);
  void SetUseSourceTableTree(bool val);
  void SetUseCullingAndLOD(bool val);
  void SetNumberOfLOD(int val);
  void SetLODDistanceAndTargetReduction(int index, float dist, float reduc);
  void SetColorByLODIndex(bool val);

  //***************************************************************************
  // Overridden to forward to the vtkGlyph3DMapper.
  void SetInterpolateScalarsBeforeMapping(int val) override;
  void SetLookupTable(vtkScalarsToColors* val) override;
  void SetMapScalars(int val) override;
  void SetStatic(int val) override;

  //***************************************************************************
  // Overridden to forward to the vtkActor used for the glyphs (GlyphActor)
  void SetOrientation(double, double, double) override;
  void SetOrigin(double, double, double) override;
  void SetPickable(int val) override;
  void SetPosition(double, double, double) override;
  void SetScale(double, double, double) override;
  void SetTexture(vtkTexture*) override;
  void SetUserTransform(const double[16]) override;

protected:
  vtkGlyph3DRepresentation();
  ~vtkGlyph3DRepresentation() override;

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Overridden to request single piece from the Glyph source.
   */
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

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
   * Used in ConvertSelection to locate the prop used for actual rendering.
   */
  vtkPVLODActor* GetRenderedProp() override { return this->GlyphActor; }

  /**
   * Overridden to ensure that the coloring decisions are passed over to the
   * glyph mapper.
   */
  void UpdateColoringParameters() override;

  /**
   * Determines bounds using the vtkGlyph3DMapper.
   */
  void ComputeGlyphBounds(double bounds[6]);

  bool IsCached(double cache_key) override;

  vtkAlgorithm* GlyphMultiBlockMaker;
  vtkPVCacheKeeper* GlyphCacheKeeper;

  vtkGlyph3DMapper* GlyphMapper;
  vtkGlyph3DMapper* LODGlyphMapper;

  vtkPVLODActor* GlyphActor;
  vtkArrowSource* DummySource;

  bool MeshVisibility;

private:
  vtkGlyph3DRepresentation(const vtkGlyph3DRepresentation&) = delete;
  void operator=(const vtkGlyph3DRepresentation&) = delete;
};

#endif
