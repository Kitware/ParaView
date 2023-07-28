// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLightsInspector_h
#define pqLightsInspector_h

#include "pqComponentsModule.h" // for exports
#include <QWidget>

/**
 * @class pqLightsInspector
 * @brief widget to that lets user edit ParaView's lights
 *
 * pqLightsInspector is a QWidget that is used to allow user to view
 * and edit the lights in the active render view
 *
 */

class pqView;
class vtkSMProxy;

class PQCOMPONENTS_EXPORT pqLightsInspector : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqLightsInspector(
    QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags(), bool autotracking = true);
  ~pqLightsInspector() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void addLight();
  void removeLight(vtkSMProxy* = nullptr);
  void syncLightToCamera(vtkSMProxy* = nullptr);
  void resetLight(vtkSMProxy* = nullptr);
  void setActiveView(pqView*);
  void render();
  void updateAndRender();

private:
  Q_DISABLE_COPY(pqLightsInspector);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
