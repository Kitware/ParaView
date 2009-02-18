/*=========================================================================

   Program: ParaView
   Module:    pqPlotView.h

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
#ifndef __pqPlotView_h
#define __pqPlotView_h

#include "pqView.h"

class pqPlotViewInternal;
class vtkObject;


class PQCORE_EXPORT pqPlotView : public pqView
{
  Q_OBJECT
public:
  typedef pqView Superclass;

  static QString barChartType() { return "BarChartView"; }
  static QString barChartTypeName() { return "Bar Chart"; }
  static QString XYPlotType() { return "XYPlotView"; }
  static QString XYPlotTypeName() { return "XY Plot"; }

  pqPlotView(const QString& type, const QString& group, const QString& name, 
    vtkSMViewProxy* renModule, 
    pqServer* server, QObject* parent=NULL);
  virtual ~pqPlotView();

  QWidget* getWidget();

  virtual void setDefaultPropertyValues();

  virtual bool supportsUndo() const;

  /// Save a screenshot for the render module. If width or height ==0,
  /// the current window size is used.
  virtual bool saveImage(int /*width*/, int /*height*/, 
    const QString& /*filename*/);

  /// Capture the view image into a new vtkImageData with the given magnification
  /// and returns it.
  virtual vtkImageData* captureImage(int magnification);
  virtual vtkImageData* captureImage(const QSize& asize);

  /// Forces an immediate render. Overridden since for plots
  /// rendering actually happens on the GUI side, not merely
  /// in the ServerManager.
  virtual void forceRender();

  /// Request a delayed forceRender().
  virtual void render();

  /// Resets the zoom level to 100%.
  virtual void resetDisplay();

  /// Called to undo interaction.
  virtual void undo();

  /// Called to redo interaction.
  virtual void redo();

  /// Returns true if undo can be done.
  virtual bool canUndo() const;

  /// Returns true if redo can be done.
  virtual bool canRedo() const;

  /// Called to check if the given source can be shown in this view.
  virtual bool canDisplay(pqOutputPort* opPort) const;

public slots:
  /// Add display to the view. Although this model supports adding
  /// more than 1 display, it shows only 1 plot at a time.
  void addRepresentation(pqRepresentation* display);

  /// Remove a display.
  void removeRepresentation(pqRepresentation* display);

  /// Remove all displays.
  void removeAllRepresentations();

private slots:
  // Called when render is called on the undelying proxy.
  // Since ServerManager does not really "render" for plots,
  // we catch the signal and render in the client.
  void renderInternal();

  /// Internal slot.
  void delayedRender();

  /// Sets the axis layout modified.
  void setAxisLayoutModified();

signals:
  /// Give listeners a chance to modify chart title text.
  void beginSetTitleText(const pqPlotView *, QString &);

private:
  pqPlotView(const pqPlotView&); // Not implemented.
  void operator=(const pqPlotView&); // Not implemented.

  void updateTitles();
  void updateAxisLayout();
  void updateAxisOptions();

  pqPlotViewInternal* Internal;
};


#endif

