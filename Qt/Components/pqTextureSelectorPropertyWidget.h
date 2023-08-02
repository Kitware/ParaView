// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqTextureSelectorPropertyWidget_h
#define pqTextureSelectorPropertyWidget_h

#include "pqComponentsModule.h"

#include "pqPropertyWidget.h"
#include "vtkNew.h"

/**
 * Property widget for selecting the texture to apply to a surface.
 *
 * To use this widget for a property add the 'panel_widget="texture_selector"'
 * to the property's XML. Also support the hint <TextureSelectorWidget />. See
 * property hints documentation for more details.
 */
class pqTextureComboBox;
class pqDataRepresentation;
class PQCOMPONENTS_EXPORT pqTextureSelectorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT

public:
  pqTextureSelectorPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqTextureSelectorPropertyWidget() override = default;

protected Q_SLOTS:
  void onTextureChanged(vtkSMProxy* texture);
  void onPropertyChanged();
  void checkAttributes(bool tcoords, bool tangents);

private:
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  pqTextureComboBox* Selector;
  pqDataRepresentation* Representation = nullptr;
  pqView* View = nullptr;
};

#endif // pqTextureSelectorPropertyWidget_h
