/*=========================================================================

  Program:   ParaView
  Module:    vtkSkyboxRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSkyboxRepresentation
 * @brief   representation for showing slices from a
 * vtkImageData.
 *
 * vtkSkyboxRepresentation is a representation for showing an image as a skybox.
 */

#ifndef vtkSkyboxRepresentation_h
#define vtkSkyboxRepresentation_h

#include "vtkNew.h"                         // for vtkNew.
#include "vtkOpenVRRepresentationsModule.h" // for export macro
#include "vtkPVDataRepresentation.h"

class vtkImageData;
class vtkSkybox;

class VTKOPENVRREPRESENTATIONS_EXPORT vtkSkyboxRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkSkyboxRepresentation* New();
  vtkTypeMacro(vtkSkyboxRepresentation, vtkPVDataRepresentation);
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
   * Overridden to propagate to the active representation.
   */
  void SetVisibility(bool val) override;

  /**
   * Provides access to the actor used by this representation.
   */
  vtkSkybox* GetActor() { return this->Actor; }

  //@{
  /**
   * ethods farwarded to the skybox instance
   */
  virtual void SetFloorPlane(float, float, float, float);
  virtual void SetFloorRight(float, float, float);
  virtual void SetProjection(int);
  //@}

protected:
  vtkSkyboxRepresentation();
  ~vtkSkyboxRepresentation() override;

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

  vtkSkybox* Actor;
  vtkNew<vtkImageData> SliceData;

private:
  vtkSkyboxRepresentation(const vtkSkyboxRepresentation&) = delete;
  void operator=(const vtkSkyboxRepresentation&) = delete;
};

#endif
