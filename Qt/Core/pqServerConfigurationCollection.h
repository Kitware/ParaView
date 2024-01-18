// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqServerConfigurationCollection_h
#define pqServerConfigurationCollection_h

#include "pqCoreModule.h"
#include <QList>
#include <QMap>
#include <QObject>

class vtkPVXMLElement;
class pqServerResource;
class pqServerConfiguration;

/**
 * pqServerConfigurationCollection maintains a serializable collection of
 * server-configurations defined in pqServerConfiguration instances.
 *
 * During construction, this class attempts to read server-configurations from
 * following locations:
 * \li \c QApplication\::applicationDirPath()/default_servers.pvsc
 * \li \c ${COMMON_APPDATA}/QApplication\::organizationName()/servers.pvsc
 * \li \c /usr/share/QApplication\::organizationName()/servers.pvsc
 * \li \c ${QSettings INI Path}/servers.pvsc -- User-specific servers.
 *
 * The location marked as "User-specific servers" is the location where any
 * server-configurations created/imported by user are saved. User can only
 * modify the server-configurations loaded from this file. All others are
 * treated as read-only configurations.
 */
class PQCORE_EXPORT pqServerConfigurationCollection : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqServerConfigurationCollection(QObject* parent = nullptr);
  ~pqServerConfigurationCollection() override;

  /**
   * load a pvsc txt. If mutable_configs==true, then the server-configurations
   * loaded from this file can be edited by the user and get saved in
   * "User-specific servers" configuration file at destruction.
   */
  bool loadContents(const QString& contents, bool mutable_configs);

  /**
   * save the currently loaded configurations to a file. If only_mutable==true,
   * only the configurations that were marked mutable are saved, otherwise all
   * configurations are saved.
   */
  QString saveContents(bool only_mutable) const;

  /**
   * load/save configurations from/to file.
   */
  bool load(const QString& filename, bool mutable_configs);
  bool save(const QString& filename, bool only_mutable);
  bool saveNow();

  /**
   * Add a server configuration.
   */
  void addConfiguration(vtkPVXMLElement* configuration, bool mutable_config = true);
  void addConfiguration(const pqServerConfiguration&);

  /**
   * remove a configuration given the name.
   */
  void removeConfiguration(const QString&);

  /**
   * returns the set of server-configurations.
   */
  QList<pqServerConfiguration> configurations() const;

  /**
   * returns the configurations matching the selector's host.
   */
  QList<pqServerConfiguration> configurations(const pqServerResource& selector) const;

  /**
   * Returns a configuration with the given name. Returns nullptr when none is
   * found.
   */
  const pqServerConfiguration* configuration(const char* configuration_name) const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Remove all user configurations. This deletes the user servers.pvsc file.
   */
  void removeUserConfigurations();

Q_SIGNALS:
  /**
   * fired when the collection is modified.
   */
  void changed();

protected:
  QMap<QString, pqServerConfiguration> Configurations;

private:
  Q_DISABLE_COPY(pqServerConfigurationCollection)
};

#endif
