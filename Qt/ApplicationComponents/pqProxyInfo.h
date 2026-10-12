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
 * Meta data structure for the User Interface, describing an item of a
 * pqProxyCategory. Items are mostly proxies i.e. sources or filters, but can
 * also be Python scripts and separators. In XML, they are respectively
 * described using the `Proxy`, `Script` and `Separator` elements:
 *
 * ```xml
 * <Proxy group="filters" name="Clip" icon=":/pqWidgets/Icons/pqClip.svg" />
 * <Script name="Hello" label="Say Hello" tooltip="Prints a greeting.">
 *   print("Hello!")
 * </Script>
 * <Separator />
 * ```
 *
 * A `Script` can also provide its code using a `filename` attribute instead.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqProxyInfo : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqProxyInfo(pqProxyCategory* parent, const QString& name, const QString& group,
    const QString& label, const QString& icon = "",
    const QStringList& omitFromToolbar = QStringList(), bool hideFromMenu = false);

  /**
   * Type of item.
   */
  enum class ItemType
  {
    Proxy,
    Script,
    Separator
  };

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
   * Set the name. Must be done before adding it to a category.
   */
  void setName(const QString& name) { this->Name = name; }

  /**
   * Set HideFromMenu.
   */
  void setHideFromMenu(bool hidden);

  /**
   * Set Icon path.
   */
  void setIcon(const QString& iconPath);

  /**
   * Set whether the item is omitted from the toolbar of the given category.
   */
  void setOmitFromToolbar(const QString& categoryName, bool omit);

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
  ItemType type() const { return this->Type; }
  bool isProxy() const { return this->Type == ItemType::Proxy; }
  bool isScript() const { return this->Type == ItemType::Script; }
  bool isSeparator() const { return this->Type == ItemType::Separator; }
  /// for scripts: the Python code, if provided inline.
  QString script() const { return this->Script; }
  /// for scripts: the file containing the Python code, if provided.
  QString fileName() const { return this->FileName; }
  /// for scripts: the tooltip.
  QString toolTip() const { return this->ToolTip; }
  ///@}

  /**
   * Returns true if the item has the required information, which depends on
   * its type: a name and group for proxies, a name and either code or a file
   * name for scripts.
   */
  bool isValid() const;

  /**
   * Returns true if the attributes that can be customized by users, i.e. icon
   * and visibility in menus and toolbars, are the same for both items. For the
   * toolbar visibility, only the parent categories are considered.
   */
  bool hasSameCustomizableAttributes(pqProxyInfo* other);

private:
  ItemType Type = ItemType::Proxy;
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
  /// For scripts: inline Python code, file containing the code and tooltip.
  QString Script;
  QString FileName;
  QString ToolTip;
};

/// map a proxy name to its pqProxyInfo
using pqProxyInfoMap = QMap<QString, pqProxyInfo*>;

#endif
