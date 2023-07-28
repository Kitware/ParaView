// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPauseLiveSourcePropertyWidget_h
#define pqPauseLiveSourcePropertyWidget_h

#include "pqApplicationComponentsModule.h" // needed for exports
#include "pqPropertyWidget.h"

/**
 * @class pqPauseLiveSourcePropertyWidget
 * @brief widget to pause all live sources
 *
 * pqPauseLiveSourcePropertyWidget can be added to a property on any source that
 * is a "live source" i.e. has the `<LiveSource>` XML hint. This will add a
 * button to the UI that allows the user to pause all live sources.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPauseLiveSourcePropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqPauseLiveSourcePropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqPauseLiveSourcePropertyWidget() override;

private:
  Q_DISABLE_COPY(pqPauseLiveSourcePropertyWidget)
};

#endif
