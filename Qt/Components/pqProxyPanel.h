/*=========================================================================

   Program: ParaView
   Module:    pqProxyPanel.h

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

#ifndef _pqProxyPanel_h
#define _pqProxyPanel_h

#include "pqComponentsModule.h"

#include <QWidget>
#include <QPointer>
#include "pqComponentsModule.h"
class pqProxy;
class pqView;
class pqPropertyManager;
class vtkSMProxy;

/// ******DEPRECATION WARNING*******
/// pqProxyPanel and subclasses will soon be removed
/// from ParaView. Please update your plugins and custom application to use the
/// new design of creating panels for proxies (pqProxyWidget).
/// ********************************
/// Base class for Widget which provides an editor for editing properties
/// of a proxy
class PQCOMPONENTS_EXPORT pqProxyPanel : public QWidget
{
  Q_OBJECT
public:
  /// constructor
  pqProxyPanel(vtkSMProxy* proxy, QWidget* p);
  /// destructor
  ~pqProxyPanel();

  /// get the proxy for which properties are displayed
  vtkSMProxy* proxy() const;
  
  /// get the view that this object panel works with.
  pqView* view() const;
  
  /// size hint for this widget
  QSize sizeHint() const;
  
  /// property manager belongs to this panel
  pqPropertyManager* propertyManager();

  /// returns whether selected
  bool selected() const;

public slots:
  /// accept the changes made to the properties
  /// changes will be propogated down to the server manager
  /// subclasses should only change properties when accept is called to work
  /// properly with undo/redo
  virtual void accept();

  /// reset the changes made
  /// editor will query properties from the server manager
  virtual void reset();

  /// Called when the panel becomes active. Default implemnetation does
  /// nothing.
  virtual void select();

  /// Called when the panel becomes inactive. Default implemnetation does
  /// nothing.
  virtual void deselect();

  /// Set the view that this panel works with
  virtual void setView(pqView*);
  
  /// Fires modified
  virtual void setModified();
  
private slots:
  /// Called when the vtkSMProxy fires ModifiedEvent.
  /// It implies that the proxy information properties (and domains
  /// depending on those) may now be obsolete.
  void proxyModifiedEvent();


signals:
  void modified();
  void onaccept();
  void onreset();
  void onselect();
  void ondeselect();
  void viewChanged(pqView*);

protected slots:
  /// This method gets called to referesh all domains 
  /// and information properties. Subclassess can override
  /// this to update any domain related entities.
  /// Since this is not a particularly fast operation, we update 
  /// the information and domains only when the panel is selected 
  /// or an already active panel is accepted. 
  virtual void updateInformationAndDomains();

  /// Called after the algorithm executes.
  void dataUpdated();
  
protected:
  bool event(QEvent* e);

private:
  class pqImplementation;
  pqImplementation* const Implementation;

};

#endif
