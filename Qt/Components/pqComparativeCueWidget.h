// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqComparativeCueWidget_h
#define pqComparativeCueWidget_h

#include "pqComponentsModule.h"
#include "pqTimer.h"
#include "vtkSmartPointer.h"
#include <QTableWidget>

class vtkEventQtSlotConnect;
class vtkSMComparativeAnimationCueProxy;
class vtkSMProxy;

/**
 * pqComparativeCueWidget is designed to be used by
 * pqComparativeVisPanel to show/edit the values for an
 * vtkSMComparativeAnimationCueProxy.
 */
class PQCOMPONENTS_EXPORT pqComparativeCueWidget : public QTableWidget
{
  Q_OBJECT
  typedef QTableWidget Superclass;

public:
  pqComparativeCueWidget(QWidget* parent = nullptr);
  ~pqComparativeCueWidget() override;

  /**
   * Get/Set the cue that is currently being shown/edited by this widget.
   */
  void setCue(vtkSMProxy*);
  vtkSMComparativeAnimationCueProxy* cue() const;

  QSize size() const { return this->Size; }

  // Returns if this cue can accept more than 1 value as a parameter value.
  bool acceptsMultipleValues() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the comparative grid size.
   */
  void setSize(int w, int h)
  {
    this->Size = QSize(w, h);
    this->updateGUIOnIdle();
  }

Q_SIGNALS:
  // triggered every time the user changes the values.
  void valuesChanged();

protected Q_SLOTS:
  /**
   * refreshes the GUI with values from the proxy.
   */
  void updateGUI();

  void updateGUIOnIdle() { this->IdleUpdateTimer.start(); }

  void onSelectionChanged() { this->SelectionChanged = true; }

  void onCellChanged(int x, int y);

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * called when mouse is released. We use this to popup the range editing
   * dialog if the selection changed.
   */
  void mouseReleaseEvent(QMouseEvent* evt) override;

  void editRange();

private:
  Q_DISABLE_COPY(pqComparativeCueWidget)

  vtkEventQtSlotConnect* VTKConnect;
  bool InUpdateGUI;
  bool SelectionChanged;
  pqTimer IdleUpdateTimer;
  QSize Size;
  vtkSmartPointer<vtkSMComparativeAnimationCueProxy> Cue;
};

#endif
