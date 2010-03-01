/*=========================================================================

   Program: ParaView
   Module:    pqActiveXYChartOptions.h

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

/// \file pqActiveXYChartOptions.h
/// \date 2/26/2010

#ifndef _pqActiveXYChartOptions_h
#define _pqActiveXYChartOptions_h

#include "pqComponentsExport.h"
#include "pqActiveViewOptions.h"

class pqOptionsDialog;
class pqXYChartOptionsEditor;

/// \class pqActiveXYChartOptions
/// \brief
///   The pqActiveXYChartOptions class is used to dislpay an options
///   dialog for the chart view.
class PQCOMPONENTS_EXPORT pqActiveXYChartOptions : public pqActiveViewOptions
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a chart options instance.
  /// \param parent The parent object.
  pqActiveXYChartOptions(QObject *parent=0);
  virtual ~pqActiveXYChartOptions();

  /// \name pqActiveViewOptions Methods
  //@{
  virtual void showOptions(pqView *view, const QString &page,
      QWidget *parent=0);
  virtual void changeView(pqView *view);
  virtual void closeOptions();
  //@}

private slots:
  /// \brief
  ///   Completes the dialog closing process.
  ///
  /// Closing the dialog with the close button will apply any unsaved
  /// changes. If the user hits the escape key, or closes the dialog
  /// with the 'x' button, any unsaved changes will be ignored.
  ///
  /// This method also signals the manager that the dialog has closed.
  ///
  /// \param result Indicates if the user accepted the changes or not.
  void finishDialog(int result);

  /// Cleans up the options dialog data in case it is deleted.
  void cleanupDialog();

  void openUndoSet();
  void closeUndoSet();

private:
  pqXYChartOptionsEditor *Editor;
  pqOptionsDialog *Dialog;              ///< Stores the dialog.

};

#endif
