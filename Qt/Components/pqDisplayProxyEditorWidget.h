/*=========================================================================

   Program: ParaView
   Module:    pqDisplayProxyEditorWidget.h

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
#ifndef __pqDisplayProxyEditorWidget_h
#define __pqDisplayProxyEditorWidget_h

#include <QWidget>
#include "pqComponentsExport.h"
#include "pqDisplayPanel.h"

class pqOutputPort;
class pqPipelineSource;
class pqRepresentation;
class pqView;

// This is a widget that can create different kinds of display 
// editors based on the type of the representations. It encapsulates the code
// to decide what GUI for display editing must be shown to the user
// based on the type of the representation.
class PQCOMPONENTS_EXPORT pqDisplayProxyEditorWidget : public QWidget
{
  Q_OBJECT
public:
  pqDisplayProxyEditorWidget(QWidget* parent=NULL);
  virtual ~pqDisplayProxyEditorWidget();

  /// Set the source and view. Source and view are used by this class
  /// only if representation is NULL. It is used to decide if a new
  /// representation can be created for the source at all.
  void setView(pqView* view);
  void setOutputPort(pqOutputPort* port);


  pqRepresentation* getRepresentation() const;

public slots:
  void reloadGUI();

  /// Set the representation to edit. If NULL, source and view must be set so
  /// that the widget can show a default GUI which allows the user to
  /// turn visibility on which entails creating a new representation.
  void setRepresentation(pqRepresentation*);

protected slots:
  void onVisibilityChanged(bool);

private:
  pqDisplayProxyEditorWidget(const pqDisplayProxyEditorWidget&); // Not implemented.
  void operator=(const pqDisplayProxyEditorWidget&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
  void showDefaultWidget();

  void updatePanel();
};


/// default representation panel with only a visibility checkbox
class pqDefaultDisplayPanel : public pqDisplayPanel
{
  Q_OBJECT
public:
  pqDefaultDisplayPanel(pqRepresentation* repr, QWidget* p);
  ~pqDefaultDisplayPanel();

signals:
  void visibilityChanged(bool);

protected slots:
  void onStateChanged(int);

protected:
  class pqInternal;
  pqInternal* Internal;
};

#endif

