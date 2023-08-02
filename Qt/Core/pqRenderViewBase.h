// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRenderViewBase_h
#define pqRenderViewBase_h

#include "pqSMProxy.h" //needed for pqSMProxy.
#include "pqView.h"
class pqTimer;

/**
 * pqRenderViewBase is an abstract base class for all render-view based views.
 * It encapuslates some of the commonly needed functionality for all such
 * views.
 */
class PQCORE_EXPORT pqRenderViewBase : public pqView
{
  Q_OBJECT
  typedef pqView Superclass;

protected:
  // Constructor:
  // \c type  :- view type.
  // \c group :- SManager registration group name.
  // \c name  :- SManager registration name.
  // \c view  :- RenderView proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqRenderViewBase(const QString& type, const QString& group, const QString& name,
    vtkSMViewProxy* renModule, pqServer* server, QObject* parent = nullptr);

public:
  // Destructor.
  ~pqRenderViewBase() override;

  /**
   * Resets the camera to include all visible data.
   * It is essential to call this resetCamera, to ensure that the reset camera
   * action gets pushed on the interaction undo stack.
   *
   * OffsetRatio can be used to add a zoom offset (only applicable when closest is true).
   */
  virtual void resetCamera(bool closest = false, double offsetRatio = 0.9) = 0;

  /**
   * Called to reset the view's display.  This method calls resetCamera().
   */
  void resetDisplay(bool closest = false) override;

protected Q_SLOTS:
  virtual void initializeAfterObjectsCreated();

  /**
   * Triggered by DelayNonInteractiveRenderEvent
   */
  void beginDelayInteractiveRender();
  void endDelayInteractiveRender();

  /**
   * Triggered by internal timer to update the status bar message
   */
  void updateStatusMessage();

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Overridden to popup the context menu, if some actions have been added
   * using addMenuAction.
   */
  bool eventFilter(QObject* caller, QEvent* e) override;

  /**
   * Creates a new instance of the QWidget subclass to be used to show this
   * view. Default implementation creates a pqQVTKWidget.
   */
  QWidget* createWidget() override;

  /**
   * Use this method to initialize the pqObject state using the underlying
   * vtkSMProxy. This needs to be done only once, after the object has been
   * created.
   */
  void initialize() override;

private:
  Q_DISABLE_COPY(pqRenderViewBase)

  class pqInternal;
  pqInternal* Internal;
  pqTimer* InteractiveDelayUpdateTimer;
};

#endif
