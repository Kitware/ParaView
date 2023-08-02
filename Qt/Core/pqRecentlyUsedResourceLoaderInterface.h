// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqRecentlyUsedResourceLoaderInterface_h
#define pqRecentlyUsedResourceLoaderInterface_h

#include <QObject>

#include "pqCoreModule.h" // needed for export macro
#include <QIcon>          // needed for QIcon
#include <QtPlugin>       // needed for Q_DECLARE_INTERFACE

/**
 * @class pqRecentlyUsedResourceLoaderInterface
 * @brief abstract interface used to load recently used resources.
 *
 * pqRecentlyUsedResourceLoaderInterface provides the interface for code that
 * handles loading of recently used resources typically maintained in
 * `pqRecentlyUsedResourcesList`. `pqRecentFilesMenu` uses implementations of
 * this interface to determine the action to trigger when user clicks on a
 * specific resource in the menu.
 */

class pqServer;
class pqServerResource;

class PQCORE_EXPORT pqRecentlyUsedResourceLoaderInterface
{
public:
  virtual ~pqRecentlyUsedResourceLoaderInterface();

  /**
   * Return true of the interface supports loading the given resource.
   */
  virtual bool canLoad(const pqServerResource& resource) = 0;

  /**
   * Loads the resource and returns the true if successfully loaded, otherwise
   * false. If returned false, the resource is treated as defunct and may be removed.
   * from the recent list.
   */
  virtual bool load(const pqServerResource& resource, pqServer* server) = 0;

  /**
   * Return an icon, if any, to use for the resource. Default implementation
   * simply returns an empty QIcon.
   */
  virtual QIcon icon(const pqServerResource& resource);

  /**
   * Return label to use for the resource.
   */
  virtual QString label(const pqServerResource& resource) = 0;

protected:
  pqRecentlyUsedResourceLoaderInterface();

private:
  Q_DISABLE_COPY(pqRecentlyUsedResourceLoaderInterface)
};

Q_DECLARE_INTERFACE(
  pqRecentlyUsedResourceLoaderInterface, "com.kitware/paraview/recentlyusedresourceloader")
#endif
