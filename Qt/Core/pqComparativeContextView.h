// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqComparativeContextView_h
#define pqComparativeContextView_h

#include "pqContextView.h"
#include <QPointer>

class vtkSMComparativeViewProxy;

/**
 * The abstract base class for comparative chart views. It handles the layout
 * of the individual chart views in the comparative view.
 */
class PQCORE_EXPORT pqComparativeContextView : public pqContextView
{
  Q_OBJECT
  typedef pqContextView Superclass;

public:
  ~pqComparativeContextView() override;

  /**
   * \returns the internal vtkContextView which provides the implementation for
   * the chart rendering.
   */
  vtkContextView* getVTKContextView() const override;

  /**
   * Returns the context view proxy associated with this object.
   */
  vtkSMContextViewProxy* getContextViewProxy() const override;

  /**
   * \returns the comparative view proxy.
   */
  vtkSMComparativeViewProxy* getComparativeViewProxy() const;

  /**
   * Returns the proxy of the root plot view in the comparative view.
   */
  virtual vtkSMViewProxy* getViewProxy() const;

protected Q_SLOTS:
  /**
   * Called when the layout on the comparative vis changes.
   */
  void updateViewWidgets();

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Create the QWidget for this view.
   */
  QWidget* createWidget() override;

  /**
   * Constructor:
   * \c type  :- view type.
   * \c group :- SManager registration group.
   * \c name  :- SManager registration name.
   * \c view  :- View proxy.
   * \c server:- server on which the proxy is created.
   * \c parent:- QObject parent.
   */
  pqComparativeContextView(const QString& type, const QString& group, const QString& name,
    vtkSMComparativeViewProxy* view, pqServer* server, QObject* parent = nullptr);

  QPointer<QWidget> Widget;

private:
  Q_DISABLE_COPY(pqComparativeContextView)

  class pqInternal;
  pqInternal* Internal;
};

#endif
