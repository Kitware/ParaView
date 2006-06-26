/*=========================================================================

   Program: ParaView
   Module:    pqBundleManager.h

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

/// \file pqBundleManager.h
/// \date 6/23/2006

#ifndef _pqBundleManager_h
#define _pqBundleManager_h


#include "pqWidgetsExport.h"
#include <QDialog>

class pqBundleManagerForm;
class pqBundleManagerModel;
class QItemSelection;
class QStringList;


/// \class pqBundleManager
/// \brief
///   The pqBundleManager class displays the list of registered
///   pipeline bundle definitions.
///
/// The bundle manager uses a pqBundleManagerModel to get the list of
/// registered bundles. The bundle manager uses the server manager to
/// import and export bundle definitions. It can also unregister the
/// selected bundle definitions.
class PQWIDGETS_EXPORT pqBundleManager : public QDialog
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a bundle manager dialog.
  /// \param model The list of registered bundles to display.
  /// \param parent The parent widget for the dialog.
  pqBundleManager(pqBundleManagerModel *model, QWidget *parent=0);
  virtual ~pqBundleManager();

public slots:
  /// \brief
  ///   Selects the given bundle in the list.
  /// \param name The bundle name to select.
  void selectBundle(const QString &name);

  /// \brief
  ///   Registers the pipeline bundle definitions in the files.
  /// \param files The list of files to import.
  void importFiles(const QStringList &files);

  /// \brief
  ///   Saves the selected bundle definitions to the given files.
  /// \param files The list of files to export to.
  void exportSelected(const QStringList &files);

private slots:
  /// \brief
  ///   Opens the file dialog to select import files.
  /// \sa pqBundleManager::importFiles(const QStringList &)
  void importFiles();

  /// \brief
  ///   Opens the file dialog to select export files.
  /// \sa pqBundleManager::exportSelected(const QStringList &)
  void exportSelected();

  /// Unregisters the selected pipeline bundle definitions.
  void removeSelected();

  /// \brief
  ///   Updates the dialog buttons based on the selection.
  ///
  /// If there is no selection, the export and remove buttons are
  /// disabled.
  ///
  /// \param selected The list of newly selected items.
  /// \param deselected The list of deselected items.
  void updateButtons(const QItemSelection &selected,
      const QItemSelection &deselected);

private:
  pqBundleManagerModel *Model; ///< Stores the bundle list.
  pqBundleManagerForm *Form;   ///< Defines the gui layout.
};

#endif
