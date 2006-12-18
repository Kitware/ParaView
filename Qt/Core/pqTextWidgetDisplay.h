/*=========================================================================

   Program: ParaView
   Module:    pqTextWidgetDisplay.h

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
#ifndef __pqTextWidgetDisplay_h
#define __pqTextWidgetDisplay_h

#include "pqConsumerDisplay.h"

class pqTextWidgetDisplayInternal;

// This is a display representation for TextWidgetDisplay proxy.
// TextWidgetDisplay is not really a consumer display i.e. it doesn't
// have an input. However, we fake one my linking the Text property
// from the dummy input with the property on the display. 
// All this is managed by this display.
class PQCORE_EXPORT pqTextWidgetDisplay : public pqConsumerDisplay
{
  Q_OBJECT

  typedef pqConsumerDisplay Superclass;
public:
  pqTextWidgetDisplay(const QString& group, const QString& name,
    vtkSMProxy* display, pqServer* server,
    QObject* parent=0);
  virtual ~pqTextWidgetDisplay();

protected slots:
  // called when input property on display changes. We must detect if
  // (and when) the display is connected to a new proxy.
  virtual void onInputChanged();

private:
  pqTextWidgetDisplay(const pqTextWidgetDisplay&); // Not implemented.
  void operator=(const pqTextWidgetDisplay&); // Not implemented.

  pqTextWidgetDisplayInternal *Internal;
};

#endif

