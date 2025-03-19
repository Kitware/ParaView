// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqSelectionInputWidget_h
#define pqSelectionInputWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

#include "pqSMProxy.h" // For property.

/**
 * pqSelectionInputWidget is a custom widget used for specifying
 * the selection to use on filters that have a selection as input.
 */
class PQCOMPONENTS_EXPORT pqSelectionInputWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(pqSMProxy selection READ selection WRITE setSelection USER true)
  typedef QWidget Superclass;

public:
  pqSelectionInputWidget(QWidget* parent = nullptr);
  ~pqSelectionInputWidget() override;

  virtual pqSMProxy selection() { return this->AppendSelections; }

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  virtual void setSelection(pqSMProxy newAppendSelections);

  ///@{
  /**
   * Register and unregister selection source objects.
   * This is needed for undo/redo and Python tracing.
   * Call this in the `Apply` process, around the actual
   * Apply part.
   */
  virtual void preAccept();
  virtual void postAccept();
  ///@}

Q_SIGNALS:
  /**
   * Signal that the selection proxy changed.
   */
  void selectionChanged(pqSMProxy);

protected Q_SLOTS:
  // Copy active selection.
  void copyActiveSelection();

  void onActiveSelectionChanged();

  void updateLabels();

protected: // NOLINT(readability-redundant-access-specifiers)
  pqSMProxy AppendSelections;

private:
  Q_DISABLE_COPY(pqSelectionInputWidget)

  class pqUi;
  pqUi* Ui;
};

#endif
