// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPropertyWidgetInterface_h
#define pqPropertyWidgetInterface_h

#include <QtPlugin>

#include "pqComponentsModule.h"

class pqPropertyWidget;
class pqPropertyWidgetDecorator;
class vtkPVXMLElement;
class vtkSMProperty;
class vtkSMPropertyGroup;
class vtkSMProxy;

/**
 * pqPropertyWidgetInterface is part of the ParaView Plugin infrastructure that
 * enables support for plugins to add new pqPropertyWidget and
 * pqPropertyWidgetDecorator types and make them available within the
 * application.
 */
class PQCOMPONENTS_EXPORT pqPropertyWidgetInterface
{
public:
  // Destroys the property widget interface object.
  virtual ~pqPropertyWidgetInterface();

  /**
   * Given a proxy and its property, create a widget for the same, of possible.
   * For unsupported/unknown proxies/properties, implementations should simply
   * return nullptr without raising any errors (or messages).
   */
  virtual pqPropertyWidget* createWidgetForProperty(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parentWidget);

  /**
   * Given a proxy and its property group, create a widget for the same, of possible.
   * For unsupported/unknown proxies/property-groups, implementations should simply
   * return nullptr without raising any errors (or messages).
   */
  virtual pqPropertyWidget* createWidgetForPropertyGroup(
    vtkSMProxy* proxy, vtkSMPropertyGroup* group, QWidget* parentWidget);

  /**
   * Given the type of the decorator and the pqPropertyWidget that needs to be
   * decorated, create the pqPropertyWidgetDecorator instance, if possible.
   * For unsupported/unknown, implementations should simply return nullptr without
   * raising any errors (or messages).
   */
  virtual pqPropertyWidgetDecorator* createWidgetDecorator(
    const QString& type, vtkPVXMLElement* config, pqPropertyWidget* widget);

  /**
   * Create all default decorators for a specific widget.
   */
  virtual void createDefaultWidgetDecorators(pqPropertyWidget* widget);
};

Q_DECLARE_INTERFACE(pqPropertyWidgetInterface, "com.kitware/paraview/propertywidget")

#endif // pqPropertyWidgetInterface_h
