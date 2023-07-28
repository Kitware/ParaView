// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqColorPaletteSelectorWidget_h
#define pqColorPaletteSelectorWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include <QPointer>

class QComboBox;

/**
 * @class pqColorPaletteSelectorWidget
 * @brief widget to choose a color palette to load/select.
 *
 * pqColorPaletteSelectorWidget is a pqPropertyWidget intended to be used in two
 * roles:
 * 1. To load a specific color palette.
 * 2. To select a specific color palette.
 *
 * Mode (1) is used when the widget is used for a `vtkSMProperty` e.g. **'Load
 * Palette'** property on the **ColorPalette** proxy. In that case, the
 * user's action is expected to update the proxy with the chosen palette.
 *
 * Mode (2) is used when the widget is used for a `vtkSMStringVectorProperty`
 * e.g. **OverrideColorPalette** property on **ImageOptions** proxy. In that
 * case, the selected palette name is simply set on the
 * vtkSMStringVectorProperty.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqColorPaletteSelectorWidget : public pqPropertyWidget
{
  Q_OBJECT
  Q_PROPERTY(QString paletteName READ paletteName WRITE setPaletteName)
  typedef pqPropertyWidget Superclass;

public:
  pqColorPaletteSelectorWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqColorPaletteSelectorWidget() override;

  QString paletteName() const;
  void setPaletteName(const QString& name);

Q_SIGNALS:
  void paletteNameChanged();

private Q_SLOTS:
  void loadPalette(int);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqColorPaletteSelectorWidget)
  QPointer<QComboBox> ComboBox;
};

#endif
