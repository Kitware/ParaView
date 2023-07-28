// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqInterfaceTracker_h
#define pqInterfaceTracker_h

#include "pqCoreModule.h"
#include <QObject>

class vtkObject;

/**
 * pqInterfaceTracker is used by ParaView components to locate
 * interface-implementations. These implementations can be either those loaded
 * from plugins or registered explicitly using addInterface/removeInterface API.
 * In previous versions of ParaView, this role was performed by the
 * pqPluginManager class itself.
 */
class PQCORE_EXPORT pqInterfaceTracker : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqInterfaceTracker(QObject* parent = nullptr);
  ~pqInterfaceTracker() override;

  /**
   * Return all interfaces that have been loaded/registered.
   */
  QObjectList interfaces() const { return this->Interfaces + this->RegisteredInterfaces; }

  /**
   * Returns all interfaces that have been loaded/registered that are of the
   * requested type.
   */
  template <class T>
  QList<T> interfaces() const
  {
    QList<T> list;
    QObjectList objList = this->interfaces();
    Q_FOREACH (QObject* object, objList)
    {
      if (object && qobject_cast<T>(object))
      {
        list.push_back(qobject_cast<T>(object));
      }
    }
    return list;
  }

  /**
   * add an extra interface.
   * these interfaces are appended to the ones loaded from plugins
   */
  void addInterface(QObject* iface);

  /**
   * remove an extra interface
   */
  void removeInterface(QObject* iface);

  /**
   * initializes the tracker using existing plugins.
   */
  void initialize();
Q_SIGNALS:
  /**
   * fired every time an interface is registered either from a plugin on
   * manually.
   */
  void interfaceRegistered(QObject* iface);

protected:
  /**
   * Callback when a plugin is loaded. We locate and load any interafaces
   * defined in the plugin.
   */
  void onPluginLoaded(vtkObject*, unsigned long, void* calldata);

  QObjectList Interfaces;
  QObjectList RegisteredInterfaces;
  unsigned long ObserverID;

private:
  Q_DISABLE_COPY(pqInterfaceTracker)
};

#endif
