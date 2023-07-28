// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDisplayColor2Widget_h
#define pqDisplayColor2Widget_h

#include "pqComponentsModule.h"

#include <QWidget>

#include <memory>

class pqDataRepresentation;

/**
 * pqDisplayColor2Widget enables a user to select the array corresponding to the y-axis
 * of a 2D transfer function. This feature is available when the representation
 * is rendering volumes.
 *
 * This widget uses a `pqArraySelectorWidget` to present the available arrays in a combobox and
 * a pqIntVectorPropertyWidget enumerates the components of the selected array in another combobox.
 * Both the widgets are laid out horizontally.
 *
 * The first array in this widget is "Gradient Magnitude". This entry corresponds the
 * "UseGradientForTransfer2D" property on the volume representation.
 * As a result, this entry will always be present and it is the default.
 */
class PQCOMPONENTS_EXPORT pqDisplayColor2Widget : public QWidget
{
  Q_OBJECT
  using Superclass = QWidget;

public:
  pqDisplayColor2Widget(QWidget* parent = nullptr);
  ~pqDisplayColor2Widget() override;

  /**
   * Set the representation on which the scalar Color2 array will be set.
   */
  void setRepresentation(pqDataRepresentation* display);

private:
  Q_DISABLE_COPY(pqDisplayColor2Widget);

  void onArrayModified();

  class pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
