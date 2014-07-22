/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#ifndef __pqProxyWidgetDialog_h
#define __pqProxyWidgetDialog_h

#include <QDialog>
#include "pqComponentsModule.h"

class vtkSMProxy;

/// pqProxyWidgetDialog is used to show properties of any proxy in a dialog. It
/// simply wraps the pqProxyWidget for the proxy in a dialog with Apply,
/// Cancel, and Ok buttons. Tool buttons (QPushButtons with only icons) are
/// also provided to save the currently applied settings as default properties
/// as well as to reset the defaults to the application defaults.
class PQCOMPONENTS_EXPORT pqProxyWidgetDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;
public:
  pqProxyWidgetDialog(vtkSMProxy* proxy, QWidget* parent=0);
  pqProxyWidgetDialog(vtkSMProxy* proxy, const QStringList& properties, QWidget* parent=0);
  virtual ~pqProxyWidgetDialog();

  /// Returns whether that dialog has any visible widgets.
  bool hasVisibleWidgets() const;

protected slots:
  /// slot to enable appropriate buttons when changes are available
  virtual void onChangeAvailable();

  /// slot to handle accepted() signals
  virtual void onAccepted();

private:
  Q_DISABLE_COPY(pqProxyWidgetDialog)

  class pqInternals;
  pqInternals* Internals;
};

#endif
