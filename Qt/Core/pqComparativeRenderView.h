// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqComparativeRenderView_h
#define pqComparativeRenderView_h

#include "pqRenderView.h"

class vtkSMComparativeViewProxy;

/**
 * RenderView used for comparative visualization (or film-strip visualization).
 */
class PQCORE_EXPORT pqComparativeRenderView : public pqRenderView
{
  Q_OBJECT
  typedef pqRenderView Superclass;

public:
  static QString comparativeRenderViewType() { return "ComparativeRenderView"; }

  // Constructor:
  // \c group :- SManager registration group name.
  // \c name  :- SManager registration name.
  // \c view  :- RenderView proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqComparativeRenderView(const QString& group, const QString& name, vtkSMViewProxy* renModule,
    pqServer* server, QObject* parent = nullptr);
  ~pqComparativeRenderView() override;

  /**
   * Returns the comparative view proxy.
   */
  vtkSMComparativeViewProxy* getComparativeRenderViewProxy() const;

  /**
   * Returns the root render view in the comparative view.
   */
  vtkSMRenderViewProxy* getRenderViewProxy() const override;

protected Q_SLOTS:
  /**
   * Called when the layout on the comparative vis changes.
   */
  void updateViewWidgets(QWidget* container = nullptr);

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Creates a new instance of the QWidget subclass to be used to show this
   * view. Default implementation creates a pqQVTKWidgetBase.
   */
  QWidget* createWidget() override;

private:
  Q_DISABLE_COPY(pqComparativeRenderView)

  class pqInternal;
  pqInternal* Internal;
};

#endif
