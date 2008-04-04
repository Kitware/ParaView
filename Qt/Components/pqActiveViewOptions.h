/*=========================================================================

   Program: ParaView
   Module:    pqActiveViewOptions.h

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

/// \file pqActiveViewOptions.h
/// \date 7/27/2007

#ifndef _pqActiveViewOptions_h
#define _pqActiveViewOptions_h


#include "pqComponentsExport.h"
#include <QObject>

class pqView;
class QString;
class QWidget;


/// \class pqActiveViewOptions
/// \brief
///   The pqActiveViewOptions class is the interface to the view
///   options dialogs used by the pqActiveViewOptionsManager.
class PQCOMPONENTS_EXPORT pqActiveViewOptions : public QObject
{
  Q_OBJECT

public:
  /// \brief
  ///   Constructs a view options class.
  /// \param parent The parent object.
  pqActiveViewOptions(QObject *parent=0);
  virtual ~pqActiveViewOptions() {}

  /// \brief
  ///   Opens the options dialog for the given view.
  ///
  /// If the \c page parameter is empty, the default page should be
  /// shown.
  ///
  /// \param view The view to show the options for.
  /// \param page The path to the properties page to display.
  /// \param parent The parent widget for the options dialog.
  virtual void showOptions(pqView *view, const QString &page,
      QWidget *parent=0) = 0;

  /// \brief
  ///   Changes the view displayed in the options dialog.
  ///
  /// This method is called when the active view is changed while the
  /// options dialog is open.
  ///
  /// \param view The new view to show the options for.
  virtual void changeView(pqView *view) = 0;

  /// \brief
  ///   Closes the open options dialog.
  ///
  /// This method is called when the active view is changed to a view
  /// type that is not supported by the current options dialog.
  virtual void closeOptions() = 0;

signals:
  /// \brief
  ///   Emitted when the view options dialog has been closed.
  ///
  /// This signal is used to notify the view options manager that the
  /// current dialog has been closed. Subclasses can use the dialog's
  /// \c finished(int) signal to emit this signal.
  ///
  /// \param options The view options whose dialog closed.
  void optionsClosed(pqActiveViewOptions *options);
};

#endif
