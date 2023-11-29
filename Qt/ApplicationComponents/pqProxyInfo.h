// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProxyInfo_h
#define pqProxyInfo_h

#include "pqApplicationComponentsModule.h"

#include <QObject>

#include <QMap>     // for QMap
#include <QPointer> // for QPointer
#include <QString>  // for QString

class pqProxyCategory;
class vtkPVXMLElement;

/**
 * Proxy meta data structure for the User Interface.
 */
class pqProxyInfo : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqProxyInfo(pqProxyCategory* parent, const QString& name, const QString& group,
    const QString& label, const QString& icon = "",
    const QStringList& omitFromToolbar = QStringList(), bool hideFromMenu = false);

  /**
   * create a pqProxyInfo with same attributes as other.
   */
  pqProxyInfo(pqProxyCategory* parent, pqProxyInfo* other);
  pqProxyInfo(pqProxyCategory* parent, vtkPVXMLElement* xmlElement);
  pqProxyInfo() = default;

  /**
   * Set label.
   */
  void setLabel(const QString& label);

  /**
   * Set HideFromMenu.
   */
  void setHideFromMenu(bool hidden);

  /**
   * Set Icon path.
   */
  void setIcon(const QString& iconPath);

  /**
   * Merge pqProxyInfo meta data into self.
   * Append OmitFromToolbar values.
   * For other properties, keep those from self if non empty, else copy other.
   */
  void merge(pqProxyInfo* other);

  /**
   * Merge otherProxies into proxies and return a new map
   */
  static QMap<QString, QPointer<pqProxyInfo>> mergeProxies(
    const QMap<QString, QPointer<pqProxyInfo>>& proxies,
    const QMap<QString, QPointer<pqProxyInfo>>& otherProxies);

  /**
   * Create the xml element and add it under given root.
   */
  void convertToXML(vtkPVXMLElement* root);

  ///@{
  /**
   * Return member.
   */
  QString name() { return this->Name; }
  QString group() { return this->Group; }
  /// return the translated label, or create one from name if empty.
  QString label();
  QString icon() { return this->Icon; }
  QStringList omitFromToolbar() { return this->OmitFromToolbar; }
  bool hideFromMenu();
  ///@}

private:
  /// XML Name
  QString Name;
  /// XML Group
  QString Group;
  /// Display label
  QString Label;
  /// Icon name (opt)
  QString Icon;
  /// List of category names whose toolbars should not contain this proxy.
  QStringList OmitFromToolbar;
  /// Flag to hide proxy in menus
  bool HideFromMenu = false;
};

/// map a proxy name to its pqProxyInfo
using pqProxyInfoMap = QMap<QString, pqProxyInfo*>;

#endif
