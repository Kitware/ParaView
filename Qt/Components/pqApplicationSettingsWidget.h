/*=========================================================================

   Program: ParaView
   Module:    pqApplicationSettingsWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#ifndef __pqApplicationSettingsWidget_h 
#define __pqApplicationSettingsWidget_h

#include <QWidget>
#include "pqComponentsExport.h"

/// pqApplicationSettingsWidget is the widget used in the pqSettingsDialog to
/// show application settings.
class PQCOMPONENTS_EXPORT pqApplicationSettingsWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;
public:
  pqApplicationSettingsWidget(QWidget* parent=0);
  virtual ~pqApplicationSettingsWidget();

public slots:
  /// Called to accept all user changes.
  void accept();

private:
  pqApplicationSettingsWidget(const pqApplicationSettingsWidget&); // Not implemented.
  void operator=(const pqApplicationSettingsWidget&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;

};

#endif


