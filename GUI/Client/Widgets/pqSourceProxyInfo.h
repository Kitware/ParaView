/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

=========================================================================*/


/// \file pqSourceProxyInfo.h
/// \brief
///   The pqSourceProxyInfo class is used to group items in the filter
///   menu.
///
/// \date 1/27/2006

#ifndef _pqSourceProxyInfo_h
#define _pqSourceProxyInfo_h


#include "pqWidgetsExport.h"

class pqSourceProxyInfoInternal;
class QString;
class QStringList;
class vtkPVXMLElement;


/// \class pqSourceProxyInfo
/// \brief
///   The pqSourceProxyInfo class is used to group items in the filter
///   menu.
class PQWIDGETS_EXPORT pqSourceProxyInfo
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

  /*!
      \brief
        Loads the filter grouping information from an xml structure.
     
      The current filter information will be cleared before reading in
      the new information. The xml structure should define categories
      inside of category groups. Grouping the categories determines
      where to put the menu separators.
     
      The xml should be formatted as follows:
      \code
      <SomeRootName>
        <CategoryGroup>
          <Category name="Favorites" menuName="&Favorites">
            <Filter name="Clip" />
            ...
          </Category>
          <Category name="Alphbetical" menuName="&Alphbetical" />
        </CategoryGroup>
        <CategoryGroup>
          ...
        </CategoryGroup>
        ...
      <\SomeRootName>
      \endcode
      The xml root name can be anything. The other elements should be
      named accordint to the example. The category menu name is optional.
      if there is no menu name, the menu will display the name. The menu
      name can be used to specify a keyboard shortcut for the menu. Each
      of the sub-elements can be entered multiple times. There is no limit
      to the number of filters that can be added to a category, etc.
     
      \param root The root of the filter information in the xml.
   */
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
