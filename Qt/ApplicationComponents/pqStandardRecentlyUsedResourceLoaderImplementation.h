/*=========================================================================

   Program: ParaView
   Module:  pqStandardRecentlyUsedResourceLoaderImplementation.h

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
#ifndef pqStandardRecentlyUsedResourceLoaderImplementation_h
#define pqStandardRecentlyUsedResourceLoaderImplementation_h

#include "pqRecentlyUsedResourceLoaderInterface.h"

#include "pqApplicationComponentsModule.h" // needed for export macros
#include <QObject>                         // needed for QObject
#include <QStringList>                     // needed for QStringList

/**
 * @class pqStandardRecentlyUsedResourceLoaderImplementation
 * @brief support loading states, and data files when loaded from pqRecentFilesMenu.
 *
 * pqStandardRecentlyUsedResourceLoaderImplementation is an implementation of
 * the pqRecentlyUsedResourceLoaderInterface that support loading resources
 * known to ParaView e.g. data files and state files. `pqParaViewBehaviors`
 * instantiates this unless
 * `pqParaViewBehaviors::enableStandardRecentlyUsedResourceLoader` is false.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqStandardRecentlyUsedResourceLoaderImplementation
  : public QObject,
    public pqRecentlyUsedResourceLoaderInterface
{
  Q_OBJECT
  Q_INTERFACES(pqRecentlyUsedResourceLoaderInterface)

  typedef QObject Superclass;

public:
  pqStandardRecentlyUsedResourceLoaderImplementation(QObject* parent = 0);
  ~pqStandardRecentlyUsedResourceLoaderImplementation() override;

  bool canLoad(const pqServerResource& resource) override;
  bool load(const pqServerResource& resource, pqServer* server) override;
  QIcon icon(const pqServerResource& resource) override;
  QString label(const pqServerResource& resource) override;

  /**
   * Add data files(s) to the recently used resources list.
   */
  static bool addDataFilesToRecentResources(
    pqServer* server, const QStringList& files, const QString& smgroup, const QString& smname);

  /**
   * Add state file to the recently used resources list.
   */
  static bool addStateFileToRecentResources(pqServer* server, const QString& file);

private:
  Q_DISABLE_COPY(pqStandardRecentlyUsedResourceLoaderImplementation)

  bool loadState(const pqServerResource& resource, pqServer* server);
  bool loadData(const pqServerResource& resource, pqServer* server);
};

#endif
