// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGeometryRepresentation
 * @brief   representation for showing any datasets as
 * external shell of polygons.
 *
 * vtkGeometryRepresentation is a representation for showing polygon geometry.
 * It handles non-polygonal datasets by extracting external surfaces. One can
 * use this representation to show surface/wireframe/points/surface-with-edges.
 * @par Thanks:
 * The addition of a transformation matrix was supported by CEA/DIF
 * Commissariat a l'Energie Atomique, Centre DAM Ile-De-France, Arpajon, France.
 */

#ifndef vtkGeometryRepresentation_h
#define vtkGeometryRepresentation_h

#include "vtkPVDataRepresentation.h"
#include "vtkParaViewDeprecation.h" // for PV_DEPRECATED
#include "vtkProperty.h"            // needed for VTK_POINTS etc.
#include "vtkRemotingViewsModule.h" // needed for exports
#include "vtkVector.h"              // for vtkVector.

#include <set>           // needed for std::set
#include <string>        // needed for std::string
#include <unordered_map> // needed for std::unordered_map
#include <vector>        // needed for std::vector

class vtkCompositeDataDisplayAttributes;
class vtkMapper;
class vtkPiecewiseFunction;
class vtkPVGeometryFilter;
class vtkPVLODActor;
class vtkScalarsToColors;
class vtkTexture;
class vtkTransform;

namespace vtkGeometryRepresentation_detail
{
// This is defined to either vtkQuadricClustering or vtkmLevelOfDetail in the
// implementation file:
class DecimationFilterType;
}

class VTKREMOTINGVIEWS_EXPORT vtkGeometryRepresentation : public vtkPVDataRepresentation
{

public:
  static vtkGeometryRepresentation* New();
  vtkTypeMacro(vtkGeometryRepresentation, vtkPVDataRepresentation);
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

  ///@{
  /**
   * Determines the number of distinct values in vtkBlockColors
   * See also vtkPVGeometryFilter
   */
  void SetBlockColorsDistinctValues(int distinctValues);
  int GetBlockColorsDistinctValues();
  ///@}

  /**
   * Enable/Disable LOD;
   */
  virtual void SetSuppressLOD(bool suppress) { this->SuppressLOD = suppress; }

  ///@{
  /**
   * Disables the lighting on the object.
   * It only displays the Diffuse component
   * of the lighting color of the object.
   */
  vtkBooleanMacro(DisableLighting, bool);
  vtkSetMacro(DisableLighting, bool);
  vtkGetMacro(DisableLighting, bool);
  ///@}
  ///@{
  /**
   * Set the lighting properties of the object. vtkGeometryRepresentation
   * overrides these based of the following conditions:
   * \li When Representation is wireframe or points, it disables diffuse or
   * specular.
   * \li When scalar coloring is employed, it disabled specular.
   */
  vtkSetMacro(Ambient, double);
  vtkSetMacro(Diffuse, double);
  vtkSetMacro(Specular, double);
  vtkGetMacro(Ambient, double);
  vtkGetMacro(Diffuse, double);
  vtkGetMacro(Specular, double);
  ///@}

  enum RepresentationTypes
  {
    POINTS = VTK_POINTS,
    WIREFRAME = VTK_WIREFRAME,
    SURFACE = VTK_SURFACE,
    SURFACE_WITH_EDGES = 3
  };

  ///@{
  /**
   * Set the shift scale method for the point coordinates
   * see vtkOpenGLVertexBufferObject.h for more information.
   */
  void SetCoordinateShiftScaleMethod(int val);
  int GetCoordinateShiftScaleMethod();
  ///@}

  ///@{
  /**
   * Set the representation type. This adds VTK_SURFACE_WITH_EDGES to those
   * defined in vtkProperty.
   */
  vtkSetClampMacro(Representation, int, POINTS, SURFACE_WITH_EDGES);
  vtkGetMacro(Representation, int);
  ///@}

  /**
   * Overload to set representation type using string. Accepted strings are:
   * "Points", "Wireframe", "Surface" and "Surface With Edges".
   */
  virtual void SetRepresentation(const char*);

  /**
   * Returns the data object that is rendered from the given input port.
   */
  vtkDataObject* GetRenderedDataObject(int port) override;

  ///@{
  /**
   * Representations that use geometry representation as the internal
   * representation should turn this flag off so that we don't end up requesting
   * ghost cells twice.
   */
  vtkSetMacro(RequestGhostCellsIfNeeded, bool);
  vtkGetMacro(RequestGhostCellsIfNeeded, bool);
  vtkBooleanMacro(RequestGhostCellsIfNeeded, bool);
  ///@}

  /**
   * Set the normal array used for smooth shading.
   * It must be a three components array.
   */
  virtual void SetNormalArray(const char* val);

