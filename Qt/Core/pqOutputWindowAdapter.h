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

#include "pqCoreModule.h"

#include <QObject>
#include <vtkOutputWindow.h>

/**
 * @class pqOutputWindowAdapter
 * @brief deprecated as of ParaView 5.4. Please consider using pqOutputWidget.
 *
 * @deprecated ParaView 5.4. Please use pqOutputWidget in your application
 * instead.
 *
 * @sa pqOutputWindow
*/
class PQCORE_EXPORT pqOutputWindowAdapter : public QObject, public vtkOutputWindow
{
  Q_OBJECT

public:
  static pqOutputWindowAdapter* New();
  vtkTypeMacro(pqOutputWindowAdapter, vtkOutputWindow);

  /**
  * If active signals are emitted on messages.
  */
  void setActive(bool active);

  /**
  * These are same as DisplayText() and DisplayErrorText() except that
  * they don't pad the text with "\n" nor do they echo the text to the
  * terminal. Used in pqPythonManager to display Python text.
  */
  void DisplayTextInWindow(const QString&);
  void DisplayErrorTextInWindow(const QString&);

signals:
  /**
  * Signal emitted by VTK messages
  */
  void displayText(const QString&);

  /**
  * Signal emitted by VTK error messages
  */
  void displayErrorText(const QString&);

  /**
  * Signal emitted by VTK warning messages
  */
  void displayWarningText(const QString&);

  /**
  * Signal emitted by VTK warning messages
  */
  void displayGenericWarningText(const QString&);

  /**
  * Signal emitted by Python messages
  */
  void displayTextInWindow(const QString&);

  /**
  * Signal emitted by Python errors.
  */
  void displayErrorTextInWindow(const QString&);

private:
  pqOutputWindowAdapter();
  pqOutputWindowAdapter(const pqOutputWindowAdapter&);
  pqOutputWindowAdapter& operator=(const pqOutputWindowAdapter&);
  ~pqOutputWindowAdapter();

  virtual void DisplayText(const char*) VTK_OVERRIDE;
  virtual void DisplayErrorText(const char*) VTK_OVERRIDE;
  virtual void DisplayWarningText(const char*) VTK_OVERRIDE;
  virtual void DisplayGenericWarningText(const char*) VTK_OVERRIDE;

  bool Active;
};

#endif // !_pqOutputWindowAdapter_h
