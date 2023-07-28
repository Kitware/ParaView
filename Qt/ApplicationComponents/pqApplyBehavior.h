// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqApplyBehavior_h
#define pqApplyBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QScopedPointer>

class pqPipelineFilter;
class pqPipelineSource;
class pqPropertiesPanel;
class pqProxy;
class pqView;

/**
 * @ingroup Behaviors
 * pqApplyBehavior collects the logic that needs to happen after the user hits
 * "Apply" on the pqPropertiesPanel. Since "Apply" is an important concept in
 * ParaView application, it gets its own behavior so applications can customize
 * it, if needed.
 *
 * pqApplyBehavior can work with or without registered pqPropertiesPanel.
 * One could still manually register any pqPropertiesPanel instance(s).
 * pqParaViewBehaviors does that automatically
 * for pqPropertiesPanel instances available during the startup.
 *
 * This behavior is also responsible for managing the AutoApply mechanism
 * with is controllable using vtkPVGeneralSettings
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqApplyBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqApplyBehavior(QObject* parent = nullptr);
  ~pqApplyBehavior() override;

  ///@{
  /**
   * Register/unregister pqPropertiesPanel instances to monitor.
   */
  void registerPanel(pqPropertiesPanel* panel);
  void unregisterPanel(pqPropertiesPanel* panel);
  ///@}

  static void hideInputIfRequired(pqPipelineFilter* filter, pqView* view);

Q_SIGNALS:
  void triggerApply();

protected Q_SLOTS:
  virtual void applied(pqPropertiesPanel* panel, pqProxy* proxy);
  virtual void applied(pqPropertiesPanel* panel = nullptr);

private Q_SLOTS:
  void onApplied(pqProxy*);
  void onApplied();
  void onModified();
  void onResetDone();

protected:
  virtual void showData(pqPipelineSource* source, pqView* view);

private:
  Q_DISABLE_COPY(pqApplyBehavior)
  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
};

#endif