  /**
   * Set the texture coordinates array used for texture mapping.
   * It must be a two components array.
   */
  virtual void SetTCoordArray(const char* val);

  /**
   * Set the tangent coordinates array used for normal mapping.
   * It must be a three components array.
   */
  virtual void SetTangentArray(const char* val);

  //***************************************************************************
  // Forwarded to vtkPVGeometryFilter
  virtual void SetUseOutline(int);
  void SetTriangulate(int);
  void SetNonlinearSubdivisionLevel(int);
  void SetMatchBoundariesIgnoringCellOrder(int);
  virtual void SetGenerateFeatureEdges(bool);
  void SetComputePointNormals(bool);
  void SetSplitting(bool);
  void SetFeatureAngle(double);

  //***************************************************************************
  // Forwarded to vtkProperty.
  virtual void SetAmbientColor(double r, double g, double b);
  virtual void SetColor(double r, double g, double b);
  virtual void SetDiffuseColor(double r, double g, double b);
  virtual void SetLighting(bool lighting);
  virtual void SetEdgeColor(double r, double g, double b);
  virtual void SetInteractiveSelectionColor(double r, double g, double b);
  virtual void SetInterpolation(int val);
  virtual void SetLineWidth(double val);
  virtual void SetOpacity(double val);
  virtual void SetEdgeOpacity(double val);
  virtual void SetPointSize(double val);
  virtual void SetSpecularColor(double r, double g, double b);
  virtual void SetSpecularPower(double val);
  virtual void SetLuminosity(double val);
  virtual void SetRenderPointsAsSpheres(bool);
  virtual void SetRenderLinesAsTubes(bool);
  virtual void SetRoughness(double val);
  virtual void SetMetallic(double val);
  virtual void SetEdgeTint(double r, double g, double b);
  virtual void SetAnisotropy(double val);
  virtual void SetAnisotropyRotation(double val);
  virtual void SetBaseIOR(double val);
  virtual void SetCoatIOR(double val);
  virtual void SetCoatStrength(double val);
  virtual void SetCoatRoughness(double val);
  virtual void SetCoatNormalScale(double val);
  virtual void SetCoatColor(double r, double g, double b);
  virtual void SetBaseColorTexture(vtkTexture* tex);
  virtual void SetMaterialTexture(vtkTexture* tex);
  virtual void SetAnisotropyTexture(vtkTexture* tex);
  virtual void SetNormalTexture(vtkTexture* tex);
  virtual void SetCoatNormalTexture(vtkTexture* tex);
  virtual void SetEmissiveTexture(vtkTexture* tex);
  virtual void SetNormalScale(double val);
  virtual void SetOcclusionStrength(double val);
  virtual void SetEmissiveFactor(double rval, double gval, double bval);
  virtual void SetShowTexturesOnBackface(bool);

  //***************************************************************************
  // Forwarded to Actor.
  virtual void SetOrientation(double, double, double);
  virtual void SetOrigin(double, double, double);
  virtual void SetPickable(int val);
  virtual void SetPosition(double, double, double);
  virtual void SetScale(double, double, double);
  virtual void SetTexture(vtkTexture*);
  virtual void SetUserTransform(const double[16]);
  PARAVIEW_DEPRECATED_IN_5_13_0("Use SetTextureTransform instead")
  virtual void SetFlipTextures(bool);
  virtual void SetCoordinateSystem(int);

  //***************************************************************************
  // Forwarded to all textures
  virtual void SetTextureTransform(vtkTransform*);
  vtkGetObjectMacro(TextureTransform, vtkTransform);
  virtual void SetRepeatTextures(bool);
  vtkGetMacro(RepeatTextures, bool);
  virtual void SetInterpolateTextures(bool);
  vtkGetMacro(InterpolateTextures, bool);
  virtual void SetUseMipmapTextures(bool);
  vtkGetMacro(UseMipmapTextures, bool);

  //***************************************************************************
  // Forwarded to Mapper and LODMapper.
  virtual void SetInterpolateScalarsBeforeMapping(int val);
  virtual void SetLookupTable(vtkScalarsToColors* val);
  virtual void SetColorMissingArraysWithNanColor(bool val);
  virtual void SetSeamlessU(bool);
  virtual void SetSeamlessV(bool);
  ///@{
  /**
   * Sets if scalars are mapped through a color-map or are used
   * directly as colors.
   * 0 maps to VTK_COLOR_MODE_DIRECT_SCALARS
   * 1 maps to VTK_COLOR_MODE_MAP_SCALARS
   * @see vtkScalarsToColors::MapScalars
   */
  virtual void SetMapScalars(int val);
  virtual void SetStatic(int val);
  ///@}

  /**
   * Sets the selection used by the mapper.
   */
  virtual void SetSelection(vtkSelection* selection);

