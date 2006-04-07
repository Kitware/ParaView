/*=========================================================================

   Program:   ParaQ
   Module:    pqObjectInspector.h

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

/// \file pqObjectInspector.h
/// \brief
///   The pqObjectInspector class is a model for an object's properties.
///
/// \date 11/7/2005

#ifndef _pqObjectInspector_h
#define _pqObjectInspector_h

#include "pqWidgetsExport.h"
#include <QAbstractItemModel>

class pqObjectInspectorInternal;
class pqObjectInspectorItem;
class vtkSMProxy;

/// \class pqObjectInspector
/// \brief
///   The pqObjectInspector class is a model for an object's properties.
///
/// The model contains a list of properties. If the property contains
/// a list of values, it is represented using a list of sub-items.
/// Since the model is hirarchical, it is best viewed in a tree view.
/// The pqObjectInspectorDelegate class can be used in conjuntion with
/// the model to allow the user to edit the properties.
class PQWIDGETS_EXPORT pqObjectInspector : public QAbstractItemModel
{
  Q_OBJECT

public:
  /// \enum CommitType
  /// \brief
  ///   Used to set the value change policy.
  enum CommitType {
    Individually, ///< Changes should be committed one at a time.
    Collectively  ///< Changes should be committed as a group.
  };

public:
  /// \brief
  ///   Creates a pqObjectInspector instance.
  /// \param parent The parent object.
  pqObjectInspector(QObject *parent=0);
  virtual ~pqObjectInspector();

  /// \name QAbstractItemModel Methods
  //@{
  /// \brief
  ///   Gets the number of rows for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of rows for the given index.
  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the number of columns for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of columns for the given index.
  virtual int columnCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets whether or not the given index has child items.
  /// \param parent The parent index.
  /// \return
  ///   True if the given index has child items.
  virtual bool hasChildren(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets a model index for a given location.
  /// \param row The row number.
  /// \param column The column number.
  /// \param parent The parent index.
  /// \return
  ///   A model index for the given location.
  virtual QModelIndex index(int row, int column,
      const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the parent for a given index.
  /// \param index The model index.
  /// \return
  ///   A model index for the parent of the given index.
  virtual QModelIndex parent(const QModelIndex &index) const;

  /// \brief
  ///   Gets the data for a given model index.
  /// \param index The model index.
  /// \param role The role to get data for.
  /// \return
  ///   The data for the given model index.
  virtual QVariant data(const QModelIndex &index,
      int role=Qt::DisplayRole) const;

  /// \brief
  ///   Sets the value of a given model index.
  /// \param index The model index.
  /// \param value The new value.
  /// \param role The role to set data for.
  /// \return
  ///   True if the model was successfully modified.
  virtual bool setData(const QModelIndex &index, const QVariant &value,
      int role=Qt::EditRole);

  /// \brief
  ///   Gets the flags for a given model index.
  ///
  /// The flags for an item indicate if it is enabled, editable, etc.
  ///
  /// \param index The model index.
  /// \return
  ///   The flags for the given model index.
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;
  //@}

  /// \brief
  ///   Gets the property domain for a given index.
  ///
  /// This method is used by the delegate to determine the appropriate
  /// editor for the property.
  ///
  /// \return
  ///   The domain of the property.
  /// \sa pqObjectInspectorDelegate
  QVariant domain(const QModelIndex &index) const;

  /// \brief
  ///   Gets the current object.
  /// \return
  ///   A pointer to the current object.
  vtkSMProxy *proxy() const {return this->Proxy;}

  /// \brief
  ///   Gets the change commit policy.
  /// \return
  ///   The change commit policy.
  CommitType commitType() const {return this->Commit;}

public slots:
  /// \brief
  ///   Sets the current object.
  ///
  /// If there is already a current object, its properties will be
  /// removed from the list. The properties for the new object will
  /// be added to the list. The pqSMAdaptor is used to get the values
  /// for each of the properties. It is also used to link those
  /// properties to the object inspector items. Model reset is called
  /// after all the changes are made to notify the view.
  ///
  /// \param proxy The current object.
  void setProxy(vtkSMProxy *proxy);

  /// \brief
  ///   Sets the change commit policy.
  /// \param commit The new change commit policy.
  void setCommitType(CommitType commit);

  /// \brief
  ///   Commits the changes for all modified properties.
  ///
  /// Searches the list of property items for modified valus. Each
  /// modified item is committed. This method should be called when
  /// using the commit \c Collectively policy.
  void commitChanges();

  /// update display properties when changed in object inspector
  void updateDisplayProperties(pqObjectInspectorItem* item);

private:
  /// \brief
  ///   Cleans up the list of property items.
  ///
  /// This method does not emit any Qt model/view signals.
  ///
  /// \sa pqObjectInspector::setProxy(vtkSMProxy *)
  void cleanData();

  /// \brief
  ///   Gets the index of the given item.
  /// \param item The item to find the index for.
  /// \return
  ///   The index of the given item or -1 if it doesn't exist.
  int itemIndex(pqObjectInspectorItem *item) const;

  /// \brief
  ///   Commits changes for a specific item.
  ///
  /// This method does not check the commit policy. It is used for
  /// individual commits and by the collective commit method.
  ///
  /// \param item The item to commit changes for.
  /// \sa pqObjectInspector::commitChanges()
  void commitChange(pqObjectInspectorItem *item);

private slots:
  /// \brief
  ///   Signals the view that the model has changed.
  /// \param item A pointer to the modified item.
  /// \sa pqObjectInspector::handleValueChange(pqObjectInspectorItem *)
  void handleNameChange(pqObjectInspectorItem *item);

  /// \brief
  ///   Signals the view that the model has changed.
  ///
  /// Each object is linked to the associated property, which will
  /// update the individual items when a change is made. The view
  /// needs to be updated as well. This method can be connected to
  /// the item's \c nameChanged signal to update the view.
  ///
  /// \param item A pointer to the modified item.
  void handleValueChange(pqObjectInspectorItem *item);

private:
  CommitType Commit;                   ///< The change commit policy.
  pqObjectInspectorInternal *Internal; ///< The list of property items.
  vtkSMProxy *Proxy;             ///< A pointer to the current object.
};

#endif
