// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqSMAdaptor_h
#define pqSMAdaptor_h

class vtkSMProperty;
class vtkSMProxy;
class vtkObject;

#include "pqCoreModule.h"
#include "pqSMProxy.h"
#include "vtkVariant.h"
#include <QList>
#include <QPair>
#include <QVariant>

/**
 * Translates server manager events into Qt-compatible slots and signals
 */
class PQCORE_EXPORT pqSMAdaptor
{
protected:
  // class not instantiated
  pqSMAdaptor();
  ~pqSMAdaptor();

public:
  /**
   * enumeration for types of properties this class handles
   */
  enum PropertyType
  {
    UNKNOWN,
    PROXY,
    PROXYLIST,
    PROXYSELECTION,
    SELECTION,
    ENUMERATION,
    SINGLE_ELEMENT,
    MULTIPLE_ELEMENTS,
    FILE_LIST,
    COMPOSITE_TREE,
  };

  enum PropertyValueType
  {
    CHECKED,
    UNCHECKED
  };

  /**
   * Get the type of the property
   */
  static PropertyType getPropertyType(vtkSMProperty* Property);

  /**
   * get the proxy for a property
   * for example, glyph filter accepts a source (proxy) to glyph with
   */
  static pqSMProxy getProxyProperty(vtkSMProperty* Property, PropertyValueType Type = CHECKED);

  /**
   * get the proxy for a property
   * for example, glyph filter accepts a source (proxy) to glyph with
   */
  static void addProxyProperty(vtkSMProperty* Property, pqSMProxy Value);
  static void removeProxyProperty(vtkSMProperty* Property, pqSMProxy Value);
  static void setProxyProperty(vtkSMProperty* Property, pqSMProxy Value);
  static void setUncheckedProxyProperty(vtkSMProperty* Property, pqSMProxy Value);
  static void addInputProperty(vtkSMProperty* Property, pqSMProxy Value, int opport);
  static void setInputProperty(vtkSMProperty* Property, pqSMProxy Value, int opport);

  /**
   * get the list of proxies for a property
   * for example, append filter accepts a list of proxies
   */
  static QList<QVariant> getProxyListProperty(vtkSMProperty* Property);
  /**
   * get the list of proxies for a property
   * for example, append filter accepts a list of proxies
   */
  static void setProxyListProperty(vtkSMProperty* Property, QList<QVariant> Value);

  /**
   * get the list of possible proxies for a property
   */
  static QList<pqSMProxy> getProxyPropertyDomain(vtkSMProperty* Property);

  /**
   * get the pairs of selections for a selection property
   */
  static QList<QList<QVariant>> getSelectionProperty(
    vtkSMProperty* Property, PropertyValueType Type = CHECKED);
  /**
   * get the pairs of selections for a selection property
   */
  static QList<QVariant> getSelectionProperty(
    vtkSMProperty* Property, unsigned int Index, PropertyValueType Type = CHECKED);
  /**
   * set the pairs of selections for a selection property
   */
  static void setSelectionProperty(
    vtkSMProperty* Property, QList<QList<QVariant>> Value, PropertyValueType Type = CHECKED);

  /**
   * used to set the status of an array, for example. note that this method
   * can only be used for properties with vtkSMArraySelectionDomain or
   * vtkSMStringListRangeDomain.
   */
  static void setSelectionProperty(
    vtkSMProperty* Property, QList<QVariant> Value, PropertyValueType Type = CHECKED);

  /**
   * get the possible names for the selection property
   */
  static QList<QVariant> getSelectionPropertyDomain(vtkSMProperty* Property);

  /**
   * get the list of strings for the given property based on domain and current values.
   */
  static QList<QVariant> getStringListProperty(
    vtkSMProperty* Property, PropertyValueType Type = CHECKED);

  /**
   * get the enumeration for a property
   */
  static QVariant getEnumerationProperty(vtkSMProperty* Property, PropertyValueType Type = CHECKED);
  /**
   * set the enumeration for a property
   */
  static void setEnumerationProperty(
    vtkSMProperty* Property, QVariant Value, PropertyValueType Type = CHECKED);
  /**
   * get the possible enumerations (string) for a property
   */
  static QList<QVariant> getEnumerationPropertyDomain(vtkSMProperty* Property);

  /**
   * get the single element of a property (integer, string, real, etc..)
   */
  static QVariant getElementProperty(vtkSMProperty* Property, PropertyValueType Type = CHECKED);
  /**
   * set the single element of a property (integer, string, real, etc..)
   */
  static void setElementProperty(
    vtkSMProperty* Property, QVariant Value, PropertyValueType Type = CHECKED);
  /**
   * get the range of possible values to set the single element of a property
   */
  static QList<QVariant> getElementPropertyDomain(vtkSMProperty* Property);

  /**
   * get the multiple elements of a property (integer, string, real, etc..)
   */
  static QList<QVariant> getMultipleElementProperty(
    vtkSMProperty* Property, PropertyValueType Type = CHECKED);
  /**
   * set the multiple elements of a property (integer, string, real, etc..)
   */
  static void setMultipleElementProperty(
    vtkSMProperty* Property, QList<QVariant> Value, PropertyValueType Type = CHECKED);
  /**
   * get the ranges of possible values to set the multiple elements of a
   * property
   */
  static QList<QList<QVariant>> getMultipleElementPropertyDomain(vtkSMProperty* Property);

  /**
   * get one of the multiple elements of a property (integer, string, real,
   * etc..)
   */
  static QVariant getMultipleElementProperty(
    vtkSMProperty* Property, unsigned int Index, PropertyValueType Type = CHECKED);
  /**
   * set one of the multiple elements of a property (integer, string, real,
   * etc..)
   */
  static void setMultipleElementProperty(
    vtkSMProperty* Property, unsigned int Index, QVariant Value, PropertyValueType Type = CHECKED);

  /**
   * get one of the ranges of possible values to set the multiple elements of a
   * property
   */
  static QList<QVariant> getMultipleElementPropertyDomain(
    vtkSMProperty* Property, unsigned int Index);

  /**
   * get the single element of a property (integer, string, real, etc..)
   */
  static QStringList getFileListProperty(vtkSMProperty* Property, PropertyValueType Type = CHECKED);
  /**
   * set the single element of a property (integer, string, real, etc..)
   */
  static void setFileListProperty(
    vtkSMProperty* Property, QStringList Value, PropertyValueType Type = CHECKED);

  /**
   * Returns a list of domains types for the property. eg. if a property has
   * vtkSMBoundsDomain and vtkSMArrayListDomain then this method will returns
   * ["vtkSMBoundsDomain", "vtkSMArrayListDomain"].
   */
  static QList<QString> getDomainTypes(vtkSMProperty* property);

  /**
   * Clears any unchecked values on the property.
   */
  static void clearUncheckedProperties(vtkSMProperty* property);

  /**
   * Converts a vtkVariant into a QVariant.
   */
  static QVariant convertToQVariant(const vtkVariant& variant);
};

#endif // !pqSMAdaptor_h
