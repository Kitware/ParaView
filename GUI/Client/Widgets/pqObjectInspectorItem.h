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

/// \file pqObjectInspectorItem.h
/// \brief
///   The pqObjectInspectorItem class is used to represent an object
///   property.
///
/// \date 11/7/2005

#ifndef _pqObjectInspectorItem_h
#define _pqObjectInspectorItem_h

#include <QObject>
#include <QString>  // needed for Name
#include <QVariant> // needed for Value

class pqObjectInspectorItemInternal;
class vtkSMProperty;
class vtkSMProxy;


/// \class pqObjectInspectorItem
/// \brief
///   The pqObjectInspectorItem class is used to represent an object
///   property.
///
/// The pqObjectInspector contains a list of properties using this
/// class. The pqObjectInspectorItem can be used to create a hierarchy
/// of properties. This is used when a property contains a list of values.
class pqObjectInspectorItem : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString propertyName READ propertyName WRITE setPropertyName)
  Q_PROPERTY(QVariant value READ value WRITE setValue)

public:
  /// \brief
  ///   Creates a pqObjectInspectorItem instance.
  /// \param parent The parent object.
  pqObjectInspectorItem(QObject *parent=0);
  ~pqObjectInspectorItem();

  /// \name Property Information
  //@{
  /// \brief
  ///   Gets the property name.
  /// \return
  ///   The name  of the property.
  const QString &propertyName() const {return this->Name;}

  /// \brief
  ///   Sets the property name.
  /// \param name The name of the property.
  void setPropertyName(const QString &name);

  /// \brief
  ///   Gets the property value.
  /// \return
  ///   The value of the property.
  const QVariant &value() const {return this->Value;}

  /// \brief
  ///   Sets the property value.
  /// \param value The value of the property.
  void setValue(const QVariant &value);

  /// \brief
  ///   Gets the property domain.
  /// \return
  ///   The domain of the property.
  const QVariant &domain() const {return this->Domain;}

  /// \brief
  ///   Sets the property domain.
  /// \param d The domain of the property.
  /// \sa pqObjectInspectorItem::updateDomain(vtkSMProperty *)
  void setDomain(const QVariant &d) {this->Domain = d;}

  /// \brief
  ///   Gets whether or not the property has been modified by the user.
  /// return
  ///   True if the property has been modified by the user.
  bool isModified() const {return this->Modified;}

  /// \brief
  ///   Sets whether or not the property has been modified by the user.
  /// \param modified True if the property has been modified by the user.
  void setModified(bool modified) {this->Modified = modified;}
  //@}

  /// \name Hierarchy Methods
  //@{
  /// \brief
  ///   Gets the number of child items.
  /// \return
  ///   The number of child items.
  int childCount() const;

  /// \brief
  ///   Gets the index of a specific child item.
  /// \return
  ///   The index of the child item. -1 if it doesn't exist.
  int childIndex(pqObjectInspectorItem *child) const;

  /// \brief
  ///   Gets the child item at a specific index.
  /// \return
  ///   A pointer to the child at the given index.
  pqObjectInspectorItem *child(int index) const;

  /// \brief
  ///   Clears the list of child items.
  ///
  /// The child items will be deleted before removing them from the
  /// list.
  void clearChildren();

  /// \brief
  ///   Adds an item to the list of children.
  /// \param child The item to add to the list.
  void addChild(pqObjectInspectorItem *child);

  /// \brief
  ///   Gets the parent of this item.
  /// \return
  ///   A pointer to the parent item or null if there is no parent.
  pqObjectInspectorItem *parent() const {return this->Parent;}

  /// \brief
  ///   Sets the parent of this item.
  /// \param p The new parent of this item.
  void setParent(pqObjectInspectorItem *p) {this->Parent = p;}
  //@}

public slots:
  /// \brief
  ///   Updates the property domain information.
  ///
  /// The \c property parameter is used to get the domain information.
  /// The domain data is converted and stored for use in the item
  /// delegate. The domain is used to determine an appropriate editor.
  ///
  /// \param property The property to get domain information from.
  /// \sa pqObjectInspectorItem::convertLimit(const char *, int),
  ///     pqObjectInspectorDelegate
  void updateDomain(vtkSMProxy* proxy, vtkSMProperty *property);

signals:
  /// \brief
  ///   Called when the property name changes.
  /// \param item The instance being modified.
  void nameChanged(pqObjectInspectorItem *item);

  /// \brief
  ///   Called when the property value changes.
  /// \param item The instance being modified.
  void valueChanged(pqObjectInspectorItem *item);

private:
  /// \brief
  ///   Converts a min/max limit from a property.
  ///
  /// The property returns the limit as a string. This method converts
  /// the string to a value based on the type of value (int or double).
  /// If the string is empty (meaning there is no limit), an invalid
  /// value is returned.
  ///
  /// \param limit The min/max limit string to be converted.
  /// \param type The value type.
  /// \return
  ///   The converted value of the limit string.
  /// \sa pqObjectInspectorItem::updateDomain(vtkSMProperty *)
  QVariant convertLimit(const char *limit, int type) const;

private:
  QString Name;                            ///< The property name.
  QVariant Value;                          ///< The property value.
  QVariant Domain;                         ///< The property domain.
  pqObjectInspectorItemInternal *Internal; ///< The list of child items.
  pqObjectInspectorItem *Parent;           ///< A pointer to the parent.
  bool Modified;                           ///< True if the item is modified.
};


#endif
