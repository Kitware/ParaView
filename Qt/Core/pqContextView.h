/*=========================================================================

   Program: ParaView
   Module:    pqContextView.h

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
#ifndef __pqContextView_h
#define __pqContextView_h

#include "pqView.h"

class vtkSMContextViewProxy;
class vtkContextView;

/// pqContextView is an abstract base class for all charting views based on the
/// VTK context charting library.
class PQCORE_EXPORT pqContextView : public pqView
{
  Q_OBJECT
  typedef pqView Superclass;
public:
  virtual ~pqContextView();

  /// Return a widget associated with this view.
  virtual QWidget* getWidget();

  /// Returns the internal vtkContextView which provides the implementation for
  /// the chart rendering.
  virtual vtkContextView* getVTKContextView() const;

  /// Returns the context view proxy associated with this object.
  virtual vtkSMContextViewProxy* getContextViewProxy() const;

  /// Save a screenshot for the render module. If width or height ==0,
  /// the current window size is used.
  virtual bool saveImage(int width, int height, const QString& filename);

  /// Capture the view image into a new vtkImageData with the given magnification
  /// and returns it. The caller is responsible for freeing the returned image.
  virtual vtkImageData* captureImage(int magnification);
  virtual vtkImageData* captureImage(const QSize& asize)
    { return this->Superclass::captureImage(asize); }

  /// This view supports undo so this method always returns true.
  virtual bool supportsUndo() const {return true;}

  /// Called to undo interaction.
  virtual void undo();

  /// Called to redo interaction.
  virtual void redo();

  /// Returns true if undo can be done.
  virtual bool canUndo() const;

  /// Returns true if redo can be done.
  virtual bool canRedo() const;

  /// Resets the zoom level to 100%.
  virtual void resetDisplay();

  /// Returns true if data on the given output port can be displayed by this view.
  virtual bool canDisplay(pqOutputPort* opPort) const;

protected slots:
  virtual void initializeAfterObjectsCreated();

  /// Sets up the interactors correctly.
  virtual void initializeInteractors();

protected:
  /// Constructor:
  /// \c type  :- view type.
  /// \c group :- SManager registration group.
  /// \c name  :- SManager registration name.
  /// \c view  :- View proxy.
  /// \c server:- server on which the proxy is created.
  /// \c parent:- QObject parent.
  pqContextView(const QString& type,
    const QString& group,
    const QString& name,
    vtkSMViewProxy* view,
    pqServer* server,
    QObject* parent=NULL);

  /// Creates a new instance of the QWidget subclass to be used to show this
  /// view. Default implementation creates a QVTKWidget.
  virtual QWidget* createWidget();

  /// Overridden to set up some default signal-slot connections.
  virtual void initialize();

  /// Listen for new selection events, and pass them back to ParaView
  void selectionChanged();
  class command;
  command* Command;

private:
  pqContextView(const pqContextView&); // Not implemented.
  void operator=(const pqContextView&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif
