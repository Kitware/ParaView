/*=========================================================================

   Program: ParaView
   Module:    pqOutputWindowAdapter.h

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

#ifndef _pqOutputWindowAdapter_h
#define _pqOutputWindowAdapter_h

#include "pqCoreExport.h"

#include <vtkOutputWindow.h>
#include <QObject>

/**
vtkOutputWindow implementation that converts VTK output messages into Qt signals.

To use, create an instance of pqOutputWindowAdapter and pass it to the
vtkOutputWindow::setInstance() static method.

\sa pqOutputWindow
*/
class PQCORE_EXPORT pqOutputWindowAdapter :
  public QObject,
  public vtkOutputWindow
{
  Q_OBJECT
  
public:
  static pqOutputWindowAdapter *New();
  vtkTypeMacro(pqOutputWindowAdapter, vtkOutputWindow);

  /// Returns the number of text messages received
  unsigned int getTextCount();
  /// Returns the number of error messages received
  unsigned int getErrorCount();
  /// Returns the number of warning messages received
  unsigned int getWarningCount();
  /// Returns the number of generic warning messages received
  unsigned int getGenericWarningCount();
  /// If active signals are emitted on messages.
  void setActive(bool active);

signals:
  /// Signal emitted by VTK messages
  void displayText(const QString&);
  /// Signal emitted by VTK error messages
  void displayErrorText(const QString&);
  /// Signal emitted by VTK warning messages
  void displayWarningText(const QString&);
  /// Signal emitted by VTK warning messages
  void displayGenericWarningText(const QString&);
  
private:
  pqOutputWindowAdapter();
  pqOutputWindowAdapter(const pqOutputWindowAdapter&);
  pqOutputWindowAdapter& operator=(const pqOutputWindowAdapter&);
  ~pqOutputWindowAdapter();

  unsigned int TextCount;
  unsigned int ErrorCount;
  unsigned int WarningCount;
  unsigned int GenericWarningCount;

  virtual void DisplayText(const char*);
  virtual void DisplayErrorText(const char*);
  virtual void DisplayWarningText(const char*);
  virtual void DisplayGenericWarningText(const char*);
  
  bool Active;
};

#endif // !_pqOutputWindowAdapter_h
