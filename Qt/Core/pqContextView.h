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
#ifndef pqContextView_h
#define pqContextView_h

#include "pqView.h"
#include "vtkType.h"

class vtkSMContextViewProxy;
class vtkContextView;
class vtkObject;
class vtkSelection;

/**
* pqContextView is an abstract base class for all charting views based on the
* VTK context charting library.
*/
class PQCORE_EXPORT pqContextView : public pqView
{
  Q_OBJECT
  typedef pqView Superclass;

public:
  ~pqContextView() override;

  /**
  * Returns the internal vtkContextView which provides the implementation for
  * the chart rendering.
  */
  virtual vtkContextView* getVTKContextView() const;

  /**
  * Returns the context view proxy associated with this object.
  */
  virtual vtkSMContextViewProxy* getContextViewProxy() const;

  /**
  * Returns true if selection can be done.
  */
  virtual bool supportsSelection() const;

  /**
  * Returns if this view module can support
  * image capture. Returns false by default. Subclassess must override
  * if that's not the case.
  */
  bool supportsCapture() const override { return true; }

  /**
  * set/get the selection action in the context view, defined
  * by vtkChart enumeration from SELECT to SELECT_POLYGON.
  */
  // Default is vtkChart::SELECT_RECTANGLE
  virtual void setSelectionAction(int selAction);
  virtual int selectionAction();

  /**
  * Resets the zoom level to 100%.
  */
  void resetDisplay() override;

protected slots:
  //@{
  /**
   * Slots called when vtkCommand::StartInteractionEvent and
   * vtkCommand::EndInteractionEvent are fired by vtkSMContextViewProxy. We use
   * these to mark an undo-set.
   */
  void startInteraction();
  void endInteraction();
  //@}

protected:
  /**
  * Constructor:
  * \c type  :- view type.
  * \c group :- SManager registration group.
  * \c name  :- SManager registration name.
  * \c view  :- View proxy.
  * \c server:- server on which the proxy is created.
  * \c parent:- QObject parent.
  */
  pqContextView(const QString& type, const QString& group, const QString& name,
    vtkSMViewProxy* view, pqServer* server, QObject* parent = NULL);

  /**
  * Creates a new instance of the QWidget subclass to be used to show this
  * view. This will create a pqQVTKWidget for the render window.
  */
  QWidget* createWidget() override;

  /**
  * Listen for new selection events, and pass them back to ParaView
  */
  virtual void selectionChanged();

  /**
  * Set selection to the view
  */
  virtual void setSelection(vtkSelection*);
  class command;
  command* Command;

private:
  Q_DISABLE_COPY(pqContextView)

  class pqInternal;
  pqInternal* Internal;
};

#endif
