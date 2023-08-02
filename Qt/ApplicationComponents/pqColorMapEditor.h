// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqColorMapEditor_h
#define pqColorMapEditor_h

#include "pqApplicationComponentsModule.h"
#include <QWidget>

class vtkSMProxy;
class pqDataRepresentation;

/**
 * pqColorMapEditor is a widget that can be used to edit the active color-map,
 * if any. The panel is implemented as an auto-generated panel (similar to the
 * Properties panel) that shows the properties on the lookup-table proxy.
 * Custom widgets such as pqColorOpacityEditorWidget,
 * pqColorAnnotationsPropertyWidget, and others are used to
 * control certain properties on the proxy.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqColorMapEditor : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqColorMapEditor(QWidget* parent = nullptr);
  ~pqColorMapEditor() override;

protected Q_SLOTS:
  /**
   * slot called to update the currently showing proxies.
   */
  void updateActive();

  /**
   * slot called to update the visible widgets.
   */
  void updatePanel();

  /**
   * render's view when transfer function is modified.
   */
  void renderViews();

  /**
   * Save the current transfer function(s) as default.
   */
  void saveAsDefault();

  /**
   * Save the current transfer function(s) as default for arrays with
   * the same name as the selected array.
   */
  void saveAsArrayDefault();

  /**
   * Restore the defaults (undoes effects of saveAsDefault()).
   */
  void restoreDefaults();

  /**
   * called when AutoUpdate button is toggled.
   */
  void setAutoUpdate(bool);

  void updateIfNeeded();

protected: // NOLINT(readability-redundant-access-specifiers)
  void setDataRepresentation(pqDataRepresentation* repr);
  void setColorTransferFunction(vtkSMProxy* ctf);

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * update the enabled state for show/edit scalar bar buttons.
   */
  void updateScalarBarButtons();
  void updateColorArraySelectorWidgets();
  void updateOpacityArraySelectorWidgets();
  void updateColor2ArraySelectorWidgets();

private:
  Q_DISABLE_COPY(pqColorMapEditor)
  class pqInternals;
  pqInternals* Internals;
  friend class pqInternals;
};

#endif
