/*=========================================================================

   Program: ParaView
   Module:    pqProxyTabWidget.h

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
#ifndef _pqProxyTabWidget_h
#define _pqProxyTabWidget_h

#include <QTabWidget>
#include <QPointer>
#include "pqComponentsExport.h"

class pqProxy;
class pqRenderModule;
class pqObjectInspectorWidget;
class pqDisplayProxyEditor;
class pqProxyInformationWidget;

/// Tabbed widget with 3 tabs (object inspector, display editor, information)
class PQCOMPONENTS_EXPORT pqProxyTabWidget : public QTabWidget
{
  Q_OBJECT
public:
  /// constructor
  pqProxyTabWidget(QWidget* p);
  /// destructor
  ~pqProxyTabWidget();

  /// get the object inspector
  pqObjectInspectorWidget* getObjectInspector();
  
  /// get the proxy for which properties are displayed
  pqProxy* getProxy();

public slots:
  /// Set the display whose properties we want to edit. 
  void setProxy(pqProxy* source);
  
  /// set the current render module that these panels work on
  void setRenderModule(pqRenderModule* rm);

protected slots:
  void updateDisplayTab();

private:
  pqObjectInspectorWidget* Inspector;
  pqDisplayProxyEditor* Display;
  pqProxyInformationWidget* Information;
  
  QPointer<pqProxy> Proxy;
  QPointer<pqRenderModule> RenderModule;

};

#endif

