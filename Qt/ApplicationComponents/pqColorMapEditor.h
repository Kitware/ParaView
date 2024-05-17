// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqColorMapEditor_h
#define pqColorMapEditor_h

#include "pqApplicationComponentsModule.h"
#include "vtkParaViewDeprecation.h" // For PARAVIEW_DEPRECATED

#include <QScopedPointer>
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

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set the selected properties type.
   */
  void setSelectedPropertiesType(int selectedPropertiesType);

protected Q_SLOTS:
  ///@{
  /**
   * slot called to update the currently showing proxies.
   */
  void updateActive(bool forceUpdate);
  void updateActive() { this->updateActive(false); }
  ///@}

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
  void setRepresentation(pqDataRepresentation* repr, bool forceUpdate = false);
  PARAVIEW_DEPRECATED_IN_5_13_0("Use setRepresentation instead.")
  void setDataRepresentation(pqDataRepresentation* repr, bool forceUpdate = false)
  {
    this->setRepresentation(repr, forceUpdate);
  }
  void setColorTransferFunctions(std::vector<vtkSMProxy*> ctfs);
  void setColorTransferFunction(vtkSMProxy* ctf)
  {
    this->setColorTransferFunctions(std::vector<vtkSMProxy*>(1, ctf));
  }

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
  QScopedPointer<pqInternals> Internals;
};

#endif
