// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLightsEditor_h
#define pqLightsEditor_h

#include "pqComponentsModule.h" // for exports
#include "pqPropertyGroupWidget.h"

class vtkSMProxy;
class vtkSMPropertyGroup;

class PQCOMPONENTS_EXPORT pqLightsEditor : public pqPropertyGroupWidget
{
  Q_OBJECT
  typedef pqPropertyGroupWidget Superclass;

public:
  pqLightsEditor(vtkSMProxy* proxy, vtkSMPropertyGroup* smGroup, QWidget* parent = nullptr);
  ~pqLightsEditor() override;

protected Q_SLOTS:
  void resetLights();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
