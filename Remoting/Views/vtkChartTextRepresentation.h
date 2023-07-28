// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkChartTextRepresentation
 * @brief representation to add text to vtkPVContextView
 *
 * vtkChartTextRepresentation supports adding text to vtkPVContextView. It is
 * the `vtkTextSourceRepresentation` counterpart except for `vtkPVContextView`
 * instead of `vtkPVRenderView`. It adds a `vtkBlockItem` to the
 * `vtkContextScene` maintained by the `vtkPVContextView` to render the text.
 *
 * In theory, we can support interacting with the label to place it
 * interactively. For now, we only support anchoring the label at one of the
 * predefined anchor locations or specifying normalized coordinates manually.
 */

#ifndef vtkChartTextRepresentation_h
#define vtkChartTextRepresentation_h

#include "vtkNew.h" // for vtkNew
#include "vtkPVDataRepresentation.h"

class vtkBlockItem;
class vtkTextProperty;

class VTKREMOTINGVIEWS_EXPORT vtkChartTextRepresentation : public vtkPVDataRepresentation
{
public:
  static vtkChartTextRepresentation* New();
  vtkTypeMacro(vtkChartTextRepresentation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
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
  void SetLabelLocation(int location);
  ///@}

  ///@{
  /**
   * Get/Set the normalized text position in the scene when `AnyLocation` is used.
   * Default is { 0.05, 0.05 }.
   */
  vtkGetVector2Macro(Position, double);
  vtkSetVector2Macro(Position, double);
  ///@}

  ///@{
  /**
   * Set the interactivity.
   */
  void SetInteractivity(bool);
  ///@}

  ///@{
  /**
   * Forwarded to vtkBlockItem
   */
  void SetTextProperty(vtkTextProperty*);
  ///@}

  ///@{
  // Superclass overrides
  void SetVisibility(bool val) override;
  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;
  ///@}

protected:
  vtkChartTextRepresentation();
  ~vtkChartTextRepresentation() override = default;

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
  vtkChartTextRepresentation(const vtkChartTextRepresentation&) = delete;
  void operator=(const vtkChartTextRepresentation&) = delete;

  void OnInteractionEvent();

  vtkNew<vtkBlockItem> BlockItem;
  vtkNew<vtkTable> PreparedData;
  double Position[2] = { 0.05, 0.05 };
  int LabelLocation = vtkChartTextRepresentation::AnyLocation;
};

#endif
