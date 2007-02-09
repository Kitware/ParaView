/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkBrowser.h

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

#ifndef _pqLookmarkBrowser_h
#define _pqLookmarkBrowser_h


#include "pqComponentsExport.h"
#include <QWidget>

class pqLookmarkBrowserForm;
class pqLookmarkBrowserModel;
class QItemSelection;
class QStringList;
class QModelIndex;
class pqServer;

/// \class pqLookmarkBrowser
/// \brief
///   The pqLookmarkBrowser class displays the list of lookmarks (their names and icons)
///
/// The lookmark browser uses a pqLookmarkBrowserModel to get the list of lookmarks. 
/// 
/// It provides an interface for loading and removing lookmarks.
///
/// Still to do: 
///  - hook up importing and exporting lookmarks
///  - convert to a tree view
///  - should we allow user to create a lookmark from the browser?


class PQCOMPONENTS_EXPORT pqLookmarkBrowser : public QWidget
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a lookmark Browser dialog.
  /// \param model The list of registered lookmarks to display.
  /// \param parent The parent widget for the dialog.
  pqLookmarkBrowser(pqLookmarkBrowserModel *model, QWidget *parent=0);
  virtual ~pqLookmarkBrowser();

public slots:
  /// \brief
  ///   Selects the given lookmark in the list.
  /// \param name The lookmark name to select.
  void selectLookmark(const QString &name);

  /// \brief
  ///   load the given lookmark in the list.
  /// \param index The index in the list of the lookmark to be loaded.
  void onLoadLookmark(const  QModelIndex &index);

  /// \brief
  ///   Creates lookmark definitions from the files.
  /// \param files The list of files to import.
  void importFiles(const QStringList &files);

  /// \brief
  ///   Saves the selected lookmark definitions to the given files.
  /// \param files The list of files to export to.
  void exportSelected(const QStringList &files);

  /// \brief
  ///   Keep track of the current server.
  ///   Question: should this be kept in pqLookmarkBrowserModel instead?
  /// \param server The new server.
  void setActiveServer(pqServer *server){this->ActiveServer = server;};

private slots:
  /// \brief
  ///   Opens the file dialog to select import files.
  /// \sa pqLookmarkBrowser::importFiles(const QStringList &)
  void importFiles();

  /// \brief
  ///   Opens the file dialog to select export files.
  /// \sa pqLookmarkBrowser::exportSelected(const QStringList &)
  void exportSelected();

  /// \brief
  ///   Delete the selected lookmarks from the display as well as the model.
  /// \sa pqLookmarkBrowserModel::removeLookmark(const QModelIndex&)
  void removeSelected();

  /// \brief
  ///   Updates the dialog buttons based on the selection.
  ///
  /// If there is no selection, the export, create, and remove buttons are
  /// disabled.
  ///
  /// \param selected The list of newly selected items.
  /// \param deselected The list of deselected items.
  void updateButtons(const QItemSelection &selected,
      const QItemSelection &deselected);

  /// \brief
  ///   Called after the user selects a server from a browser on which load the lookmark's state. 
  ///
  ///   Loads the lookmark at the currently selected index.
  ///
  /// \param server the server on which to load the state
  void loadCurrentLookmark(pqServer *server);

private:
  pqLookmarkBrowserModel *Model; ///< Stores the lookmark list.
  pqLookmarkBrowserForm *Form;   ///< Defines the gui layout.
  pqServer *ActiveServer;

};

#endif
