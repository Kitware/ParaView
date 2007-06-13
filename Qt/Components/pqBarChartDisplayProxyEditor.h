/*=========================================================================

   Program: ParaView
   Module:    pqBarChartDisplayProxyEditor.h

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
#ifndef __pqBarChartDisplayProxyEditor_h
#define __pqBarChartDisplayProxyEditor_h

#include "pqDisplayPanel.h"

class pqRepresentation;

/// pqBarChartDisplayProxyEditor is the editor widget for
/// a Bar Chart display.
class PQCOMPONENTS_EXPORT pqBarChartDisplayProxyEditor : public pqDisplayPanel
{
  Q_OBJECT
public:
  pqBarChartDisplayProxyEditor(pqRepresentation* display, QWidget* parent=0);
  virtual ~pqBarChartDisplayProxyEditor();

public slots:
  /// Forces a reload of the GUI elements that depend on
  /// the display proxy.
  void reloadGUI();

protected slots:
  /// Opens the color map editor.
  void openColorMapEditor();

protected:
  /// Cleans up internal data structures.
  void cleanup();

private:
  
  /// Set the display whose properties this editor is editing.
  /// This call will raise an error is the display is not
  /// a BarChartPlotDisplay proxy.
  void setRepresentation(pqRepresentation* display);

  pqBarChartDisplayProxyEditor(const pqBarChartDisplayProxyEditor&); // Not implemented.
  void operator=(const pqBarChartDisplayProxyEditor&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif

