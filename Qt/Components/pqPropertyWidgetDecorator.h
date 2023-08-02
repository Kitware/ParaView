// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPropertyWidgetDecorator_h
#define pqPropertyWidgetDecorator_h

#include "pqPropertyWidget.h"
#include "vtkSmartPointer.h" // needed for vtkSmartPointer

class vtkPVXMLElement;

/**
 * pqPropertyWidgetDecorator provides a mechanism to decorate pqPropertyWidget
 * instances to add logic to the widget to add additional control logic.
 * Subclasses can be used to logic to control when the widget is
 * enabled/disabled, hidden/visible, etc. based on values of other properties
 * of UI elements.
 */
class PQCOMPONENTS_EXPORT pqPropertyWidgetDecorator : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
   * Constructor.
   *
   * @param xml The XML element from the `<Hints/>` section for the proxy/property that
   * resulted in the creation of the decorator. Decorators can be provided
   * configuration parameters from the XML.
   * @param parent Parent widget
   */
  pqPropertyWidgetDecorator(vtkPVXMLElement* xml, pqPropertyWidget* parent);
  ~pqPropertyWidgetDecorator() override;

  /**
   * Returns the pqPropertyWidget parent.
   */
  pqPropertyWidget* parentWidget() const;

  /**
   * Override this method to override the visibility of the widget in the
   * panel. This is called after the generic tests for advanced and text
   * filtering are passed. Since there can be multiple decorators, the first
   * decorator that returns 'false' wins. Default implementation returns true.
   * Thus subclasses typically override this method only to force the widget
   * invisible given the current state.
   */
  virtual bool canShowWidget(bool show_advanced) const
  {
    Q_UNUSED(show_advanced);
    return true;
  }

  /**
   * Override this method to override the enable state of the widget in the
   * panel. This is called after the generic tests for advanced and text
   * filtering are passed. Since there can be multiple decorators, the first
   * decorator that returns 'false' wins. Default implementation returns true.
   * Thus subclasses typically override this method only to force the widget
   * disabled given the current state.
   */
  virtual bool enableWidget() const { return true; }

  /**
   * Creates a new decorator, given the xml config and the parent
   * pqPropertyWidget for the decorator.
   */
  static pqPropertyWidgetDecorator* create(vtkPVXMLElement* xml, pqPropertyWidget* parent);

Q_SIGNALS:
  /**
   * This signal is fired whenever the decorator has determined that the panel
   * *may* need a refresh since the state of the system has changed which would
   * deem changes in the widget visibility or enable state.
   */
  void visibilityChanged();
  void enableStateChanged();

protected:
  vtkPVXMLElement* xml() const;

private:
  Q_DISABLE_COPY(pqPropertyWidgetDecorator)

  vtkSmartPointer<vtkPVXMLElement> XML;
};

#endif
