/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _ClientChartView_h
#define _ClientChartView_h

#include <pqSingleInputView.h>

class vtkQtChartViewBase;

class ClientChartView : public pqSingleInputView
{
  Q_OBJECT

public:
  ClientChartView(
    const QString& viewtypemodule, 
    const QString& group, 
    const QString& name, 
    vtkSMViewProxy* viewmodule, 
    pqServer* server, 
    QObject* p);
  ~ClientChartView();

  QWidget* getWidget();

  bool canDisplay(pqOutputPort* opPort) const;

  /// Returns if this view module can support 
  /// undo/redo. 
  virtual bool supportsUndo() const { return true; }

  virtual bool canUndo() const;
  virtual bool canRedo() const;

  virtual void setDefaultPropertyValues();

  /// Save a screenshot for the view. If width or height ==0,
  /// the current window size is used.
  virtual bool saveImage(int width, int height, const QString& filename);

  void resetAxes();

public slots:
  /// Called to undo interaction.
  virtual void undo();

  /// Called to redo interaction.
  virtual void redo();

protected slots:
  virtual void renderInternal();

private slots:

  /// Sets the axis layout modified.
  void setAxisLayoutModified();

protected:
  vtkQtChartViewBase *ChartView;

private:
  virtual void showRepresentation(pqRepresentation*);
  virtual void updateRepresentation(pqRepresentation*);
  virtual void hideRepresentation(pqRepresentation*);

  void updateTitles();
  void updateAxisLayout();
  void updateAxisOptions();
  void updateZoomingBehavior();

  class implementation;
  implementation* const Implementation;
};

#endif // _ClientChartView_h

