/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#ifndef pqProxyWidget_h
#define pqProxyWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

class pqPropertyWidget;
class pqView;
class vtkSMProperty;
class vtkSMProxy;

/**
* pqProxyWidget represents a panel for a vtkSMProxy. pqProxyWidget creates
* widgets for each of the properties (or proxy groups) of the proxy respecting
* any registered pqPropertyWidgetInterface instances to create custom widgets.
* pqProxyWidget is used by pqPropertiesPanel to create panels for the
* source/filter and the display/representation sections of the panel.
*
* pqProxyWidget doesn't show any widgets in the panel by default (after
* constructor). Use filterWidgets() or updatePanel() to show widgets matching
* criteria.
*
* Note: This class replaces pqProxyPanel (and subclasses). pqProxyPanel is
* still available (and supported) for backwards compatibility.
*/
class PQCOMPONENTS_EXPORT pqProxyWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqProxyWidget(vtkSMProxy* proxy, QWidget* parent = 0, Qt::WindowFlags flags = 0);
  pqProxyWidget(vtkSMProxy* proxy, const QStringList& properties, QWidget* parent = 0,
    Qt::WindowFlags flags = 0);
  ~pqProxyWidget() override;

  /**
  * Returns the proxy this panel shows.
  */
  vtkSMProxy* proxy() const;

  /**
  * When set to true, whenever the widget changes, the values are immediately
  * pushed to the ServerManager property without having to wait for apply().
  * This is used for panels such as the display panel. Default is false.
  */
  void setApplyChangesImmediately(bool value);
  bool applyChangesImmediately() const { return this->ApplyChangesImmediately; }

  /**
  * When this is true, the panel uses a descriptive layout where the
  * documentation for properties is used instead of their labels. pqProxyWidget
  * automatically adopts this style of layout if `<UseDocumentationForLabels />`
  * hint is present in the proxy.
  */
  bool useDocumentationForLabels() const { return this->UseDocumentationForLabels; }

  /**
  * Returns a new widget that has the label and a h-line separator. This is
  * used on the pqProxyWidget to separate groups. Other widgets can use it for
  * the same purpose, as needed.
  */
  static QWidget* newGroupLabelWidget(const QString& label, QWidget* parentWidget);

  /**
  * Returns true of the proxy provided has XML hints indicating that labels
  * should use documentation instead of the XML label for the widgets in the
  * UI.
  */
  static bool useDocumentationForLabels(vtkSMProxy* proxy);

  enum DocumentationType
  {
    NONE,
    USE_DESCRIPTION,
    USE_SHORT_HELP,
    USE_LONG_HELP
  };

  /**
  * Returns formatted (HTML or plainText) documentation for the property.
  * \c type cannot be NONE.
  */
  static QString documentationText(
    vtkSMProperty* property, DocumentationType type = USE_DESCRIPTION);

  /**
  * Returns formatted (HTML or plainText) documentation for the proxy.
  * \c type cannot be NONE.
  */
  static QString documentationText(vtkSMProxy* property, DocumentationType type = USE_DESCRIPTION);

  /**
  * Returns true if the proxy has XML hints indicating that the panel should
  * show a header label for the documentation. pqProxyWidget uses the
  * `<ShowProxyDocumentationInPanel />` hint for this purpose.
  */
  static DocumentationType showProxyDocumentationInPanel(vtkSMProxy* proxy);

signals:
  /**
  * This signal is fired as soon as the user starts editing in the widget. The
  * editing may not be complete.
  */
  void changeAvailable();

  /**
  * This signal is fired as soon as the user is done with making an atomic
  * change. changeAvailable() is always fired before changeFinished().
  */
  void changeFinished();

  /**
  * Indicates that a restart of the program is required for the setting
  * to take effect.
  */
  void restartRequired();

public slots:
  /**
  * Updates the property widgets shown based on the filterText or
  * show_advanced flag. Calling filterWidgets() without any arguments will
  * result in the panel showing all the non-advanced properties.
  * Returns true, if any widgets were shown.
  */
  bool filterWidgets(bool show_advanced = false, const QString& filterText = QString());

  /**
  * Accepts the property widget changes changes.
  */
  void apply() const;

  /**
  * Cleans the property widget changes and resets the widgets.
  */
  void reset() const;

  /**
  * Set the current view to use to show 3D widgets, if any for the panel.
  */
  void setView(pqView*);

  /**
  * Same as calling filterWidgets() with the arguments specified to the most
  * recent call to filterWidgets().
  */
  void updatePanel();

  /**
  * Restores application default proxy settings.
  * Returns true if any properties were modified.
  */
  virtual bool restoreDefaults();

  /**
  * Saves settings as defaults for proxy
  */
  void saveAsDefaults();

protected:
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;

private slots:
  /**
  * Called when a pqPropertyWidget fires changeFinished() signal.
  * This callback fires changeFinished() signal and handles AutoUpdateVTKObjects.
  */
  void onChangeFinished();

private:
  /**
  * the actual constructor implementation.
  */
  void constructor(
    vtkSMProxy* proxy, const QStringList& properties, QWidget* parent, Qt::WindowFlags flags);

  /**
  * create all widgets
  */
  void createWidgets(const QStringList& properties = QStringList());

  /**
  * create individual property widgets.
  */
  void createPropertyWidgets(const QStringList& properties = QStringList());

  /**
  * create 3D widgets, if any.
  */
  void create3DWidgets();

  /**
  * create a widget for a property.
  */
  pqPropertyWidget* createWidgetForProperty(
    vtkSMProperty* property, vtkSMProxy* proxy, QWidget* parentObj);

private:
  Q_DISABLE_COPY(pqProxyWidget)

  bool ApplyChangesImmediately;
  bool UseDocumentationForLabels;
  class pqInternals;
  pqInternals* Internals;
};

#endif
