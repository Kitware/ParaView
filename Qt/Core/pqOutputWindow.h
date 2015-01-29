/*=========================================================================

   Program: ParaView
   Module:    pqOutputWindow.h

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

#ifndef _pqOutputWindow_h
#define _pqOutputWindow_h

#include "pqCoreModule.h"

#include <QDialog>

/**
Provides an output dialog that will display all VTK/ParaView debug/warning/error output.

To use, create an instance of pqOutputWindowAdapter and connect its output signals
to the corresponding pqOutputWindow slots.

\sa pqOutputWindowAdapter
*/
class PQCORE_EXPORT pqOutputWindow :
  public QDialog
{
  typedef QDialog Superclass;  
  Q_OBJECT

public:
  enum MessageType
  {
    ERROR,
    WARNING,
    DEBUG,
    MESSAGE_TYPE_COUNT
  };

  pqOutputWindow(QWidget* Parent);
  ~pqOutputWindow();

  ///
  static pqOutputWindow* instance();

public slots:
  void onDisplayText(const QString&);
  void onDisplayErrorText(const QString&);
  void onDisplayWarningText(const QString&);
  void onDisplayGenericWarningText(const QString&);

  /// These are same as onDisplayText() and onDisplayErrorText() except that
  /// they don't pad the text with "\n" nor do they echo the text to the
  /// terminal. Used in pqPythonManager to display Python text.
  void onDisplayTextInWindow(const QString&);
  void onDisplayErrorTextInWindow(const QString&);

private slots:
  /// Standard methods for a QDialog. Both hide the dialog.
  void accept();
  void reject();
  /// Clears all messages from the output window
  void clear();
  /// Sets the console view if 'on' is true or the table view if 'on' is false.
  void setConsoleView(int on);
  /// Slots called to filter (on or off) errors, warnings or debug messages.
  void errorToggled(bool checked);
  void warningToggled(bool checked);
  void debugToggled(bool checked);

private:
  pqOutputWindow(const pqOutputWindow&);
  pqOutputWindow& operator=(const pqOutputWindow&);
  void addMessage(int messageType, const QString& text);
  void addPythonMessages(int messageType, const QString& text);

  void addMessage(int messageType, const QString& location, 
                  const QString& message);
  
  virtual void showEvent(QShowEvent*);
  virtual void hideEvent(QHideEvent*);
  
  bool Show[MESSAGE_TYPE_COUNT];
  struct pqImplementation;
  const QScopedPointer<pqImplementation> Implementation;
};

#endif // !_pqOutputWindow_h
