// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqStandardRecentlyUsedResourceLoaderImplementation_h
#define pqStandardRecentlyUsedResourceLoaderImplementation_h

#include "pqRecentlyUsedResourceLoaderInterface.h"

#include "pqApplicationComponentsModule.h" // needed for export macros

#include "vtkType.h" // needed for vtkTypeUInt32

#include <QObject>     // needed for QObject
#include <QStringList> // needed for QStringList

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
  : public QObject
  , public pqRecentlyUsedResourceLoaderInterface
{
  Q_OBJECT
  Q_INTERFACES(pqRecentlyUsedResourceLoaderInterface)

  typedef QObject Superclass;

public:
  pqStandardRecentlyUsedResourceLoaderImplementation(QObject* parent = nullptr);
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
  static bool addStateFileToRecentResources(
    pqServer* server, const QString& file, vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/);

private:
  Q_DISABLE_COPY(pqStandardRecentlyUsedResourceLoaderImplementation)

  bool loadState(const pqServerResource& resource, pqServer* server,
    vtkTypeUInt32 location = 0x10 /*vtkPVSession::CLIENT*/);
  bool loadData(const pqServerResource& resource, pqServer* server);
};

#endif
