/*=========================================================================

   Program: ParaView
   Module:    pqSelectionInputWidget.h

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
#ifndef pqSelectionInputWidget_h
#define pqSelectionInputWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

#include "pqSMProxy.h" // For property.

/**
* pqSelectionInputWidget is a custom widget used for specifying
* the selection to use on filters that have a selection as input.
*/
class PQCOMPONENTS_EXPORT pqSelectionInputWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(pqSMProxy selection READ selection WRITE setSelection USER true)
  typedef QWidget Superclass;

public:
  pqSelectionInputWidget(QWidget* parent = 0);
  ~pqSelectionInputWidget() override;

  virtual pqSMProxy selection() { return this->SelectionSource; }

public Q_SLOTS:
  virtual void setSelection(pqSMProxy selection);

  /**
  * This must be connected to the panel-accept signal to ensure that the new
  * selection source object gets registered for undo-redo/state to work. This
  * method also gets rid of any obsolete selection_sources.
  */
  virtual void preAccept();
  virtual void postAccept();

Q_SIGNALS:
  /**
  * Signal that the selection proxy changed.
  */
  void selectionChanged(pqSMProxy);

protected Q_SLOTS:
  // Copy active selection.
  void copyActiveSelection();

  void onActiveSelectionChanged();

  void updateLabels();

protected:
  pqSMProxy SelectionSource;

private:
  Q_DISABLE_COPY(pqSelectionInputWidget)

  class UI;
  UI* ui;
};

#endif
