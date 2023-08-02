// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPluginDocumentationBehavior_h
#define pqPluginDocumentationBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class QHelpEngine;
class vtkPVPlugin;

/**
 * @ingroup Behaviors
 * pqPluginDocumentationBehavior is a helper class that ensures that
 * documentation from plugins is registered with the QHelpEngine.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPluginDocumentationBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPluginDocumentationBehavior(QHelpEngine* engine);
  ~pqPluginDocumentationBehavior() override;

protected Q_SLOTS:
  void updatePlugin(vtkPVPlugin*);
  void updatePlugins();
  void refreshHelpEngine();

private:
  Q_DISABLE_COPY(pqPluginDocumentationBehavior)

  class pqInternals;
  pqInternals* Internals;
};

#endif
