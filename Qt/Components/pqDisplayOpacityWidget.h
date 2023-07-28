// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDisplayOpacityWidget_h
#define pqDisplayOpacityWidget_h

#include "pqComponentsModule.h"

#include <QWidget>

#include <memory>

class pqDataRepresentation;

/**
 * `pqDisplayOpacityWidget` enables a user to select the array that maps to
 * opacity on the volume representation. This feature is available only when
 * the representation is rendering volumes.
 *
 * This widget is composed of a `pqArraySelectorWidget` to present the available
 * arrays in a combobox and a `pqIntVectorPropertyWidget` enumerates the components
 * of the selected array in another combobox. Both these widgets are laid out
 * horizontally.
 */

class PQCOMPONENTS_EXPORT pqDisplayOpacityWidget : public QWidget
{
  Q_OBJECT
  using Superclass = QWidget;

public:
  pqDisplayOpacityWidget(QWidget* parent = nullptr);
  ~pqDisplayOpacityWidget() override;

  /**
   * Set the representation on which the scalar opacity array will be set.
   */
  void setRepresentation(pqDataRepresentation* display);

private:
  Q_DISABLE_COPY(pqDisplayOpacityWidget);

  void onArrayModified();

  class pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
