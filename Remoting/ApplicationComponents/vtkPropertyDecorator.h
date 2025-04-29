// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkPropertyDecorator_h
#define vtkPropertyDecorator_h

#include "vtkRemotingApplicationComponentsModule.h"

#include "vtkCommand.h"
#include "vtkObject.h"
#include "vtkSMProxy.h"

#include "vtkSmartPointer.h" // needed for vtkSmartPointer
#include "vtkWeakPointer.h"

#include <string>

class vtkPVXMLElement;
class vtkSMProxy;

/**
 * vtkPropertyDecorator hold the logic of pqPropertyDecorator
 * TODO provides a mechanism to decorate pqProperty
 * instances to add logic to the widget to add additional control logic.
 * Subclasses can be used to logic to control when the widget is
 * enabled/disabled, hidden/visible, etc. based on values of other properties
 * of UI elements.
 */
class VTKREMOTINGAPPLICATIONCOMPONENTS_EXPORT vtkPropertyDecorator : public vtkObject
{

public:
  static vtkPropertyDecorator* New();
  vtkTypeMacro(vtkPropertyDecorator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Constructor.
   *
   * @param xml The XML element from the `<Hints/>` section for the proxy/property that
   * resulted in the creation of the decorator. Decorators can be provided
   * configuration parameters from the XML.
   *
   * @param proxy The proxy that owns the property of this decorator
   */
  void virtual Initialize(vtkPVXMLElement* xml, vtkSMProxy* proxy);

  /**
   * Override this method to override the visibility of the widget in the
   * panel. This is called after the generic tests for advanced and text
   * filtering are passed. Since there can be multiple decorators, the first
   * decorator that returns 'false' wins. Default implementation returns true.
   * Thus subclasses typically override this method only to force the widget
   * invisible given the current state.
   */
  virtual bool CanShow(bool show_advanced) const
  {
    (void)show_advanced;
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
  virtual bool Enable() const { return true; }

  /**
   * This event is fired whenever the decorator has determined that the panel
   * *may* need a refresh since the state of the system has changed which would
   * deem changes in the widget visibility or enable state.
   */
  enum
  {
    VisibilityChangedEvent = vtkCommand::UserEvent + 1000,
    EnableStateChangedEvent = vtkCommand::UserEvent + 1001
  };

  /**
   * Creates a new decorator, given the xml config and the proxy containing the property
   * For unsupported/unknown, implementations should simply return nullptr without
   * raising any errors (or messages).
   * Supported types are:
   * \li \c EnableWidgetDecorator : vtkEnableWidgetDecorator
   * \li \c ShowWidgetDecorator : vtkShowWidgetDecorator
   * \li \c InputDataTypeDecorator : vtkInputDataTypeDecorator
   * \li \c GenericDecorator: vtkGenericPropertyWidgetDecorator
   * \li \c OSPRayHidingDecorator: vtkOSPRayHidingDecorator
   * \li \c MultiComponentsDecorator: vtkMultiComponentsDecorator
   * \li \c CompositeDecorator: vtkCompositePropertyWidgetDecorator
   * \li \c SessionTypeDecorator: vtkSessionTypeDecorator
   */
  static vtkSmartPointer<vtkPropertyDecorator> Create(vtkPVXMLElement* xml, vtkSMProxy* proxy);

protected:
  vtkPropertyDecorator();
  ~vtkPropertyDecorator() override;
  vtkPVXMLElement* XML() const;
  vtkSMProxy* Proxy() const;

  void InvokeVisibilityChangedEvent();
  void InvokeEnableStateChangedEvent();

private:
  vtkPropertyDecorator(const vtkPropertyDecorator&) = delete;
  void operator=(const vtkPropertyDecorator&) = delete;

  vtkWeakPointer<vtkPVXMLElement> xml_;
  vtkWeakPointer<vtkSMProxy> proxy_;
};

#endif