  /**
   * Provides access to the actor used by this representation.
   */
  vtkPVLODActor* GetActor() { return this->GetRenderedProp(); }

  ///@{
  /**
   * Get/Set the name of the assembly to use for mapping block visibilities,
   * colors and opacities.
   *
   * The default is Hierarchy.
   */
  vtkSetStringMacro(ActiveAssembly);
  vtkGetStringMacro(ActiveAssembly);
  ///@}

  ///@{
  /**
   * Update list of selectors that determine the selected blocks.
   */
  void AddBlockSelector(const char* selector);
  void RemoveAllBlockSelectors();
  ///@}

  ///@{
  /**
   * Set the color for a single block.
   */
  void SetBlockColor(const char* selector, double r, double g, double b);
  void RemoveAllBlockColors();
  ///@}

  ///@{
  /**
   * Set the opacity for a single block.
   */
  void SetBlockOpacity(const char* selector, double opacity);
  void RemoveAllBlockOpacities();
  ///@}

  ///@{
  /**
   * Set if to interpolate scalars before mapping for a single block.
   */
  void SetBlockInterpolateScalarsBeforeMapping(const char* selector, bool interpolate);
  void RemoveAllBlockInterpolateScalarsBeforeMappings();
  ///@}

  ///@{
  /**
   * Set the block to map scalars for a single block.
   *
   * Note: Similar to SetMapScalars, but for blocks.
   */
  void SetBlockMapScalars(const char* selector, int val);
  void RemoveAllBlockMapScalars();
  ///@}

  ///@{
  /**
   * Set the color array for a single block.
   */
  void SetBlockArrayName(const char* selector, int assoc, const char* arrayName);
  void RemoveAllBlockArrayNames();
  ///@}

  ///@{
  /**
   * Set the look up table for a single block.
   *
   * Note: we need two methods because we can't wrap a string and a vtkObject using either
   * vtkSMStringVectorProperty or vtkSMProxyProperty.
   */
  void SetBlockLookupTable(vtkScalarsToColors* lut);
  void RemoveAllBlockLookupTables();
  ///@}

  /**
   * Convenience method to get the array name used to scalar color with.
   */
  const char* GetColorArrayName();

  /**
   * Convenience method to get bounds from a dataset/composite dataset.
   * If a vtkCompositeDataDisplayAttributes \a cdAttributes is provided and
   * if the input data \a dataObject is vtkCompositeDataSet, only visible
   * blocks of the data will be used to compute the bounds.
   * Returns true if valid bounds were computed.
   */
  static bool GetBounds(
    vtkDataObject* dataObject, double bounds[6], vtkCompositeDataDisplayAttributes* cdAttributes);

  ///@{
  /**
   * For OSPRay controls sizing of implicit spheres (points) and
   * cylinders (lines)
   */
  virtual void SetEnableScaling(int v);
  virtual void SetScalingArrayName(const char*);
  virtual void SetScalingFunction(vtkPiecewiseFunction* pwf);
  ///@}

  /**
   * For OSPRay, choose from among available materials.
   */
  virtual void SetMaterial(const char*);

  ///@{
  /**
   * Specify whether or not to redistribute the data. The default is false
   * since that is the only way in general to guarantee correct rendering.
   * Can set to true if all rendered data sets are based on the same
   * data partitioning in order to save on the data redistribution.
   */
  vtkSetMacro(UseDataPartitions, bool);
  vtkGetMacro(UseDataPartitions, bool);
  ///@}

  ///@{
  /**
   * Specify whether or not to shader replacements string must be used.
   */
  virtual void SetUseShaderReplacements(bool);
  vtkGetMacro(UseShaderReplacements, bool);
  ///@}

  /**
   * Specify shader replacements using a Json string.
   * Please refer to the XML definition of the property for details about
   * the expected Json string format.
   */
  virtual void SetShaderReplacements(const char*);

  /**
   * Specify the array names used for the selection.
   * Use nullptr to use default ones. ("vtkOriginalPointIds" and "vtkOriginalCellIds")
   */
  void SetArrayIdNames(const char* pointArray, const char* cellArray) override;

  ///@{
  /**
   * Set/Get the placeholder data type.
   *
   * This value is set by the proxy and is needed to ensure that the client knows the type of the
   * input that lives on the server.
   *
   * In the past, we were always creating a placeholder of type vtkMultiBlockDataSet when no
   * input was available on the client. This is no longer valid because vtkPVGeometryFilter
   * can now generate also vtkPartitionedDataSetCollection.
   *
   * This can be potentially removed in the future once vtkMultiBlockDataSet is deprecated.
   */
  void SetPlaceHolderDataType(int datatype);
  vtkGetMacro(PlaceHolderDataType, int);
  ///@}

protected:
  vtkGeometryRepresentation();
  ~vtkGeometryRepresentation() override;

