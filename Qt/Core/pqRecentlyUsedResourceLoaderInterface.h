/*=========================================================================

   Program: ParaView
   Module:  pqRecentlyUsedResourceLoaderInterface.h

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
