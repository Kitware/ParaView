/*=========================================================================

   Program: ParaView
   Module:    pqCustomFilterManager.h

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

/**
* \file pqCustomFilterManager.h
* \date 6/23/2006
*/

#ifndef _pqCustomFilterManager_h
#define _pqCustomFilterManager_h

#include "pqComponentsModule.h"
#include <QDialog>

class pqCustomFilterManagerForm;
class pqCustomFilterManagerModel;
class QItemSelection;
class QStringList;

/**
* \class pqCustomFilterManager
* \brief
*   The pqCustomFilterManager class displays the list of registered
*   custom filter definitions.
*
* The custom filter manager uses a pqCustomFilterManagerModel to get
* the list of registered custom filters. The custom filter manager
* uses the server manager to import and export custom filter
* definitions. It can also unregister the selected custom filter.
*/
class PQCOMPONENTS_EXPORT pqCustomFilterManager : public QDialog
{
  Q_OBJECT

public:
  /**
  * \brief
  *   Creates a custom filter manager dialog.
  * \param model The list of registered custom filters to display.
  * \param parent The parent widget for the dialog.
  */
  pqCustomFilterManager(pqCustomFilterManagerModel* model, QWidget* parent = 0);
  ~pqCustomFilterManager() override;

public Q_SLOTS:
  /**
  * \brief
  *   Selects the given custom filter in the list.
  * \param name The custom filter name to select.
  */
  void selectCustomFilter(const QString& name);

  /**
  * \brief
  *   Registers the custom filter definitions in the files.
  * \param files The list of files to import.
  */
  void importFiles(const QStringList& files);

  /**
  * \brief
  *   Saves the selected custom filter definitions to the given files.
  * \param files The list of files to export to.
  */
  void exportSelected(const QStringList& files);

private Q_SLOTS:
  /**
  * \brief
  *   Opens the file dialog to select import files.
  * \sa pqCustomFilterManager::importFiles(const QStringList &)
  */
  void importFiles();

  /**
  * \brief
  *   Opens the file dialog to select export files.
  * \sa pqCustomFilterManager::exportSelected(const QStringList &)
  */
  void exportSelected();

  /**
  * Unregisters the selected custom filter definitions.
  */
  void removeSelected();

  /**
  * \brief
  *   Updates the dialog buttons based on the selection.
  *
  * If there is no selection, the export and remove buttons are
  * disabled.
  *
  * \param selected The list of newly selected items.
  * \param deselected The list of deselected items.
  */
  void updateButtons(const QItemSelection& selected, const QItemSelection& deselected);

protected:
  QString getUnusedFilterName(const QString& group, const QString& name);

private:
  pqCustomFilterManagerModel* Model; ///< Stores the custom filter list.
  pqCustomFilterManagerForm* Form;   ///< Defines the gui layout.
};

#endif
