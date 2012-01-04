/*=========================================================================

   Program: ParaView
   Module:    pqSummaryPanelInterface.h

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

#ifndef _pqSummaryPanelInterface_h
#define _pqSummaryPanelInterface_h

#include <QtPlugin>

#include "pqComponentsExport.h"

class QWidget;

class pqProxy;
class pqObjectPanel;
class pqRepresentation;

// Interface class for plugins that create widgets to be shown on
// the summary panel.
class PQCOMPONENTS_EXPORT pqSummaryPanelInterface
{
public:
  // Destroys the summary panel interface object.
  virtual ~pqSummaryPanelInterface();

  // Creates and returns a widget to show in the properties frame
  // on the summary panel for the proxy. Returns 0 if the proxy is
  // not supported.
  virtual pqObjectPanel* createPropertiesPanel(pqProxy *proxy) const;

  // Creates and returns a widget to show in the display frame on
  // the summary panel for the representation. Returns 0 if the
  // representation is not supported.
  virtual QWidget* createDisplayPanel(pqRepresentation *representation) const;
};

Q_DECLARE_INTERFACE(pqSummaryPanelInterface, "com.kitware/paraview/summarypanel")

#endif
