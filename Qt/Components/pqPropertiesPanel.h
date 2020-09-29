/*=========================================================================

   Program: ParaView
   Module:  pqPropertiesPanel.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef pqPropertiesPanel_h
#define pqPropertiesPanel_h

#include "pqComponentsModule.h"
#include "vtkLegacy.h" // for legacy macros
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
  pqPropertiesPanel(QWidget* parent = 0);
  ~pqPropertiesPanel() override;

  /**
  * Enable/disable auto-apply.
  */
  static void setAutoApply(bool enabled);

  /**
  * Returns \c true if auto-apply is enabled.
  */
  static bool autoApply();

  /**
  * Sets the delay for auto-apply to \p delay (in msec).
  */
  static void setAutoApplyDelay(int delay);

  /**
  * Returns the delay for the auto-apply (in msec).
  */
  static int autoApplyDelay();

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
   * Update the panel to show the widgets for the given pair.
   * @deprecated in ParaView 5.9. Use `setRepresentation`, `setView` and
   * `setPipelineProxy` instead.
   */
  VTK_LEGACY(void updatePanel(pqOutputPort* port));

  /**
   * Returns true if there are changes to be applied.
   */
  bool canApply();

  /**
   * Returns true if there are changes to be reset.
   */
  bool canReset();

  /**
   * This has been replaced by `setPipelineProxy` to add support for other types
   * of pqProxy subclasses such as pqExtractor.
   *
   * @deprecated in ParaView 5.9. Use `setPipelineProxy` instead.
   */
  VTK_LEGACY(void setOutputPort(pqOutputPort*));

public Q_SLOTS:
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
  * Set the view currently managed by the
  * panel, should be called automatically
  * when the active view changes.
  */
  void setView(pqView*);

  /**
   * Set the `pqProxy` to show properties for under the "Properties" section.
   * Typically, this is a pqPipelineSource (or subclass), pqOutputPort, or
   * a pqExtractor.
   */
  void setPipelineProxy(pqProxy*);

  /**
  * Set the representation currently managed by the
  * panel, should be called automatically
  * when the active representation changes.
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
  * This is called when the user clicks the "Delete" button on the
  * properties panel. This triggers the deleteRequested() signal with proper
  * arguments.
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

  /**
  * called when vtkPVGeneralSettings instance is modified. We update the
  * auto-apply status.
  */
  void generalSettingsChanged();

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
  static bool AutoApply;
  static int AutoApplyDelay;

  class pqInternals;
  friend class pqInternals;

  pqInternals* Internals;
  int PanelMode;

  Q_DISABLE_COPY(pqPropertiesPanel)
};

#endif
