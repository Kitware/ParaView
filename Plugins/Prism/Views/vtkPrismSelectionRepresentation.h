// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPrismSelectionRepresentation
 * @brief   representation for selection in PrismView
 *
 * vtkPrismSelectionRepresentation is a representation for selection in PrismView.
 */
#ifndef vtkPrismSelectionRepresentation_h
#define vtkPrismSelectionRepresentation_h

#include "vtkPrismViewsModule.h" // for exports
#include "vtkSelectionRepresentation.h"

class VTKPRISMVIEWS_EXPORT vtkPrismSelectionRepresentation : public vtkSelectionRepresentation
{
public:
  static vtkPrismSelectionRepresentation* New();
  vtkTypeMacro(vtkPrismSelectionRepresentation, vtkSelectionRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set If the Data are simulation data or not. If they are, they need to be converted to the prism
   * space.
   */
  void SetIsSimulationData(bool isSimulationData);
  bool GetIsSimulationData();
  ///@}

  ///@{
  /**
   * Control which AttributeType the filter operates on (point data or cell data
   * for vtkDataSets). The default value is vtkDataObject::POINT. The input value for
   * this function should be either vtkDataObject::POINT or vtkDataObject::CELL.
   *
   * The default is vtkDataObject::CELL.
   */
  void SetAttributeType(int type);
  int GetAttributeType();
  ///@}

  ///@{
  /**
   * Set the name of the array to use as the X axis.
   */
  void SetXArrayName(const char* name);
  const char* GetXArrayName();
  ///@}

  ///@{
  /**
   * Set the name of the array to use as the Y axis.
   */
  void SetYArrayName(const char* name);
  const char* GetYArrayName();
  ///@}

  ///@{
  /**
   * Set the name of the array to use as the Z axis.
   */
  void SetZArrayName(const char* name);
  const char* GetZArrayName();
  ///@}

protected:
  vtkPrismSelectionRepresentation();
  ~vtkPrismSelectionRepresentation() override;

  /**
   * Fires UpdateDataEvent
   */
  void TriggerUpdateDataEvent() override;

private:
  vtkPrismSelectionRepresentation(const vtkPrismSelectionRepresentation&) = delete;
  void operator=(const vtkPrismSelectionRepresentation&) = delete;
};

#endif // vtkPrismSelectionRepresentation_h
