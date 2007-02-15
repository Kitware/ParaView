/*=========================================================================

   Program: ParaView
   Module:    pqDisplayPanel.h

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

=========================================================================*/
#ifndef _pqDisplayPanel_h
#define _pqDisplayPanel_h

#include <QWidget>
#include <QPointer>
#include "pqComponentsExport.h"
#include "pqDisplay.h"


/// Widget which provides an editor for the properties of a display.
class PQCOMPONENTS_EXPORT pqDisplayPanel : public QWidget
{
  Q_OBJECT
public:
  /// constructor
  pqDisplayPanel(pqDisplay* display, QWidget* p = NULL);
  /// destructor
  ~pqDisplayPanel();

  /// get the proxy for which properties are displayed
  pqDisplay* getDisplay();

public slots:
  /// TODO: get rid of this function once the server manager can
  /// inform us of display property changes
  virtual void reloadGUI();
  
  /// Requests update on all views the
  /// display is visible in.
  virtual void updateAllViews();

protected:
  QPointer<pqDisplay> Display;
};

#endif

