/*=========================================================================

   Program: ParaView
   Module:    pqObjectInspectorWidget.h

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

/// \file pqObjectInspectorWidget.h
/// \brief
///   The pqObjectInspectorWidget class is used to display the properties
///   of an object in an editable form.
///
/// \date 11/25/2005

#ifndef _pqObjectInspectorWidget_h
#define _pqObjectInspectorWidget_h

#include "pqComponentsExport.h"
#include <QWidget>
#include <QMap>
#include <QPointer>
#include "pqProxy.h"
#include "pqTimer.h"

class pqObjectPanel;
class QPushButton;
class pqPipelineSource;
class pqObjectPanel;
class pqView;
class pqOutputPort;
class pqObjectPanelInterface;


/// \class pqObjectInspectorWidget
/// \brief
///   The pqObjectInspectorWidget class is used to display the properties
///   of an object in an editable form.
class PQCOMPONENTS_EXPORT pqObjectInspectorWidget : public QWidget
{
  Q_OBJECT
public:
  pqObjectInspectorWidget(QWidget *parent=0);
  virtual ~pqObjectInspectorWidget();

  /// hint for sizing this widget
  virtual QSize sizeHint() const;
  
  /// get the render module to work in
  pqView* view();

  /// gets whether auto accept is on/off
  static bool autoAccept();
  
  /// sets whether auto accept is on/off
  static void setAutoAccept(bool);

  /// When set to true, on accept(), newly created sources will be shown.
  /// Default is false.
  void setShowOnAccept(bool val)
    { this->ShowOnAccept = val; }
  bool showOnAccept() const
    { return this->ShowOnAccept; }

public slots:
  void setProxy(pqProxy *proxy);

  /// accept the changes made to the properties
  /// changes will be propogated down to the server manager
  void accept();

  /// reset the changes made
  /// editor will query properties from the server manager
  void reset();

  /// Updates the accept/reset button state.
  void canAccept(bool status);

  /// set the render module to work in
  void setView(pqView* view);

  /// set the visibility of the delete button.
  void setDeleteButtonVisibility(bool visible);

  /// sets the enabled state of the delete button.
  void updateDeleteButtonState();

  void setOutputPort(pqOutputPort *port);

signals:
  /// emitted before accept.
  void preaccept();
  /// emitted on accept() after preaccept() but before postaccept()/
  void accepted();
  ///emitted after accept;
  void postaccept();

  /// emitted before reject.
  void prereject();
  /// emitted after reject.
  void postreject();

  /// emitted when render module is changed
  void viewChanged(pqView*);

  void helpRequested(const QString& proxyType);
  void helpRequested(const QString& groupname, const QString& proxyType);

  void canAccept();

protected slots:

  void removeProxy(pqPipelineSource* proxy);
  void showHelp();

  void deleteProxy();

  /// checks the enabled state of the delete button.
  void handleConnectionChanged(pqPipelineSource* in, pqPipelineSource* out);

  void updateAcceptState();

protected:
  /// shows the source.
  void show(pqPipelineSource*);
  
private:

  pqObjectPanelInterface* StandardCustomPanels;

  QWidget* PanelArea;
  QPushButton* AcceptButton;
  QPushButton* ResetButton;
  QPushButton* DeleteButton;
  QPushButton* HelpButton;
  QPointer<pqView> View;
  pqTimer AutoAcceptTimer;
  static bool AutoAccept;
  bool ShowOnAccept;
  pqOutputPort *OutputPort;
  
  pqObjectPanel* CurrentPanel;

  // This keeps all the panels created. 
  QMap<pqProxy*, QPointer<pqObjectPanel> > PanelStore;
};

#endif
