// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProxyCategory_h
#define pqProxyCategory_h

#include "pqApplicationComponentsModule.h"

#include "pqProxyInfo.h" // for pqProxyInfoMap
#include <QMap>          // for QMap
#include <QString>       // for QString

class vtkPVXMLElement;

/**
 * @class pqProxyCategory
 * @brief The pqProxyCategory class reads and writes XML that describes the proxies organisation
 * into categories.
 *
 * @details Categories can be nested and are describe in XML files, like this example:
 * ```
 * <ParaViewFilters>
 *   <Category name="DataModel" menu_label="Data Model" show_in_toolbar="0">
 *     <Category name="Hyper Tree Grid" menu_label="&amp;Hyper Tree Grid">
 *       <Proxy group="filters" name="Clip" />
 *       <Proxy group="filters" name="Cut" />
 *     </Category>
 *     <Category name="Chemistry" menu_label="Chemistry">
 *       <Proxy group="filters" name="AppendMolecule" />
 *       <Proxy group="filters" name="ComputeMoleculeBonds" />
 *       <Proxy group="filters" name="MoleculeToLines" />
 *       <Proxy group="filters" name="PointSetToMolecule" />
 *     </Category>
 *   </Category>
 *   <Category name="Statistics" menu_label="&amp;Statistics">
 *     <Proxy group="filters" name="ContingencyStatistics" />
 *     <Proxy group="filters" name="DescriptiveStatistics" />
 *   </Category>
 * </ParaViewFilters>
 * ```
 *
 * Proxies can have properties, like icon:
 * ```
 * <ParaViewFilters>
 *   <Proxy group="filters" name="Clip" icon=":/pqWidgets/Icons/pqClip.svg" />
 * </ParaViewFilters>
 * ```
 *
 * The xml hints `<ShowInMenu>` can also be used at proxy definition to assign it in a category.
 * This was mainly designed for plugins, but those also support the `ParaViewFilters` syntax,
 * so this hints will most likely be deprecated at some point.
 * ```
 * <Proxy name="MyFilter">
 * ...
 *   <Hints>
 *     <ShowInMenu category="SomeCategory" icon="path/to/icon" />
 *   </Hints>
 * </Proxy>
 * ```
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqProxyCategory : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqProxyCategory(pqProxyCategory* parent = nullptr);
  pqProxyCategory(pqProxyCategory* parent, const QString& name, const QString& label);
  ~pqProxyCategory() override;

  /**
   * XML Parsing methods.
   */
  ///@{
  /// Parse XML node to create children categories and proxies. Return true if modified.
  bool parseXML(vtkPVXMLElement* root);
  /// Add Proxies and SubCategories XML declaration to root element.
  void convertToXML(vtkPVXMLElement* root) { this->convertToXML(this, root); }
  /**
   * Parse `<Hints>` tag and look for `<ShowInMenu>` children to create proxy and category
   * information. Example of expected xml:
   * ``` xml
   * <Proxy>
   *   <Hints>
   *     <ShowInMenu category="SomeCategory" icon="path/to/icon" />
   *   </Hints>
   * </Proxy>
   * ```
   * Search recursively for relevant category under "root" and create it if needed.
   * This is mainly for plugins. Note that plugins also support `<ParaViewFilters>` tag.
   * Returns true if category was modified.
   */
  bool parseXMLHintsTag(
    const QString& proxyGroup, const QString& proxyName, vtkPVXMLElement* hints);
  /// Modify the vtkPVXMLElement for backward compatibility.
  static void convertLegacyXML(vtkPVXMLElement* root);
  ///@}

  /**
   * Settings methods.
   */
  ///@{
  /// Write categories into ParaView settings.
  void writeSettings(const QString& resourceTag);
  /// Load categories from ParaView settings.
  void loadSettings(const QString& resourceTag);
  ///@}

  /**
   * Copy methods.
   */
  ///@{
  /**
   * Copy only the attributes from given category.
   * Proxy list and subcategories are untouched.
   */
  void copyAttributes(pqProxyCategory* other);
  /**
   * Deep copy category.
   */
  void deepCopy(pqProxyCategory* other);
  ///@}

  /**
   * Contained proxies.
   */
  ///@{
  /// Return true if a direct child exists with the given name.
  bool hasProxy(const QString& name);
  /// Return child proxy from name. If recursive is true, forward request to subcategories until
  /// finding a matching proxy.
  pqProxyInfo* findProxy(const QString& name, bool recursive = false);
  /// Return direct child proxies.
  QList<pqProxyInfo*> getRootProxies();
  /// Return proxies including subcategories proxies.
  QList<pqProxyInfo*> getProxiesRecursive();
  /// Return direct child proxies names, preserving the declaration order.
  QStringList getOrderedRootProxiesNames();
  ///@}

  /**
   * Contained categories.
   */
  ///@{
  /// Return contained category by name. If not found, return an empty one.
  pqProxyCategory* findSubCategory(const QString& name);
  /// Return contained categories.
  QMap<QString, pqProxyCategory*> getSubCategories();
  /// Return contained categories recursively.
  QMap<QString, pqProxyCategory*> getSubCategoriesRecursive();
  /// Return categories by alphabetical name order (case insensitive).
  QList<pqProxyCategory*> getCategoriesAlphabetically();
  /// Make "name" unique in category child list by appending a numeric value.
  QString makeUniqueCategoryName(const QString& name);
  /// Make "label" unique in category child list by appending a numeric value.
  QString makeUniqueCategoryLabel(const QString& label);
  ///@}

  /**
   * Category children modifiers.
   */
  ///@{
  /// Set given category as a subcategory. Will replace any subcategory with same name.
  void addCategory(pqProxyCategory* category);
  /// remove child category
  void removeCategory(const QString& name);
  /// add a child proxy
  void addProxy(pqProxyInfo* info);
  /// remove child proxy
  void removeProxy(const QString& name);
  /// Remove children categories and proxies
  void clear();
  ///@}

  /**
   * Get properties.
   */
  ///@{
  /// Return true if it has a direct child proxy or any subcategory has a proxy.
  bool hasProxiesRecursive();
  /// Return parent category.
  pqProxyCategory* parentCategory() { return dynamic_cast<pqProxyCategory*>(this->parent()); }
  /// Returns true if there is no subcategories and no proxies.
  bool isEmpty();
  /// Return category name.
  QString name() { return this->Name; }
  /// Return category label.
  QString label();
  /// Return true if read order should be used in display.
  bool preserveOrder() { return this->PreserveOrder; }
  /// Return true if a toolbar should be created.
  bool showInToolbar() { return this->ShowInToolbar; }
  ///@}

  /**
   * Set properties.
   */
  ///@{
  /// Set category name. This also update parent cache.
  void rename(const QString& name);
  /// Create label from name if empty.
  void updateLabel(const QString& label = "");
  /// If true, a toolbar will be generated for this category. Default is false.
  void setShowInToolbar(bool show);
  ///@}

protected:
  /**
   * Add a category from the xml node.
   * If a category already exists for given name, return it instead.
   */
  pqProxyCategory* addCategory(const QString& name, vtkPVXMLElement* node);

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)

  void cleanDeletedProxy(QObject* deleted);

private:
  /**
   * Recursively parse xml to fill given category.
   */
  bool parseXML(pqProxyCategory* categoryRoot, vtkPVXMLElement* root);

  /**
   * Recursively create XML
   */
  void convertToXML(pqProxyCategory* parent, vtkPVXMLElement* root);

  QString Name;
  QString Label;

  bool PreserveOrder = false;
  bool ShowInToolbar = false;

  pqProxyInfoMap Proxies;
  QMap<QString, pqProxyCategory*> SubCategories;
  QStringList OrderedProxies;
};

/// map a category name to its pqProxyCategory
using pqCategoryMap = QMap<QString, pqProxyCategory*>;

#endif
