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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  virtual int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) VTK_OVERRIDE;

  virtual void MarkModified() VTK_OVERRIDE;

  /**
   * Toggle the visibility of the original mesh.
   * If this->GetVisibility() is false, then this has no effect.
   */
  void SetMeshVisibility(bool visible);

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   */
  virtual void SetVisibility(bool) VTK_OVERRIDE;

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

  //***************************************************************************
  // Overridden to forward to the vtkGlyph3DMapper.
  virtual void SetInterpolateScalarsBeforeMapping(int val) VTK_OVERRIDE;
  virtual void SetLookupTable(vtkScalarsToColors* val) VTK_OVERRIDE;
  virtual void SetMapScalars(int val) VTK_OVERRIDE;
  virtual void SetStatic(int val) VTK_OVERRIDE;

  //***************************************************************************
  // Overridden to forward to the vtkActor used for the glyphs (GlyphActor)
  virtual void SetOrientation(double, double, double) VTK_OVERRIDE;
  virtual void SetOrigin(double, double, double) VTK_OVERRIDE;
  virtual void SetPickable(int val) VTK_OVERRIDE;
  virtual void SetPosition(double, double, double) VTK_OVERRIDE;
  virtual void SetScale(double, double, double) VTK_OVERRIDE;
  virtual void SetTexture(vtkTexture*) VTK_OVERRIDE;
  virtual void SetUserTransform(const double[16]) VTK_OVERRIDE;

protected:
  vtkGlyph3DRepresentation();
  ~vtkGlyph3DRepresentation();

  /**
   * Fill input port information.
   */
  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  /**
   * Overridden to request single piece from the Glyph source.
   */
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

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
   * Used in ConvertSelection to locate the prop used for actual rendering.
   */
  virtual vtkPVLODActor* GetRenderedProp() VTK_OVERRIDE { return this->GlyphActor; }

  /**
   * Overridden to ensure that the coloring decisions are passed over to the
   * glyph mapper.
   */
  virtual void UpdateColoringParameters() VTK_OVERRIDE;

  /**
   * Determines bounds using the vtkGlyph3DMapper.
   */
  void ComputeGlyphBounds(double bounds[6]);

  virtual bool IsCached(double cache_key) VTK_OVERRIDE;

  vtkAlgorithm* GlyphMultiBlockMaker;
  vtkPVCacheKeeper* GlyphCacheKeeper;

  vtkGlyph3DMapper* GlyphMapper;
  vtkGlyph3DMapper* LODGlyphMapper;

  vtkPVLODActor* GlyphActor;
  vtkArrowSource* DummySource;

  bool MeshVisibility;

private:
  vtkGlyph3DRepresentation(const vtkGlyph3DRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGlyph3DRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
