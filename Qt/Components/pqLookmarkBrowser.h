/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkBrowser.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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
class QItemSelectionModel;

/// \class pqLookmarkBrowser
/// \brief
///   The pqLookmarkBrowser class displays the list of lookmarks (their names and icons)
///
/// The lookmark browser uses a pqLookmarkBrowserModel to get the list of lookmarks. 
/// 
/// It provides an interface for loading, removing, importing, and exporting lookmarks.
///
/// Still to do: 
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

  QItemSelectionModel* getSelectionModel();

public slots:
  /// \brief
  ///   Selects the given lookmark in the list.
  /// \param name The lookmark name to select.
  void selectLookmark(const QString &name);

  /// \brief
  ///   Load the given lookmark in the list. Handled by pqMainWindowCore.
  /// \param index The index in the list of the lookmark to be loaded.
  void loadLookmark(const  QModelIndex &index);

  /// \brief
  ///   Saves the selected lookmark definitions to the given files. Handled by pqLookmarkManagerModel.
  /// \param files The list of files to export to.
  void exportSelected(const QStringList &files);

private slots:
  /// \brief
  ///   Opens the file dialog to select import files.
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
  void updateButtons();

  /// \brief
  ///   A house-keeping method to perform tasks that need to be done when the selection changes, like updating the button state.
  void onSelectionChanged();

signals:
  void loadLookmark(const QString &name);
  void selectedLookmarksChanged(const QStringList &names);

private:
  pqLookmarkBrowserModel *Model; ///< Stores the lookmark list.
  pqLookmarkBrowserForm *Form;   ///< Defines the gui layout.

};

#endif
