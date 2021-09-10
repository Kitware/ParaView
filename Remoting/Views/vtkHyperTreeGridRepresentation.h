/*=========================================================================

  Program:   ParaView
  Module:    vtkHyperTreeGridRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridRepresentation
 * @brief   representation for showing any datasets as
 * external shell of polygons.
 *
 * vtkHyperTreeGridRepresentation is a representation for showing polygon geometry.
 * It handles non-polygonal datasets by extracting external surfaces. One can
 * use this representation to show surface/wireframe/points/surface-with-edges.
 * @par Thanks:
 * The addition of a transformation matrix was supported by CEA/DIF
 * Commissariat a l'Energie Atomique, Centre DAM Ile-De-France, Arpajon, France.
 */

#ifndef vtkHyperTreeGridRepresentation_h
#define vtkHyperTreeGridRepresentation_h

#include "vtkNew.h" // for vtkNew
#include "vtkPVDataRepresentation.h"
#include "vtkProperty.h"            // needed for VTK_POINTS etc.
#include "vtkRemotingViewsModule.h" // needed for exports
#include "vtkVector.h"              // for vtkVector.

#include <string> // needed for std::string

class vtkActor;
class vtkOpenGLHyperTreeGridMapper;
class vtkPVGeometryFilter;
class vtkPiecewiseFunction;
class vtkScalarsToColors;
class vtkTexture;

namespace vtkHyperTreeGridRepresentation_detail
{
// This is defined to either vtkQuadricClustering or vtkmLevelOfDetail in the
// implementation file:
class DecimationFilterType;
}

class VTKREMOTINGVIEWS_EXPORT vtkHyperTreeGridRepresentation : public vtkPVDataRepresentation
{

public:
  static vtkHyperTreeGridRepresentation* New();
  vtkTypeMacro(vtkHyperTreeGridRepresentation, vtkPVDataRepresentation);
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

  //@{
  /**
   * Use adaptive decimation to only render the part inside the camera frustum
   */
  vtkGetMacro(AdaptiveDecimation, bool);
  vtkSetMacro(AdaptiveDecimation, bool);
  vtkBooleanMacro(AdaptiveDecimation, bool);
  //@}

  //@{
  /**
   * Set the lighting properties of the object. vtkHyperTreeGridRepresentation
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
  //@}

  enum RepresentationTypes
  {
    WIREFRAME = VTK_WIREFRAME,
    SURFACE = VTK_SURFACE,
    SURFACE_WITH_EDGES = 3
  };

  //@{
  /**
   * Set the representation type: WIREFRAME SURFACE SURFACE_WITH_EDGES
   */
  vtkSetClampMacro(Representation, int, WIREFRAME, SURFACE_WITH_EDGES);
  vtkGetMacro(Representation, int);
  //@}

  /**
   * Overload to set representation type using string. Accepted strings are:
   * "Points", "Wireframe", "Surface" and "Surface With Edges".
   */
  virtual void SetRepresentation(const char*);

  /**
   * Returns the data object that is rendered from the given input port.
   */
  vtkDataObject* GetRenderedDataObject(int port) override;

  //***************************************************************************
  // Forwarded to vtkPVGeometryFilter
  virtual void SetUseOutline(int);

  //***************************************************************************
  // Forwarded to vtkProperty.
  virtual void SetColor(double r, double g, double b);
  virtual void SetEdgeColor(double r, double g, double b);
  virtual void SetLineWidth(double val);
  virtual void SetOpacity(double val);
  virtual void SetRenderLinesAsTubes(bool);

  //***************************************************************************
  // Forwarded to Mapper and LODMapper.
  virtual void SetInterpolateScalarsBeforeMapping(int val);
  virtual void SetLookupTable(vtkScalarsToColors* val);
  //@{
  /**
   * Sets if scalars are mapped through a color-map or are used
   * directly as colors.
   * 0 maps to VTK_COLOR_MODE_DIRECT_SCALARS
   * 1 maps to VTK_COLOR_MODE_MAP_SCALARS
   * @see vtkScalarsToColors::MapScalars
   */
  virtual void SetMapScalars(int val);
  virtual void SetStatic(int val);
  //@}

  /**
   * Sets the selection used by the mapper.
   */
  virtual void SetSelection(vtkSelection* selection);

  /**
   * Provides access to the actor used by this representation.
   */
  vtkActor* GetActor() { return this->GetRenderedProp(); }

  //@{
  /**
   * Get/Set the name of the assembly to use for mapping block visibilities,
   * colors and opacities.
   *
   * This is simply a placeholder for the future. Since this
   * representation doesn't really support PartitionedDataSetCollections and
   * hence assembly, the only assembly supported is the
   * `vtkDataAssemblyUtilities::HierarchyName`. All others are simply ignored.
   */
  void SetActiveAssembly(const char*){};
  //@}

  /**
   * Convenience method to get the array name used to scalar color with.
   */
  const char* GetColorArrayName();

protected:
  vtkHyperTreeGridRepresentation();
  ~vtkHyperTreeGridRepresentation() override;

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
  virtual vtkActor* GetRenderedProp() { return this->Actor; }

  /**
   * Returns true if this representation has translucent geometry. Unlike
   * `vtkActor::HasTranslucentPolygonalGeometry` which cannot be called in
   * `Update`, this method can be called in `Update` i.e. before the mapper has
   * all the data to decide if it is doing translucent rendering.
   */
  virtual bool NeedsOrderedCompositing();

  /*
   * Fields
   */

  vtkNew<vtkOpenGLHyperTreeGridMapper> Mapper;
  vtkActor* Actor;
  vtkProperty* Property;

  double Ambient;
  double Specular;
  double Diffuse;
  int Representation;
  bool AdaptiveDecimation;
  double VisibleDataBounds[6];

private:
  vtkHyperTreeGridRepresentation(const vtkHyperTreeGridRepresentation&) = delete;
  void operator=(const vtkHyperTreeGridRepresentation&) = delete;
};

#endif
