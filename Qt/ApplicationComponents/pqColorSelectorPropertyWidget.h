// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqColorSelectorPropertyWidget_h
#define pqColorSelectorPropertyWidget_h

#include "pqApplicationComponentsModule.h"

#include "pqPropertyWidget.h"

/**
 * A property widget with a tool button for selecting a single color.
 *
 * To use this widget for a property add the 'panel_widget="color_selector"' attribute
 * to the property's XML. To use this widget for a property whose color might possibly
 * come from the global color palette, add the 'panel_widget="color_selector_with_palette"'
 * attribute to the property's XML.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqColorSelectorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT

public:
  pqColorSelectorPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, bool withPalette, QWidget* parent = nullptr);
  ~pqColorSelectorPropertyWidget() override;
};

#endif // pqColorSelectorPropertyWidget_h
