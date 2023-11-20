// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPropertiesPanel_h
#define pqPropertiesPanel_h

#include "pqComponentsModule.h"

#include <QWidget>

class pqDataRepresentation;
class pqOutputPort;
class pqPipelineSource;
class pqPropertyWidget;
class pqProxy;
class pqView;
class vtkSMProperty;
class vtkSMProxy;

/**
 * pqPropertiesPanel is the default panel used by paraview to edit source
 * properties and display properties for pipeline objects. pqPropertiesPanel
 * supports auto-generating widgets for properties of the proxy as well as a
 * mechanism to provide custom widgets/panels for the proxy or its
 * representations. pqPropertiesPanel uses pqProxyWidget to create and manage
 * the widgets for the source and representation proxies.
 *
 * pqPropertiesPanel comprises of 3 separate parts for showing the source
 * properties, display properties and view properties. One can control which
 * parts are shown by setting the panelMode property.
 */
class PQCOMPONENTS_EXPORT pqPropertiesPanel : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(int panelMode READ panelMode WRITE setPanelMode);
  typedef QWidget Superclass;

public:
  pqPropertiesPanel(QWidget* parent = nullptr);
  ~pqPropertiesPanel() override;

  /**
   * Returns the current view, if any.
   */
  pqView* view() const;

  /**
   * methods used to obtain the recommended spacing and margins to be used for
   * widgets.
   */
  static int suggestedMargin() { return 0; }
  static QMargins suggestedMargins() { return QMargins(0, 0, 0, 0); }
  static int suggestedHorizontalSpacing() { return 4; }
  static int suggestedVerticalSpacing() { return 4; }

  enum
  {
    SOURCE_PROPERTIES = 0x01,
    DISPLAY_PROPERTIES = 0x02,
    VIEW_PROPERTIES = 0x04,
    ALL_PROPERTIES = SOURCE_PROPERTIES | DISPLAY_PROPERTIES | VIEW_PROPERTIES
  };

  /**
   * Get/Set the panel mode.
   */
  void setPanelMode(int val);
  int panelMode() const { return this->PanelMode; }

  /**
   * Returns true if there are changes to be applied.
   */
  bool canApply();

  /**
   * Returns true if there are changes to be reset.
   */
  bool canReset();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Apply the changes properties to the proxies.
   *
   * This is triggered when the user clicks the "Apply" button on the
   * properties panel.
   */
  void apply();

  /**
   * Reset the changes made.
   *
   * This is triggered when the user clicks the "Reset" button on the
   * properties panel.
   */
  void reset();

  /**
   * Shows the help dialog.
   *
   * This is triggered when the user clicks the "?" button on the
   * properties panel.
   */
  void showHelp();

  /**
   * Restores the application defaults for the source properties.
   *
   * This is triggered when the user clicks the button with the
   * reload button next to the properties button.
   */
  void propertiesRestoreDefaults();

  /**
   * Saves the current property settings as default.
   *
   * This is triggered when the user clicks the button with the
   * save icon next to the properties button.
   */
  void propertiesSaveAsDefaults();

  /**
   * Restores the application defaults for the display properties.
   *
   * This is triggered when the user clicks the button with the
   * reload button next to the display button.
   */
  void displayRestoreDefaults();

  /**
   * Saves the current display settings as default.
   *
   * This is triggered when the user clicks the button with the
   * save icon next to the display button.
   */
  void displaySaveAsDefaults();

  /**
   * Restores the application defaults for the view properties.
   *
   * This is triggered when the user clicks the button with the
   * reload button next to the view button.
   */
  void viewRestoreDefaults();

  /**
   * Saves the current view settings as default.
   *
   * This is triggered when the user clicks the button with the
   * save icon next to the view button.
   */
  void viewSaveAsDefaults();

  /**
   * Set the view currently managed by the panel, should be called
   * automatically when the active view changes.
   */
  void setView(pqView*);

  /**
   * Set the `pqProxy` to show properties for under the "Properties" section.
   * Typically, this is a pqPipelineSource (or subclass), pqOutputPort, or
   * a pqExtractor.
   */
  void setPipelineProxy(pqProxy*);

  /**
   * Set the representation currently managed by the panel, should be called
   * automatically when the active representation changes.
   */
  void setRepresentation(pqDataRepresentation*);
Q_SIGNALS:
  /**
   * This signal is emitted after the user clicks the apply button.
   */
  void applied();

  /**
   * This signal is emitted after a panel for a proxy is applied.
   */
  void applied(pqProxy*);

  /**
   * This signal is emitted when the current view changes.
   */
  void viewChanged(pqView*);

  void modified();
  void resetDone();

  /**
   * This signal is emitted when the user clicks the help button.
   */
  void helpRequested(const QString& groupname, const QString& proxyType);

  /**
   * This signal is emitted when the user clicks the delete button.
   */
  void deleteRequested(pqProxy* source);

  /**
   * This signal is emitted when the apply button's enable state changes.
   * This is intended for other controls that call apply on the panel so
   * that they can be enabled/disabled correctly (i.e. menu items).
   */
  void applyEnableStateChanged();

private Q_SLOTS:
  /**
   * This is called when the user clicks the "Delete" button on the properties
   * panel. This triggers the deleteRequested() signal with proper arguments.
   */
  void deleteProxy();

  /**
   * slot gets called when a proxy is deleted.
   */
  void proxyDeleted(pqProxy*);

  /**
   * Updates the entire panel (properties+display) using the current
   * port/representation.
   */
  void updatePanel();

  /**
   * Updates the display part of the panel alone, unlike updatePanel().
   */
  void updateDisplayPanel();

  /**
   * renders the view, if any.
   */
  void renderActiveView();

  /**
   * Called when a property on the current proxy changes.
   */
  void sourcePropertyChanged(bool change_finished = true);
  void sourcePropertyChangeAvailable() { this->sourcePropertyChanged(false); }

  /**
   * Updates the state of all the buttons, apply/reset/delete.
   */
  void updateButtonState();

  /**
   * Updates enabled state for buttons on panel (other than
   * apply/reset/delete);
   */
  void updateButtonEnableState();

  void copyProperties();
  void pasteProperties();
  void copyDisplay();
  void pasteDisplay();
  void copyView();
  void pasteView();

protected:
  void updatePropertiesPanel(pqProxy* source);
  void updateDisplayPanel(pqDataRepresentation* repr);
  void updateViewPanel(pqView* view);

private:
  class pqInternals;
  friend class pqInternals;

  pqInternals* Internals;
  int PanelMode;

  Q_DISABLE_COPY(pqPropertiesPanel)
};

#endif
