/*=========================================================================

   Program: ParaView
   Module:    pqContourWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#ifndef __pqContourWidget_h
#define __pqContourWidget_h

#include "pq3DWidget.h"
#include <QColor>

class pqServer;
class vtkSMProxy;

/// GUI for ContourWidgetRepresentation. This is a 3D widget that edits a Contour.
class PQCOMPONENTS_EXPORT pqContourWidget : public pq3DWidget
{
  Q_OBJECT
  typedef pq3DWidget Superclass;
public:
  pqContourWidget(vtkSMProxy* refProxy, vtkSMProxy* proxy, QWidget* parent);
  virtual ~pqContourWidget();

  /// Resets the bounds of the 3D widget to the reference proxy bounds.
  /// This typically calls PlaceWidget on the underlying 3D Widget
  /// with reference proxy bounds.
  /// This should be explicitly called after the panel is created
  /// and the widget is initialized i.e. the reference proxy, controlled proxy
  /// and hints have been set.
  virtual void resetBounds(double /*bounds*/[6]) {}
  virtual void resetBounds()
    { return this->Superclass::resetBounds(); }

  // Some convenient methods
  // Set the point placer/line interpolator
  virtual void setPointPlacer(vtkSMProxy*);
  virtual void setLineInterpolator(vtkSMProxy*);

  /// Activates the widget. Respects the visibility flag.
  virtual void select();
  /// Deactivates the widget.
  virtual void deselect();

  /// Get the bounds of the representation
  virtual bool getBounds(double bounds[6]) const;

  /// Set the line color
  virtual void setLineColor(const QColor& color);

signals:
  /// Signal emitted when the representation proxy's "ClosedLoop" property
  /// is modified.
  void contourLoopClosed();
  void contourDone();

public slots:
  void removeAllNodes();
  void checkContourLoopClosed();

  /// Close the contour loop
  void closeLoop(bool);

  ///Move to the next mode ( Drawing, Editing, Done )
  void updateMode( );

  ///Toggle the edit mode, which will switch between Edit/Modify mode
  void toggleEditMode( );

  ///Finish editing the contour
  void finishContour( );

  /// Resets pending changes. Default implementation
  /// pushes the property values of the controlled widget to the
  /// 3D widget properties.
  /// The correspondence is determined from the <Hints />
  /// associated with the controlled proxy.
  virtual void reset();

protected:
  /// Internal method to create the widget.
  virtual void createWidget(pqServer*);

  /// Update the widget visibility according to the WidgetVisible and Selected flags
  virtual void updateWidgetVisibility();

  /// Internal method to cleanup widget.
  void cleanupWidget();
private:
  pqContourWidget(const pqContourWidget&); // Not implemented.
  void operator=(const pqContourWidget&); // Not implemented.

  void updateRepProperty(vtkSMProxy* smProxy,
    const char* propertyName);

  class pqInternals;
  pqInternals* Internals;
};

#endif


