// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqMultiBlockPropertiesEditorWidget_h
#define pqMultiBlockPropertiesEditorWidget_h

#include "pqComponentsModule.h"
#include "pqPropertyGroupWidget.h"

#include <QScopedPointer>

/**
 * This is a pqPropertyGroupWidget subclass that presents a widget to edit several
 * representation properties of a multi-block dataset. It's used as the
 * "widget" for \c BlockPropertiesEditor property group.
 */
class PQCOMPONENTS_EXPORT pqMultiBlockPropertiesEditorWidget : public pqPropertyGroupWidget
{
  Q_OBJECT
public:
  typedef pqPropertyGroupWidget Superclass;

  pqMultiBlockPropertiesEditorWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smGroup, QWidget* parent = nullptr);
  ~pqMultiBlockPropertiesEditorWidget() override;

private Q_SLOTS:
  void updateMapScalarsWidget();
  void updateInterpolateScalarsBeforeMappingWidget();
  void updateOpacityWidget();

private: // NOLINT(readability-redundant-access-specifiers)
  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  Q_DISABLE_COPY(pqMultiBlockPropertiesEditorWidget)
};

#endif // pqMultiBlockPropertiesEditorWidget_h
