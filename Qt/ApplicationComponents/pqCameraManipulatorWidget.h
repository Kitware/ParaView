// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCameraManipulatorWidget_h
#define pqCameraManipulatorWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

/**
 * pqCameraManipulatorWidget used on "Camera2DManipulators" and
 * "Camera3DManipulators" property on a RenderView proxy.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCameraManipulatorWidget : public pqPropertyWidget
{
  Q_OBJECT;
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(QList<QVariant> manipulatorTypes READ manipulatorTypes WRITE setManipulatorTypes);

public:
  pqCameraManipulatorWidget(
    vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqCameraManipulatorWidget() override;

  /**
   * returns a list for the selected manipulator types. This has exactly 9
   * items always.
   */
  QList<QVariant> manipulatorTypes() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the manipulator types selection. This must have exactly 9 items or
   * less. Any missing items are treated as "None".
   */
  void setManipulatorTypes(const QList<QVariant>& value);

Q_SIGNALS:
  void manipulatorTypesChanged();

private:
  Q_DISABLE_COPY(pqCameraManipulatorWidget)
  class pqInternals;
  pqInternals* Internals;
  class PropertyLinksConnection;
};

#endif
