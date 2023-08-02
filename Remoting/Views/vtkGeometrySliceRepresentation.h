// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGeometrySliceRepresentation
 * @brief   extends vtkGeometryRepresentation to
 * add support for showing just specific slices from the dataset.
 *
 * vtkGeometrySliceRepresentation extends vtkGeometryRepresentation to show
 * slices from the dataset. This is used for vtkPVMultiSliceView and
 * vtkPVOrthographicSliceView.
 */

#ifndef vtkGeometrySliceRepresentation_h
#define vtkGeometrySliceRepresentation_h

#include "vtkGeometryRepresentation.h"

class VTKREMOTINGVIEWS_EXPORT vtkGeometrySliceRepresentation : public vtkGeometryRepresentation
{
public:
  static vtkGeometrySliceRepresentation* New();
  vtkTypeMacro(vtkGeometrySliceRepresentation, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int ProcessViewRequest(vtkInformationRequestKey* request_type, vtkInformation* inInfo,
    vtkInformation* outInfo) override;

  enum
  {
    X_SLICE_ONLY,
    Y_SLICE_ONLY,
    Z_SLICE_ONLY,
    ALL_SLICES
  };
  vtkSetClampMacro(Mode, int, X_SLICE_ONLY, ALL_SLICES);
  vtkGetMacro(Mode, int);

  ///@{
  /**
   * Get/Set whether original data outline should be shown in the view.
   */
  vtkSetMacro(ShowOutline, bool);
  vtkGetMacro(ShowOutline, bool);
  ///@}

protected:
  vtkGeometrySliceRepresentation();
  ~vtkGeometrySliceRepresentation() override;

  void SetupDefaults() override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  bool AddToView(vtkView* view) override;
  bool RemoveFromView(vtkView* view) override;

private:
  vtkGeometrySliceRepresentation(const vtkGeometrySliceRepresentation&) = delete;
  void operator=(const vtkGeometrySliceRepresentation&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
  int Mode;
  bool ShowOutline;
};

#endif
