// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqGlyphScaleFactorPropertyWidget_h
#define pqGlyphScaleFactorPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqDoubleVectorPropertyWidget.h"

/**
 * pqGlyphScaleFactorPropertyWidget is used for the "Scale Factor" property on
 * the Glyph filter. It customizes the resetButtonClicked() logic since the
 * Glyph filter's scale factor setup is custom.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqGlyphScaleFactorPropertyWidget
  : public pqDoubleVectorPropertyWidget
{
  Q_OBJECT
  typedef pqDoubleVectorPropertyWidget Superclass;

public:
  pqGlyphScaleFactorPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqGlyphScaleFactorPropertyWidget() override;

protected Q_SLOTS:
  /**
   * update the property's value using the domain.
   */
  void resetButtonClicked() override;

private:
  Q_DISABLE_COPY(pqGlyphScaleFactorPropertyWidget)
};

#endif
