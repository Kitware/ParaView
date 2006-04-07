/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqWidgetsExport.h"

#include <vtkOutputWindow.h>
#include <QObject>

/// vtkOutputWindow implementation that converts VTK output messages into Qt signals
class PQWIDGETS_EXPORT pqOutputWindowAdapter :
  public QObject,
  public vtkOutputWindow
{
  Q_OBJECT
  
public:
  static pqOutputWindowAdapter *New();
  vtkTypeRevisionMacro(pqOutputWindowAdapter, vtkOutputWindow);

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
  virtual void DisplayText(const char*);
  virtual void DisplayErrorText(const char*);
  virtual void DisplayWarningText(const char*);
  virtual void DisplayGenericWarningText(const char*);
  
  pqOutputWindowAdapter();
  ~pqOutputWindowAdapter();
};

#endif // !_pqOutputWindowAdapter_h
