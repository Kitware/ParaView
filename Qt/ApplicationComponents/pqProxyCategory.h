// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqProxyCategory_h
#define pqProxyCategory_h

#include "pqApplicationComponentsModule.h"

#include "pqProxyInfo.h"            // for pqProxyInfoMap
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_6_3_0
#include "vtkSmartPointer.h"        // for vtkSmartPointer
#include <QMap>                     // for QMap
#include <QString>                  // for QString

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
 * Categories can contain other kinds of items: Python scripts, run when
 * triggered, and separators, both meaningful for categories that preserve the
 * order of their items (see pqProxyInfo). Proxies of other groups than the one
 * the category is for can also be added, e.g. sources in categories of filters.
 *
 * Top-level categories can be shown as menus of the menu bar (instead of
 * sub-menus), and any category as a toolbar:
 * ```
 * <ParaViewFilters>
 *   <Category name="MyTools" menu_label="My Tools" show_in_menubar="1"
 *             show_in_toolbar="1" preserve_order="1">
 *     <Proxy group="sources" name="SphereSource" />
 *     <Proxy group="filters" name="Cut" />
 *     <Separator />
 *     <Script name="Hello" label="Say Hello">print("Hello!")</Script>
 *   </Category>
 * </ParaViewFilters>
 * ```
 * When several configurations define the same category, their items are
 * merged, and the category is shown in the menu bar or as a toolbar if any
 * configuration requests it.
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
  /**
   * Write the user customizations in ParaView settings, i.e. the differences
   * between this category and `base`, typically the categories defined by the
   * application and plugins. Customizations are applied over the base when
   * loaded, so categories and items added, modified or removed later on in the
   * base, e.g. by plugins, are reflected, unless customized by the user.
   * Customizations that could not be applied when loaded, e.g. of categories
   * and items defined by plugins that are not loaded, are kept.
   */
  void writeSettings(const QString& resourceTag, pqProxyCategory* base);

  /**
   * Set this category to a copy of `base` with the user customizations from
   * ParaView settings applied, if any.
   * Categories saved in settings by previous versions, as a copy of the
   * complete category, are merged into the copy of `base` instead.
   */
  void loadSettings(const QString& resourceTag, pqProxyCategory* base);

  /**
   * Returns the customizations saved in ParaView settings for the given
   * resource tag, as read by loadSettings(), e.g. to find out whether they
   * changed.
   */
  static QString savedCustomizations(const QString& resourceTag);

  /// Write categories into ParaView settings.
  PARAVIEW_DEPRECATED_IN_6_3_0("Use writeSettings(resourceTag, base) instead.")
  void writeSettings(const QString& resourceTag);
  /// Load categories from ParaView settings.
  PARAVIEW_DEPRECATED_IN_6_3_0("Use loadSettings(resourceTag, base) instead.")
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
  /// Only relevant for top-level categories.
  bool showInMenuBar() { return this->ShowInMenuBar; }
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
  void setShowInMenuBar(bool show);

  /**
   * Returns the number of separators among the direct children.
   */
  int separatorCount();
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

  /**
   * Recursively add to `xml` the differences between `customized` and `base`,
   * which may be nullptr: added, removed and modified categories and items.
   */
  static void diff(pqProxyCategory* base, pqProxyCategory* customized, vtkPVXMLElement* xml);

  /**
   * Recursively apply the differences described by `xml` (see diff()). The
   * definitions of the scripts added are looked for in `base`. The differences
   * that cannot be applied, e.g. for categories and items defined by plugins
   * that are not loaded, are kept so that diff() adds them back.
   */
  void applyCustomizations(vtkPVXMLElement* xml, pqProxyCategory* base);

  /**
   * Keep a difference that could not be applied to the direct children of this
   * category (see applyCustomizations()).
   */
  void keepUnappliedCustomization(vtkPVXMLElement* element);

  /**
   * Remove and delete the direct child item with the given name.
   */
  void deleteProxy(const QString& name);

  /**
   * Warn if `script`, about to be added to `category`, has the same name as a
   * different script of the categories `category` is part of.
   */
  static void warnIfScriptNameIsUsed(pqProxyCategory* category, pqProxyInfo* script);

  QString Name;
  QString Label;

  bool PreserveOrder = false;
  bool ShowInToolbar = false;
  bool ShowInMenuBar = false;

  pqProxyInfoMap Proxies;
  QMap<QString, pqProxyCategory*> SubCategories;
  QStringList OrderedProxies;

  /// Differences that could not be applied to the direct children, as nested elements.
  vtkSmartPointer<vtkPVXMLElement> UnappliedCustomizations;
};

/// map a category name to its pqProxyCategory
using pqCategoryMap = QMap<QString, pqProxyCategory*>;

#endif
