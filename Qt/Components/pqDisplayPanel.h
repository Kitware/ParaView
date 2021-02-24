/*=========================================================================

   Program: ParaView
   Module:    pqDisplayPanel.h

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
#ifndef _pqDisplayPanel_h
#define _pqDisplayPanel_h

#include "pqComponentsModule.h"
#include "pqRepresentation.h"
#include <QPointer>
#include <QWidget>

/**
* Widget which provides an editor for the properties of a
* representation.
*/
class PQCOMPONENTS_EXPORT pqDisplayPanel : public QWidget
{
  Q_OBJECT
public:
  /**
  * constructor
  */
  pqDisplayPanel(pqRepresentation* Representation, QWidget* p = nullptr);
  /**
  * destructor
  */
  ~pqDisplayPanel() override;

  /**
  * get the proxy for which properties are displayed
  */
  pqRepresentation* getRepresentation();

public Q_SLOTS:
  /**
  * TODO: get rid of this function once the server manager can
  * inform us of Representation property changes
  */
  virtual void reloadGUI();

  /**
  * Requests update on all views the
  * Representation is visible in.
  */
  virtual void updateAllViews();

  /**
  * Called when the data information has changed.
  */
  virtual void dataUpdated();

protected:
  QPointer<pqRepresentation> Representation;
};

#endif
