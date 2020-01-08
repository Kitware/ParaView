/*=========================================================================

  Program:   ParaView
  Module:    vtkGeometryRepresentationWithFaces.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGeometryRepresentationWithFaces
 *
 * vtkGeometryRepresentationWithFaces extends vtkGeometryRepresentation to add
 * support for rendering back and front faces separately (with different
 * visibility and properties).
*/

#ifndef vtkGeometryRepresentationWithFaces_h
#define vtkGeometryRepresentationWithFaces_h

#include "vtkGeometryRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkGeometryRepresentationWithFaces : public vtkGeometryRepresentation
{
public:
  static vtkGeometryRepresentationWithFaces* New();
  vtkTypeMacro(vtkGeometryRepresentationWithFaces, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum RepresentationTypesEx
  {
    FOLLOW_FRONTFACE = 400,
    CULL_BACKFACE = 401,
    CULL_FRONTFACE = 402
  };

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
   * Set the backface representation.
   */
  vtkSetClampMacro(BackfaceRepresentation, int, POINTS, CULL_FRONTFACE);
  vtkGetMacro(BackfaceRepresentation, int);
  //@}

  //***************************************************************************
  // Forwarded to vtkProperty(BackfaceProperty)
  void SetBackfaceAmbientColor(double r, double g, double b);
  void SetBackfaceDiffuseColor(double r, double g, double b);
  void SetBackfaceOpacity(double val);

protected:
  vtkGeometryRepresentationWithFaces();
  ~vtkGeometryRepresentationWithFaces() override;

  /**
   * This method is called in the constructor. If the subclasses override any of
   * the iVar vtkObject's of this class e.g. the Mappers, GeometryFilter etc.,
   * they should call this method again in their constructor. It must be totally
   * safe to call this method repeatedly.
   */
  void SetupDefaults() override;

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

  bool NeedsOrderedCompositing() override;

  vtkMapper* BackfaceMapper;
  vtkMapper* LODBackfaceMapper;
  vtkPVLODActor* BackfaceActor;
  vtkProperty* BackfaceProperty;
  int BackfaceRepresentation;

private:
  vtkGeometryRepresentationWithFaces(const vtkGeometryRepresentationWithFaces&) = delete;
  void operator=(const vtkGeometryRepresentationWithFaces&) = delete;
};

#endif
