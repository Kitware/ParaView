// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProtractorRepresentation
 *
 * vtkProtractorRepresentation is a representation to show a protractor. The input
 * is expected to be vtkPolyData with 3 points. It uses a vtkAngleRepresentation2D
 * for the actual display, so the lines and labels are always visible.
 *
 * @see vtkAngleRepresentation2D
 */

#ifndef vtkProtractorRepresentation_h
#define vtkProtractorRepresentation_h

#include "vtkAngleRepresentation2D.h" // vtkAngleRepresentation2D
#include "vtkNew.h"                   //needed for instances of vtkPolyData
#include "vtkPVDataRepresentation.h"
#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSmartPointer.h"        // vtkSmartPointer

class vtkPolyData;
class vtkProperty2D;
class vtkTextProperty;

class VTKREMOTINGVIEWS_EXPORT vtkProtractorRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkProtractorRepresentation* New();
  vtkTypeMacro(vtkProtractorRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the text widget.
   */
  void SetAngleRepresentation(vtkAngleRepresentation2D*);
  vtkGetSmartPointerMacro(AngleRepresentation, vtkAngleRepresentation2D);
  ///@}

  ///@{
  /**
   * Forward properties to the underlying representation.
   */
  void SetTextProperty(vtkTextProperty* prop);
  void SetLineColor(double r, double g, double b);
  void SetLineThickness(double thickness);
  void SetArrowStyle(int style);
  void SetLabelFactorSize(double size);
  void SetVisibility(bool) override;
  void SetLabelFormat(char* labelFormat);
  void SetAngleScale(double factor);
  ///@}

  /**
   * vtkAlgorithm::ProcessRequest() equivalent for rendering passes. This is
   * typically called by the vtkView to request meta-data from the
   * representations or ask them to perform certain tasks e.g.
   * PrepareForRendering.
   */
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

protected:
  vtkProtractorRepresentation();
  ~vtkProtractorRepresentation() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Fill input port information.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Adds the representation to the view.  This is called from
   * vtkView::AddRepresentation(). Add the underlying representation.
   * Returns true if the addition succeeds.
   */
  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.  This is called from
   * vtkView::RemoveRepresentation(). Remove the underlying representation.
   * Returns true if the removal succeeds.
   */
  bool RemoveFromView(vtkView* view) override;

  vtkSmartPointer<vtkAngleRepresentation2D> AngleRepresentation;
  vtkNew<vtkPolyData> Clone;

private:
  vtkProtractorRepresentation(const vtkProtractorRepresentation&) = delete;
  void operator=(const vtkProtractorRepresentation&) = delete;
};

#endif
