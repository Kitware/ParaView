/*=========================================================================

   Program: ParaView
   Module:    pqActiveViewOptionsManager.h

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

/// \file pqActiveViewOptionsManager.h
/// \date 7/27/2007

#ifndef _pqActiveViewOptionsManager_h
#define _pqActiveViewOptionsManager_h


#include "pqComponentsExport.h"
#include <QObject>

class pqActiveViewOptions;
class pqActiveViewOptionsManagerInternal;
class pqView;
class QString;
class QWidget;


/// \class pqActiveViewOptionsManager
/// \brief
///   The pqActiveViewOptionsManager class is used to open the
///   appropriate view options dialog.
class PQCOMPONENTS_EXPORT pqActiveViewOptionsManager : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a view options manager.
  pqActiveViewOptionsManager(QObject *parent=0);
  virtual ~pqActiveViewOptionsManager();

  /// \brief
  ///   Registers an options dialog handler with a view type.
  /// \param viewType The name of the view type.
  /// \param options The options dialog handler.
  /// \return
  ///   True if the registration was successful.
  bool registerOptions(const QString &viewType, pqActiveViewOptions *options);

  /// \brief
  ///   Removes the options dialog from the name mapping.
  /// \param options The options dialog handler.
  void unregisterOptions(pqActiveViewOptions *options);

  /// \brief
  ///   Gets whether or not the options dialog is registered.
  /// \param options The options dialog handler.
  /// \return
  ///   True if the options dialog is associated with a view type.
  bool isRegistered(pqActiveViewOptions *options) const;

  /// \brief
  ///   Gets the options dialog handler for the specified view type.
  /// \param viewType The name of the view type.
  /// \return
  ///   A pointer to the options dialog handler.
  pqActiveViewOptions *getOptions(const QString &viewType) const;

  /// Returns if an options dialog can be shown for the given view.
  bool canShowOptions(pqView* view) const;

public slots:
  /// \brief
  ///   Sets the active view.
  /// \param view The new active view.
  void setActiveView(pqView *view);

  /// Shows the options dialog for the active view.
  void showOptions();

  /// \brief
  ///   Shows the options dialog for the active view.
  /// \param page The options page to display.
  void showOptions(const QString &page);

private slots:
  /// \brief
  ///   Clears the current options dialog handler.
  /// \param options The current options dialog handler.
  void removeCurrent(pqActiveViewOptions *options);

private:
  /// \brief
  ///   Gets the current options dialog handler.
  ///
  /// The active view is used to look up the appropriate options
  /// dialog handler.
  ///
  /// \return
  ///   A pointer to the current options dialog handler.
  pqActiveViewOptions *getCurrent() const;

private:
  /// Stores the view/handler mapping.
  pqActiveViewOptionsManagerInternal *Internal;
};

#endif
