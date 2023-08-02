// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSkyboxMovieRepresentation
 *
 * vtkSkyboxMovieRepresentation is a representation to show 360 movies
 */

#ifndef vtkSkyboxMovieRepresentation_h
#define vtkSkyboxMovieRepresentation_h

#include "vtkNew.h" // for ivars
#include "vtkPVDataRepresentation.h"
#include "vtkXRInterfaceRepresentationsModule.h" // for export macro

class vtkOpenGLMovieSphere;
class vtkFFMPEGVideoSource;

class VTKXRINTERFACEREPRESENTATIONS_EXPORT vtkSkyboxMovieRepresentation
  : public vtkPVDataRepresentation
{
public:
  static vtkSkyboxMovieRepresentation* New();
  vtkTypeMacro(vtkSkyboxMovieRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the visibility for this representation. When the visibility of
   * representation of false, all view passes are ignored.
   * Overridden to propagate to the active representation.
   */
  void SetVisibility(bool val) override;

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  void SetPlay(bool);

  ///@{
  /**
   * Methods farwarded to the skybox instance
   */
  virtual void SetFloorPlane(float, float, float, float);
  virtual void SetFloorRight(float, float, float);
  virtual void SetProjection(int);
  ///@}

protected:
  vtkSkyboxMovieRepresentation();
  ~vtkSkyboxMovieRepresentation() override;

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Overridden to invoke vtkCommand::UpdateDataEvent.
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

  vtkNew<vtkPolyData> DummyPolyData;
  vtkNew<vtkOpenGLMovieSphere> Actor;
  vtkFFMPEGVideoSource* MovieSource = nullptr;
  std::string FileName;
  bool Play = true;
  void* InternalAudioClass = nullptr;

private:
  vtkSkyboxMovieRepresentation(const vtkSkyboxMovieRepresentation&) = delete;
  void operator=(const vtkSkyboxMovieRepresentation&) = delete;
};

#endif
