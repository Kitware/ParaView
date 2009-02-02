/*=========================================================================

   Program: ParaView
   Module:    pqOptionsDialog.h

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

=========================================================================*/

/// \file pqOptionsDialog.h
/// \date 7/26/2007

#ifndef _pqOptionsDialog_h
#define _pqOptionsDialog_h


#include "pqComponentsExport.h"
#include <QDialog>

class pqOptionsContainer;
class pqOptionsDialogForm;
class pqOptionsPage;
class QString;


/// \class pqOptionsDialog
/// \brief
///   The pqOptionsDialog class is a generic options dialog.
///
/// Pages can be added to the dialog using the pqOptionsPage and
/// pqOptionsContainer interfaces. The options dialog has apply and
/// reset buttons that the pages can use.
class PQCOMPONENTS_EXPORT pqOptionsDialog : public QDialog
{
  Q_OBJECT

public:
  pqOptionsDialog(QWidget *parent=0);
  virtual ~pqOptionsDialog();

  /// \brief
  ///   Gets whether or not there are changes to apply.
  /// \return
  ///   True if there are changes to apply.
  bool isApplyNeeded() const;

  /// \brief
  ///   Sets whether or not there are changes to apply.
  /// \param applyNeeded True if there are changes to apply.
  void setApplyNeeded(bool applyNeeded);

  /// \brief
  ///   Adds a page to the options dialog.
  ///
  /// When the options object is a page container, the path parameter
  /// becomes the path prefix for the container pages.
  ///
  /// \param path The name hierarchy for the options page.
  /// \param options The options page.
  void addOptions(const QString &path, pqOptionsPage *options);

  /// \brief
  ///   Adds a container to the options dialog.
  ///
  /// Each page listed for the container is added to the root of the
  /// selection tree.
  ///
  /// \param options The options container to add.
  void addOptions(pqOptionsContainer *options);

  /// \brief
  ///   Removes the options page from the options dialog.
  ///
  /// The page name is removed from the selection tree. If the page
  /// is an options container, all the names are removed.
  ///
  /// \param options The options page/container to remove.
  void removeOptions(pqOptionsPage *options);

public slots:
  /// \brief
  ///   Sets the current options page.
  /// \param path The name of the options page to show.
  void setCurrentPage(const QString &path);

  /// Calls each page to apply any changes.
  void applyChanges();

  /// Calls each page to reset any changes.
  void resetChanges();

signals:
  /// Emitted before the option changes are applied.
  void aboutToApplyChanges();

  /// Emitted after the option changes have been applied.
  void appliedChanges();

private slots:
  /// Changes the current page to match the user selection.
  void changeCurrentPage();

  /// Enabled the apply and reset buttons.
  void enableButtons();

private:
  pqOptionsDialogForm *Form; /// Stores the form and class data.
};

#endif
