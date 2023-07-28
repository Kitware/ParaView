// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqPropertyManager_h
#define pqPropertyManager_h

#include "pqCoreModule.h"
#include <QObject>
#include <QPointer>
#include <QVariant>

class vtkSMProxy;
class vtkSMProperty;
class pqPropertyLinks;

/**
 * Manages links between Qt properties and unchecked proxy properties
 * This is useful if more than one QWidget exposes a single proxy property
 * In which case the server manager will not keep the widgets synchronized
 * Also provides a mechanims for accepting or rejecting changes for unchecked
 * properties
 */
class PQCORE_EXPORT pqPropertyManager : public QObject
{
  Q_OBJECT

public:
  /**
   * constructor
   */
  pqPropertyManager(QObject* p = nullptr);
  /**
   * destructor
   */
  ~pqPropertyManager() override;

  /**
   * register a QObject property to link with the server manager
   */
  void registerLink(QObject* qObject, const char* qProperty, const char* signal, vtkSMProxy* Proxy,
    vtkSMProperty* Property, int Index = -1);

  /**
   * unregister a QObject property to link with the server manager
   */
  void unregisterLink(QObject* qObject, const char* qProperty, const char* signal,
    vtkSMProxy* Proxy, vtkSMProperty* Property, int Index = -1);

  /**
   * returns whether there are modified properties to send to
   * the server manager
   */
  bool isModified() const;

  // Call this method to un-links all property links
  // maintained by this object.
  void removeAllLinks();

Q_SIGNALS:
  /**
   * Signal emitted when there are possible properties to send down to
   * the server manager
   */
  void modified();

  /**
   * Signal emitted when the user has accepted changes
   */
  void aboutToAccept();
  void accepted();
  /**
   * Signal emitted when the user has rejected changes
   */
  void rejected();

public Q_SLOTS:
  /**
   * accept property changes by pushing them all down to the server manager
   */
  void accept();
  /**
   * reject property changes and revert all QObject properties
   */
  void reject();
  /**
   * Called whenever a property changes from the GUI side
   */
  void propertyChanged();

protected:
  pqPropertyLinks* Links;
  bool Modified;
};
#endif // !pqPropertyManager_h
