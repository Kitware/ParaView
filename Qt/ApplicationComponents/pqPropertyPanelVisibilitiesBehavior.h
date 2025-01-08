// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPropertyPanelVisibilitiesBehavior_h
#define pqPropertyPanelVisibilitiesBehavior_h

#include <QObject>

#include <QJsonDocument>

class vtkSMProxy;

/**
 * @ingroup Behaviors
 * pqPropertyVisilityBehavior allows to override properties visibility
 * for pqProxyWidgets, similarly to the `panel_visibility` XML attribute.
 * @see vtkSMProperty panel_visibility attribute
 *
 * The configuration is read from a PropertyPanelVisibilities.json file, that should
 * be placed under a standard ParaView configuration directory.
 * @see pqCoreUtilities::findParaViewPaths for more about possible locations.
 *
 * This behavior update already registered proxies and observes proxy registration.
 * through pqServerManagerObserver.
 *
 * @note: pqProxyWidgets created before the pqPropertVisibilityBehavior instanciation
 * will not be updated, so it may be useful to instanciate this behavior early.
 *
 * @note: Expected configuration is a JSON formatted as follow:
 * {
 *   group: {
 *     proxy: {
 *       property: "default",
 *       property2: "advanced"
 *     },
 *     proxy2: {
 *       property3: "never"
 *     } ...
 *   }
 * }
 */
class pqPropertyPanelVisibilitiesBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPropertyPanelVisibilitiesBehavior(QObject* parent);
  ~pqPropertyPanelVisibilitiesBehavior() override;

protected:
  /**
   * Get configuration file from ParaView usual directories.
   * @see pqCoreUtilites::findParaViewPaths
   */
  virtual QString getConfigFilePath();

private:
  Q_DISABLE_COPY(pqPropertyPanelVisibilitiesBehavior);

  /**
   * Load configuration file containing overrides.
   */
  void loadConfiguration();

  /**
   * Parse given file into an internally stored JSON object.
   */
  bool parseSettingFile(const QString& filePath);

  /**
   * Update properties visibility for proxies registered in current session.
   */
  void updateExistingProxies();

  /**
   * Apply custom visibility for given proxy properties.
   */
  void overrideVisibility(vtkSMProxy* proxy);

  QJsonDocument Config;
};

#endif
