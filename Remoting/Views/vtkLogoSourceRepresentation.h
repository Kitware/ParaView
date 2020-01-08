/*=========================================================================

  Program:   ParaView
  Module:    vtkLogoSourceRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLogoSourceRepresentation
 *
 * vtkLogoSourceRepresentation is a representation to show a Logo. The input is
 * expected to a be a vtkImageData that can be passed to a vtkLogoRepresentation.
*/

#ifndef vtkLogoSourceRepresentation_h
#define vtkLogoSourceRepresentation_h

#include "vtkNew.h" // for vtkNew
#include "vtkPVDataRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports

class vtk3DWidgetRepresentation;
class vtkImageData;
class VTKREMOTINGVIEWS_EXPORT vtkLogoSourceRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkLogoSourceRepresentation* New();
  vtkTypeMacro(vtkLogoSourceRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the Logo widget.
   */
  void SetLogoWidgetRepresentation(vtk3DWidgetRepresentation* widget);
  vtkGetObjectMacro(LogoWidgetRepresentation, vtk3DWidgetRepresentation);
  //@}

  /**
   * Set the visibility.
   */
  void SetVisibility(bool) override;

  /**
   * Set the interactivity.
   */
  void SetInteractivity(bool);

  //@{
  /**
   * Set the opacity of the logo
   */
  vtkSetMacro(Opacity, double);
  vtkGetMacro(Opacity, double);
  //@}

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

protected:
  vtkLogoSourceRepresentation();
  ~vtkLogoSourceRepresentation() override;

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

  vtkNew<vtkImageData> ImageCache;
  vtk3DWidgetRepresentation* LogoWidgetRepresentation = nullptr;
  double Opacity = 1.0;

private:
  vtkLogoSourceRepresentation(const vtkLogoSourceRepresentation&) = delete;
  void operator=(const vtkLogoSourceRepresentation&) = delete;
};

#endif