  /**
   * This method is called in the constructor. If the subclasses override any of
   * the iVar vtkObject's of this class e.g. the Mappers, GeometryFilter etc.,
   * they should call this method again in their constructor. It must be totally
   * safe to call this method repeatedly.
   */
  virtual void SetupDefaults();

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
   * Overridden to request correct ghost-level to avoid internal surfaces.
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
   * Passes on parameters to vtkProperty and vtkMapper
   */
  virtual void UpdateColoringParameters();

  /**
   * Used in ConvertSelection to locate the prop used for actual rendering.
   */
  virtual vtkPVLODActor* GetRenderedProp() { return this->Actor; }

  // Progress Callback
  void HandleGeometryRepresentationProgress(vtkObject* caller, unsigned long, void*);

  /**
   * Block attributes in a mapper are referenced to each block through DataObject
   * pointers. Since DataObjects may change after updating the pipeline, this class
   * maintains an additional map using the flat-index as a key.  This method updates
   * the mapper's attributes with those cached in this representation; This is done
   * after the data has updated (multi-block nodes change after an update).
   */
  void PopulateBlockAttributes(vtkCompositeDataDisplayAttributes* attrs, vtkDataObject* outputData);

  /**
   * Computes the bounds of the visible data based on the block visibilities in the
   * composite data attributes of the mapper.
   */
  void ComputeVisibleDataBounds();

  /**
   * Update the mapper with the shader replacement strings if feature is enabled.
   */
  void UpdateShaderReplacements();

  /**
   * Returns true if this representation has translucent geometry. Unlike
   * `vtkActor::HasTranslucentPolygonalGeometry` which cannot be called in
   * `Update`, this method can be called in `Update` i.e. before the mapper has
   * all the data to decide if it is doing translucent rendering.
   */
  virtual bool NeedsOrderedCompositing();

  /**
   * Used by SetNormalArray, SetTCoordArray and SetTangentArray
   */
  virtual void SetPointArrayToProcess(int p, const char* val);

  /**
   * This is called whenever the texture transformation matrix changes.
   */
  void UpdateGeneralTextureTransform();

  vtkAlgorithm* GeometryFilter;
  vtkAlgorithm* MultiBlockMaker;
  vtkGeometryRepresentation_detail::DecimationFilterType* Decimator;
  vtkPVGeometryFilter* LODOutlineFilter;

  vtkMapper* Mapper;
  vtkMapper* LODMapper;
  vtkPVLODActor* Actor;
  vtkProperty* Property;

  char* ActiveAssembly = nullptr;
  bool RepeatTextures;
  bool InterpolateTextures;
  bool UseMipmapTextures;
  vtkTransform* TextureTransform;
  double Ambient;
  double Specular;
  double Diffuse;
  int Representation;
  bool SuppressLOD;
  bool RequestGhostCellsIfNeeded;
  double VisibleDataBounds[6];

  vtkTimeStamp VisibleDataBoundsTime;

  vtkPiecewiseFunction* PWF;

  bool UseDataPartitions;

  bool UseShaderReplacements;
  std::string ShaderReplacementsString;

  bool BlockAttrChanged = false;
  vtkTimeStamp BlockAttributeTime;
  bool UpdateBlockAttrLOD = false;

  // This is used to be able to create the correct placeHolder in RequestData for the client
  int PlaceHolderDataType = VTK_PARTITIONED_DATA_SET_COLLECTION;

  // These block variables are similar to the ones in vtkCompositeDataDisplayAttributes
  // Some of them are exposed and some others are not because, as of now, they are not needed.

  ///@{
  /**
   * Configurable through vtkGeometryRepresentation API
   */
  std::vector<std::string> BlockSelectors;
  std::vector<std::pair<std::string, vtkVector3d>> BlockColors;
  std::vector<std::pair<std::string, double>> BlockOpacities;
  std::vector<std::pair<std::string, bool>> BlockInterpolateScalarsBeforeMapping;
  std::vector<std::pair<std::string, int>> BlockColorModes;
  std::vector<std::pair<std::string, std::pair<int /*assoc*/, std::string>>> BlockArrayNames;
  std::vector<vtkScalarsToColors*> BlockLookupTables;
  ///@}
  ///@{
  /**
   * Configured internally in vtkGeometryRepresentation
   */
  std::vector<std::pair<std::string, bool>> BlockScalarVisibilities;
  std::vector<std::pair<std::string, bool>> BlockUseLookupTableScalarRanges;
  std::vector<std::pair<vtkDataObject*, vtkIdType>> BlockFieldDataTupleIds;
  ///@}
private:
  bool DisableLighting = false;
  vtkGeometryRepresentation(const vtkGeometryRepresentation&) = delete;
  void operator=(const vtkGeometryRepresentation&) = delete;
};

#endif
