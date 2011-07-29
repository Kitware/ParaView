/*=========================================================================

   Program: ParaView
   Module:    pqProxyTabWidget.h

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
#ifndef _pqProxyTabWidget_h
#define _pqProxyTabWidget_h

#include <QTabWidget>
#include <QPointer>
#include "pqComponentsExport.h"

class pqDataRepresentation;
class pqDisplayProxyEditorWidget;
class pqObjectInspectorWidget;
class pqOutputPort;
class pqPipelineSource;
class pqProxyInformationWidget;
class pqView;

/// Tabbed widget with 3 tabs (object inspector, display editor, information)
class PQCOMPONENTS_EXPORT pqProxyTabWidget : public QTabWidget
{
  Q_OBJECT
public:
  /// constructor
  pqProxyTabWidget(QWidget* p=0);
  /// destructor
  ~pqProxyTabWidget();

  /// get the object inspector
  pqObjectInspectorWidget* getObjectInspector();

  /// get the display editor
  pqDisplayProxyEditorWidget* getDisplayEditor() const {return this->Display;}
  
  enum TabIndexes {
    PROPERTIES =0,
    DISPLAY=1,
    INFORMATION=2
  };

  /// By default pqProxyTabWidget connects to pqActiveObjects to know when the
  /// active port/view change. If your application does not what this behavior
  /// then in  that case you should call removeDefaultConnections() and set up
  /// your connections to the public slots. If default behavior is acceptable,
  /// then no need to call setupDefaultConnections() since that's done in the
  /// constructor itself.
  void setupDefaultConnections();
  void removeDefaultConnections();

  /// When set to true, on accept(), newly created sources will be shown.
  /// Default is false.
  void setShowOnAccept(bool val);
  bool showOnAccept() const;

public slots:
  /// set the current render module that these panels work on.
  /// By default these slots are connected to corresponding signals on
  /// pqActiveObjects. So unless your application does not what that behavior,
  /// there's no need to connect to these slots.
  void setView(pqView* rm);

  /// Set the output port whose information is to be shown in the 
  /// information tab.
  /// set the current render module that these panels work on.
  /// By default these slots are connected to corresponding signals on
  /// pqActiveObjects. So unless your application does not what that behavior,
  /// there's no need to connect to these slots.
  void setOutputPort(pqOutputPort* port);

  /// Set the active representation.
  void setRepresentation(pqDataRepresentation* repr);


  void showPropertiesTab()
    {
    this->setCurrentIndex(pqProxyTabWidget::PROPERTIES);
    }

protected:
  /// Set the display whose properties we want to edit. 
  void setProxy(pqPipelineSource* source);

  /// get the proxy for which properties are displayed
  pqPipelineSource* getProxy();

private:
  pqObjectInspectorWidget* Inspector;
  pqDisplayProxyEditorWidget* Display;
  pqProxyInformationWidget* Information;
  
  QPointer<pqOutputPort> OutputPort;
  QPointer<pqView> View;
};

#endif

