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
  pqInterfaceTracker(QObject* parent = 0);
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
    foreach (QObject* object, objList)
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

protected:
  QObjectList Interfaces;
  QObjectList RegisteredInterfaces;
  unsigned long ObserverID;

private:
  Q_DISABLE_COPY(pqInterfaceTracker)
};

#endif
