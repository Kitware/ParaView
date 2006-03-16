
/// \file pqSourceProxyInfo.h
/// \brief
///   The pqSourceProxyInfo class is used to group items in the filter
///   menu.
///
/// \date 1/27/2006

#ifndef _pqSourceProxyInfo_h
#define _pqSourceProxyInfo_h


#include "QtWidgetsExport.h"

class pqSourceProxyInfoInternal;
class QString;
class QStringList;
class vtkPVXMLElement;


/// \class pqSourceProxyInfo
/// \brief
///   The pqSourceProxyInfo class is used to group items in the filter
///   menu.
class QTWIDGETS_EXPORT pqSourceProxyInfo
{
public:
  pqSourceProxyInfo();
  ~pqSourceProxyInfo();

  /// Resets the filter grouping information.
  void Reset();

  /// \brief
  ///   Gets whether or not the filter information has been loaded.
  /// \return
  ///   True if the filter information has been loaded
  bool IsFilterInfoLoaded() const;

  /// \brief
  ///   Loads the filter grouping information from an xml structure.
  ///
  /// The current filter information will be cleared before reading in
  /// the new information. The xml structure should define categories
  /// inside of category groups. Grouping the categories determines
  /// where to put the menu separators.
  ///
  /// The xml should be formatted as follows:
  /// \code
  /// <SomeRootName>
  ///   <CategoryGroup>
  ///     <Category name="Favorites" menuName="&Favorites">
  ///       <Filter name="Clip" />
  ///       ...
  ///     </Category>
  ///     <Category name="Alphbetical" menuName="&Alphbetical" />
  ///   </CategoryGroup>
  ///   <CategoryGroup>
  ///     ...
  ///   </CategoryGroup>
  ///   ...
  /// <\SomeRootName>
  /// \endcode
  /// The xml root name can be anything. The other elements should be
  /// named accordint to the example. The category menu name is optional.
  /// if there is no menu name, the menu will display the name. The menu
  /// name can be used to specify a keyboard shortcut for the menu. Each
  /// of the sub-elements can be entered multiple times. There is no limit
  /// to the number of filters that can be added to a category, etc.
  ///
  /// \param root The root of the filter information in the xml.
  void LoadFilterInfo(vtkPVXMLElement *root);

  /// \brief
  ///   Gets the list of menu items based on the filter information.
  ///
  /// Each entry in the list corresponds to a category in the xml. An
  /// empty entry is used to separate category groups. If a category
  /// does not have a menu name specified, the category name is used.
  ///
  /// \param menuList Used to return the list of menu names.
  void GetFilterMenu(QStringList &menuList) const;

  /// \brief
  ///   Gets the list of categories the specified filter is in.
  /// \param name The name of the filter.
  /// \param list Used to return the list of category names.
  void GetFilterCategories(const QString &name, QStringList &list) const;

  /// \brief
  ///   Gets the list of categories the specified filter is in.
  /// \param name The name of the filter.
  /// \param list Used to return the list of category menu names.
  void GetFilterMenuCategories(const QString &name, QStringList &list) const;

private:
  pqSourceProxyInfoInternal *Internal; ///< Stores the filter grouping.
};

#endif
