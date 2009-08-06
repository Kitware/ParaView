/*=========================================================================

   Program: ParaView
   Module:    pqScatterPlotDisplayPanel.h

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
#ifndef __pqScatterPlotDisplayPanel_h
#define __pqScatterPlotDisplayPanel_h

#include "pqDisplayPanel.h"

class pqRepresentation;

/// Editor widget for scatter plot displays.
class PQCOMPONENTS_EXPORT pqScatterPlotDisplayPanel : public pqDisplayPanel
{
  Q_OBJECT
public:
  pqScatterPlotDisplayPanel(pqRepresentation* display, QWidget* parent=0);
  virtual ~pqScatterPlotDisplayPanel();

protected slots:
  /// Reset the camera to the data bounds
  void zoomToData();
  
  /// Enable/Disable the 3D camera mode. 
  /// It is controlled by the the Z-Coords checkbox and the "ThreeDMode" 
  /// property
  void update3DMode();
  
  /// Open a ColorMap editor dialog to change the LUT.
  void openColorMapEditor();

  /// Retrieve the data range of the color array and set it to the LUT
  void rescaleColorToDataRange();

  /// When a color array is chosen, the range must be reset
  void onColorChanged();
  
  /// Enable/Disable the Glyph mode
  void updateGlyphMode();
  
  /// Refresh the screen when the visibility of the cube axes changes
  void cubeAxesVisibilityChanged();

  /// Open a CubeAxes editor dialog to change properties of the cube axes
  void openCubeAxesEditor();

private:
  pqScatterPlotDisplayPanel(const pqScatterPlotDisplayPanel&); // Not implemented.
  void operator=(const pqScatterPlotDisplayPanel&); // Not implemented.

  void setupGUIConnections();
  /// Set the display whose properties this editor is editing.
  /// This call will raise an error is the display is not
  /// a ScatterPlotRepresentation proxy.
  void setDisplay(pqRepresentation* display);

  class pqInternal;
  pqInternal* Internal;
  bool DisableSlots;
};

#endif

