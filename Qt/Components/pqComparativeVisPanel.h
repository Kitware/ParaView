// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqComparativeVisPanel_h
#define pqComparativeVisPanel_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqView;
class vtkSMProxy;
class vtkSMProperty;
class vtkEventQtSlotConnect;

/**
 * pqComparativeVisPanel is a properties page for the comparative view. It
 * allows the user to change the layout of the grid as well as add/remove
 * parameters to compare in the view.
 */
class PQCOMPONENTS_EXPORT pqComparativeVisPanel : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqComparativeVisPanel(QWidget* parent = nullptr);
  ~pqComparativeVisPanel() override;

  /**
   * Access the current view being shown by this panel.
   */
  pqView* view() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the view to shown in this panel. If the view is not a comparative view
   * then the panel will be disabled, otherwise, it shows the properties of the
   * view.
   */
  void setView(pqView*);

protected Q_SLOTS:
  /**
   * Called when the "+" button is clicked to add a new parameter.
   */
  void addParameter();

  /**
   * Updates the list of animated parameters from the proxy.
   */
  void updateParametersList();

  /**
   * Called when the selection in the active parameters widget changes.
   */
  void parameterSelectionChanged();

  void sizeUpdated();

  /**
   * Triggered when user clicks the delete button to remove a parameter.
   */
  void removeParameter(int index);

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Finds the row (-1 if none found) for the given (proxy,property).
   */
  int findRow(vtkSMProxy* animatedProxy, const QString& animatedPName, int animatedIndex);

private:
  Q_DISABLE_COPY(pqComparativeVisPanel)

  vtkEventQtSlotConnect* VTKConnect;
  class pqInternal;
  pqInternal* Internal;
};

#endif
