/*=========================================================================

  Program:   ParaView
  Module:    vtkChartLogoRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkChartLogoRepresentation
 * @brief representation to add logo to vtkPVContextView
 *
 * vtkChartLogoRepresentation supports adding logo to vtkPVContextView. It is
 * the `vtkLogoSourceRepresentation` counterpart except for `vtkPVContextView`
 * instead of `vtkPVRenderView`. It adds a `vtkImageItem` to the
 * `vtkContextScene` maintained by the `vtkPVContextView` to render the logo.
 */

#ifndef vtkChartLogoRepresentation_h
#define vtkChartLogoRepresentation_h

#include "vtkNew.h" // for vtkNew
#include "vtkPVDataRepresentation.h"

class vtkImageItem;

class VTKREMOTINGVIEWS_EXPORT vtkChartLogoRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkChartLogoRepresentation* New();
  vtkTypeMacro(vtkChartLogoRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  enum
  {
    AnyLocation = 0,
    LowerLeftCorner,
    LowerRightCorner,
    LowerCenter,
    UpperLeftCorner,
    UpperRightCorner,
    UpperCenter
  };
  vtkSetClampMacro(LogoLocation, int, AnyLocation, UpperCenter);
  //@}

  //@{
  /**
   * Get/Set the position to use when `AnyLocation` is being used.
   */
  vtkGetVector2Macro(Position, double);
  vtkSetVector2Macro(Position, double);
  //@}

  //@{
  // Superclass overrides
  void SetVisibility(bool val) override;
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;
  //@}

protected:
  vtkChartLogoRepresentation();
  ~vtkChartLogoRepresentation() override;

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

private:
  vtkChartLogoRepresentation(const vtkChartLogoRepresentation&) = delete;
  void operator=(const vtkChartLogoRepresentation&) = delete;

  void OnInteractionEvent();

  vtkNew<vtkImageItem> ImageItem;
  vtkNew<vtkImageData> PreparedData;
  double Position[2] = { 0.05, 0.05 };
  int LogoLocation = AnyLocation;
};

#endif
