// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqDoubleVectorPropertyWidget_h
#define pqDoubleVectorPropertyWidget_h

#include "pqPropertyWidget.h"

class PQCOMPONENTS_EXPORT pqDoubleVectorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqDoubleVectorPropertyWidget(
    vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parent = nullptr);

  ~pqDoubleVectorPropertyWidget() override;

  // Overridden to clear highlights from the pqHighlightablePushButton.
  void apply() override;
  void reset() override;

Q_SIGNALS:
  /**
   * internal signal used to clear highlights from pqHighlightablePushButton.
   */
  void clearHighlight();
  void highlightResetButton();

protected Q_SLOTS:
  /**
   * called when the user clicks the "reset" button for a specific property.
   */
  virtual void resetButtonClicked();

  void scaleHalf();
  void scaleTwice();
  void scale(double);

  /**
   * sets the value using active source's data bounds.
   */
  void resetToActiveDataBounds();

  /**
   * sets the value to the specified bounds.
   */
  void resetToBounds(const double bds[6]);

private:
  Q_DISABLE_COPY(pqDoubleVectorPropertyWidget)
};

#endif // pqDoubleVectorPropertyWidget_h
