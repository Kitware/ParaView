/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkBrowserModel.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

========================================================================*/


#ifndef _pqLookmarkBrowserModel_h
#define _pqLookmarkBrowserModel_h


#include "pqComponentsExport.h"
#include <QAbstractListModel>

class pqLookmarkBrowserModelInternal;
class QString;
class QImage;
class pqServer;
class vtkPVXMLElement;

/// \class pqLookmarkBrowserModel
/// \brief
///   The pqLookmarkBrowserModel class stores the list of lookmark definitions.
/// 
/// The list is modified using the \c addLookmark and
/// \c removeLookmark methods. When a new lookmark is added
/// to the model a signal is emitted. This signal can be used to
/// highlight the new lookmark.
/// 
/// A lookmark in the list can be "loaded" (i.e. have its stored server manager state loaded in vtkSMProxyManager). 
///
/// Lookmarks can be imported or exported in Lookmark Definition Files. This is an XML representation of the set of lookmark(s). Here is the format:
///   <LookmarkDefinitionFile>
///     <LookmarkDefinition name="My Lookmark">
///         <Icon value="KDJFLSKDJFLJDLSKFJLDSKJFLSDJFLSDKJFLKDSJFLSDKFLSL.../>
///         <ServerManagerState>
///           ....
///         </ServerManagerState>
///     </LookmarkDefinition>
///   ....
///   </LookmarkDefinitionFile>
///
/// This same format is used to store the contents of the lookmark browser between sessions of ParaView. 
/// It is stored as a QString in the application's pqSetttings under the key "LookmarkBrowserState".
///
/// Still to do: Convert to a hierarchical model

class PQCOMPONENTS_EXPORT pqLookmarkBrowserModel : public QAbstractListModel
{
  Q_OBJECT

public:
  pqLookmarkBrowserModel(QObject *parent=0);
  virtual ~pqLookmarkBrowserModel();

  /// \name QAbstractItemModel Methods
  //@{
  /// \brief
  ///   Gets the number of rows for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of rows for the given index.
  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;

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
  ///   Gets the data for a given model index.
  /// \param index The model index.
  /// \param role The role to get data for.
  /// \return
  ///   The data for the given model index.
  virtual QVariant data(const QModelIndex &index,
      int role=Qt::DisplayRole) const;

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

  /// \name Index Mapping Methods
  //@{
  /// \brief
  ///   Gets the lookmark name|state|icon for the given model index.
  /// \param index The model index to look up.
  /// \return
  ///   The lookmark definition name or an empty string.
  QString getLookmarkName(const QModelIndex &index) const;
  QString getLookmarkState(const QModelIndex &index) const;
  QImage getLookmarkIcon(const QModelIndex &index) const;

  /// \brief
  ///   Gets the model index for the given lookmark name.
  /// \param filter The lookmark definition name to look up.
  /// \return
  ///   The model index for the given name.
  QModelIndex getIndexFor(const QString &name) const;
  //@}


public slots:
  /// \brief
  ///   Adds a new lookmark definition to the model.
  /// \param name The name of the new lookmark definition.
  /// \param image The icon of the new lookmark definition.
  /// \param state The server manager state of the new lookmark definition.
  void addLookmark(QString name, QImage &image, QString state);

  /// \brief
  ///   Adds new lookmark definition(s) to the model.
  /// \param browserState A string representation of a lookmark definition file.
  void addLookmarks(QString browserState);

  /// \brief
  ///   Adds new lookmark definition(s) to the model.
  /// \param browserState The root element of a lookmark definition file.
  void addLookmarks(vtkPVXMLElement *browserState);

  /// \brief
  ///   Gets an XML representation of the contents of the lookmark browser.
  /// \param filter The lookmark definition name to look up.
  /// \return
  ///   The XML representation of the contents of the lookmark browser as a string.
  QString getAllLookmarks();
  QString getLookmarks(const QModelIndexList &lookmarks);

  /// \brief
  ///   Loads the server manager state stored with the lookmark at the given index.
  /// \param index The index of the lookmark in the model.
  /// \param server The server on which to load the state.
  void loadLookmark(const QModelIndex &index, pqServer *server);

  /// \brief
  ///   Removes a lookmark definition from the model.
  /// \param name The name of the lookmark definition.
  void removeLookmark(QString name);
  void removeLookmark(const QModelIndex &index);


signals:
  /// \brief
  ///   Emitted when a new lookmark definition is added to the
  ///   model.
  /// \param name The name of the new lookmark definition.
  void lookmarkAdded(const QString &name);

private:
  /// Stores the lookmark list.
  pqLookmarkBrowserModelInternal *Internal;
};

#endif
