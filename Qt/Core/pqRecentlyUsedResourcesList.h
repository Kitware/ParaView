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
#ifndef pqRecentlyUsedResourcesList_h
#define pqRecentlyUsedResourcesList_h

#include "pqServerResource.h"
#include <QList>
#include <QObject>

/**
 * @class pqRecentlyUsedResourcesList
 * @brief manages recently used resources
 *
 * pqRecentlyUsedResourcesList manages recently used resources, such as data
 * files, state files, etc. When user performs an action (e.g. loading of data,
 * loading of state file) that should be saved to the recently used resource
 * list, simply add it using `pqRecentlyUsedResourcesList::add()`. One stats
 * with the `pqServerResource` obtained from the server on which the action was
 * performed and the can add meta-data to it, as needed e.g.
 *
 * @code{.cpp}
 *    pqServer* server = pqActiveObjects::instance().activeServer();
 *    pqServerResource resource = server->getResource();
 *    resource.setPath(...);
 *    resource.addData("foo1", "bar1");
 *    resource.addData("foo2", "bar2");
 *
 *    pqApplicationCore* core = pqApplicationCore::instance();
 *    core->recentlyUsedResources().add(resource);
 * @endcode
 *
 * Now, applications can use `pqRecentFilesMenu` (or something similar) to show
 * these resources in some menu or other UI element.
 *
 * pqRecentlyUsedResourcesList itself doesn't handle reloading the resource from
 * the list. That is left to the application. `pqRecentFilesMenu`, for example,
 * uses implementations of `pqRecentlyUsedResourceLoaderInterface` registered
 * with the `pqInterfaceTracker` to attempt to load the resource.
 *
 * @sa pqServerResource, pqRecentlyUsedResourcesList,
 * pqRecentlyUsedResourceLoaderInterface
 *
 */

class pqSettings;

class PQCORE_EXPORT pqRecentlyUsedResourcesList : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqRecentlyUsedResourcesList(QObject* parent = 0);
  ~pqRecentlyUsedResourcesList() override;

  /**
  * convenience typedef.
  */
  typedef QList<pqServerResource> ListT;

  /**
  * Add a resource to the collection. Moves the resource to the beginning of
  * the list.
  */
  void add(const pqServerResource& resource);

  /**
  * Returns the contents of the collection ordered from most-recently-used to
  * least-recently-used.
  */
  const QList<pqServerResource>& list() const { return this->ResourceList; }

  /**
  * Load the collection (from local user preferences)
  */
  void load(pqSettings&);

  /**
  * Save the collection (to local user preferences)
  */
  void save(pqSettings&) const;

Q_SIGNALS:
  /**
  * Signal emitted whenever the collection is changed i.e. new  items are added
  * or removed.
  */
  void changed();

private:
  QList<pqServerResource> ResourceList;

  Q_DISABLE_COPY(pqRecentlyUsedResourcesList)
};

#endif
