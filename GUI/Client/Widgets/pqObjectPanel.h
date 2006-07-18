/*=========================================================================

   Program: ParaView
   Module:    pqObjectPanel.h

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

#ifndef _pqObjectPanel_h
#define _pqObjectPanel_h

#include <QWidget>
#include <QPointer>
class pqProxy;
class pqRenderModule;

class pqPropertyManager;

/// Base class for Widget which provides an editor for editing properties
/// of a proxy
class pqObjectPanel : public QWidget
{
  Q_OBJECT
public:
  /// constructor
  pqObjectPanel(QWidget* p);
  /// destructor
  ~pqObjectPanel();

  /// set the proxy to display properties for.
  /// subclassess should override setProxyInternal().
  void setProxy(pqProxy* proxy);

  /// get the proxy for which properties are displayed
  virtual pqProxy* proxy();
  
  /// size hint for this widget
  QSize sizeHint() const;

  /// property manager
  pqPropertyManager* getPropertyManager()
    { return this->PropertyManager; }
  
  /// get the render module that this object panel works with
  pqRenderModule* getRenderModule();

public slots:
  /// accept the changes made to the properties
  /// changes will be propogated down to the server manager
  virtual void accept();

  /// called after accept on all panels is complete
  virtual void postAccept() { }

  /// reset the changes made
  /// editor will query properties from the server manager
  virtual void reset();

  /// Called when the panel becomes active. Default implemnetation does
  /// nothing.
  virtual void select() { emit this->onselect();}

  /// Called when the panel becomes inactive. Default implemnetation does
  /// nothing.
  virtual void deselect() { emit this->ondeselect(); }

  /// Set the render module that this panel works with
  virtual void setRenderModule(pqRenderModule*);

signals:
  void canAcceptOrReject(bool);
  void onaccept();
  void onreset();
  void onselect();
  void ondeselect();
  void renderModuleChanged(pqRenderModule*);

protected:
  /// Internal method that actually sets the proxy. Subclasses must override
  /// this instead of setProxy().
  virtual void setProxyInternal(pqProxy* proxy);

  QPointer<pqProxy> Proxy;
  pqPropertyManager* PropertyManager;
  QPointer<pqRenderModule> RenderModule;
};

#endif

