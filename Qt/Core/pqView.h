/*=========================================================================

   Program: ParaView
   Module:    pqView.h

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
#ifndef pqView_h
#define pqView_h

#include "pqProxy.h"
#include <QSize> // needed for QSize.

class pqOutputPort;
class pqPipelineSource;
class pqRepresentation;
class pqServer;
class pqUndoStack;
class pqViewInternal;
class QWidget;
class vtkSMSourceProxy;
class vtkSMViewProxy;
class vtkView;

/**
* This is a PQ abstraction of a generic view module. Subclasses can be
* specific for different types of view such as render view, histogram view
* etc.
*/
class PQCORE_EXPORT pqView : public pqProxy
{
  Q_OBJECT
  typedef pqProxy Superclass;

public:
  enum SelectionModifier
  {
    PV_SELECTION_DEFAULT = 0,
    PV_SELECTION_ADDITION,
    PV_SELECTION_SUBTRACTION,
    PV_SELECTION_TOGGLE
  };

  ~pqView() override;

  /**
  * Returns the internal render Module proxy associated with this object.
  */
  vtkSMViewProxy* getViewProxy() const;

  /**
  * Return the client-side vtkView encapsulated by this view (if any),
  * or return NULL.
  */
  virtual vtkView* getClientSideView() const;

  /**
  * Return a widget associated with this view. Every view in ParaView Qt
  * application must be able to render itself in a QWidget. The first time
  * this method is called, this will call pqView::createWidget(). Subclasses
  * createWidget() to create a QWidget for the view.
  * This may return NULL if the view doesn't have QWidget associated with it
  * (which is rare, if impossible) or the QWidget was previously created but
  * since has been destroyed due to Qt cleanup.
  */
  QWidget* widget();

  /**
  * Returns if this view module can support
  * undo/redo. Returns false by default. Subclassess must override
  * if that's not the case.
  * View modules that support undo must fire
  * all undo related signals defined by this class.
  */
  virtual bool supportsUndo() const { return false; }

  /**
  * Returns if this view module can support
  * image capture. Returns false by default. Subclassess must override
  * if that's not the case.
  */
  virtual bool supportsCapture() const { return false; }

  /**
  * Returns the type of this view module.
  */
  QString getViewType() const { return this->ViewType; }

public slots:
  /**
  * Request a StillRender on idle. Multiple calls are collapsed into one.
  */
  virtual void render();

  /**
  * Forces an immediate render.
  */
  virtual void forceRender();

  /**
  * Cancels any pending renders.
  */
  void cancelPendingRenders();

  /**
  * Called to undo interaction.
  * View modules supporting interaction undo must override this method.
  */
  virtual void undo() {}

  /**
  * Called to redo interaction.
  * View modules supporting interaction undo must override this method.
  */
  virtual void redo() {}

  /**
  * Called to reset the view's display.
  * For example, reset the camera or zoom level.
  * The default implementation does nothing, but subclasses may override.
  */
  virtual void resetDisplay() {}

public:
  /**
  * Returns true if undo can be done.
  */
  virtual bool canUndo() const { return false; }

  /**
  * Returns true if redo can be done.
  */
  virtual bool canRedo() const { return false; }

  /**
  * Returns the current size of the rendering context.
  * Default implementation returns the client size of the widget. Subclasses
  * may override to change this behavior.
  */
  virtual QSize getSize();

  /**
  * This method checks if the representation is shown in this view.
  */
  bool hasRepresentation(pqRepresentation* repr) const;

  /**
  * Returns the number representations in the view.
  */
  int getNumberOfRepresentations() const;

  // Returns the number of representations currently visible in the view.
  int getNumberOfVisibleRepresentations() const;
  int getNumberOfVisibleDataRepresentations() const;

  /**
  * Returns the representation for the specified index where
  * (index < getNumberOfRepresentations()).
  */
  pqRepresentation* getRepresentation(int index) const;

  /**
  * Returns a list of representations in this view.
  */
  QList<pqRepresentation*> getRepresentations() const;

  /**
  * This method returns is any pqPipelineSource can be displayed in this
  * view. NOTE: This is no longer virtual. Simply forwards to
  * vtkSMViewProxy::CanDisplayData().
  */
  bool canDisplay(pqOutputPort* opPort) const;

  /**
  * Called when a selection is made, passing in the mode as the sole
  * argument.
  */
  virtual void emitSelectionSignals(bool frustum);

signals:
  /**
  * Fired when the vtkSMViewProxy fires the vtkCommand::UpdateDataEvent
  * The view proxy fires this event at the end of vtkSMViewProxy::Update()
  * when the update executes, not just when the method is called.
  */
  void updateDataEvent();

  /**
  * Fired after a representation has been added to this view.
  */
  void representationAdded(pqRepresentation*);

  /**
  * Fired after a representation has been removed from this view.
  */
  void representationRemoved(pqRepresentation*);

  /**
  * Fired when the render module fires a vtkCommand::StartEvent
  * signalling the beginning of rendering. Subclasses must fire
  * these signals at appropriate times.
  */
  void beginRender();

  /**
  * Fired when the render module fires a vtkCommand::EndEvent
  * signalling the end of rendering.
  * Subclasses must fire these signals at appropriate times.
  */
  void endRender();

  /**
  * Fired when any representation visibility changes.
  */
  void representationVisibilityChanged(pqRepresentation* repr, bool visible);

  /**
  * Fired when interaction undo stack status changes.
  */
  void canUndoChanged(bool);

  /**
  * Fired when interaction undo stack status changes.
  */
  void canRedoChanged(bool);

  /**
  * Fired when a selection is made in this view.
  * \c opport is the output port for the source that got selected.
  *    the selection input on the source proxy for the opport must already
  *    have been initialized to a selection source.
  */
  void selected(pqOutputPort* opport);

  /**
  * Fired when the selection changes.
  * \c frustum is a boolean indicating whether the selection is frustum-based.
  */
  void selectionModeChanged(bool frustum);

  /**
  * Fired when a port is picked.
  * \c opport is the port that got picked.
  */
  void picked(pqOutputPort* opport);

  /**
  * Fired before doing any actions that may result in progress events that
  * must be reported by the client.
  */
  void beginProgress();

  /**
  * Fired after performing any actions that may result in progress events.
  * Must match beginProgress() calls.
  */
  void endProgress();

  /**
  * Fired to notify the current execution progress. This will be generally
  * have any effect only if beginProgress() has been fired before firing this
  * signal.
  */
  void progress(const QString& message, int percent_progress);

  /**
  * Fired when UseMultipleRepresentationSelection is set to on and
  * selection on multiple representations is made in this view.
  * \c opports is a list of opport, and opport is the output port for
  *    the source that got selected. the selection input on the source
  *    proxy for the opport must already have been
  *    initialized to a selection source.
  */
  void multipleSelected(QList<pqOutputPort*> opports);

private slots:
  /**
  * Called when the "Representations" property changes.
  */
  void onRepresentationsChanged();

  /**
  * Called when the representation fires visibilityChanged() signal.
  */
  void onRepresentationVisibilityChanged(bool);

  /**
  * Called when a new representation is registered by the ServerManagerModel.
  * We check if the representation belongs to this view.
  */
  void representationCreated(pqRepresentation* repr);

  /**
  * This is called when the timer in render() times out. We test if the
  * current moment is a reasonable moment to render and if so, call
  * forceRender(). If there are any pending progress events, then we treat the
  * moment as not a "reasonable moment" to render and defer the render again.
  */
  void tryRender();

  /**
  * These slots help use avoid the undo stack being modified during rendering.
  * A few views (e.g. vtkSMContextViewProxy) may change some of its properties
  * during a render. We don't want those to get captured in the undo/redo
  * stack.
  */
  void onBeginRender();
  void onEndRender();

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
  pqView(const QString& type, const QString& group, const QString& name, vtkSMViewProxy* view,
    pqServer* server, QObject* parent = NULL);

  /**
  * Use this method to initialize the pqObject state using the
  * underlying vtkSMProxy. This needs to be done only once,
  * after the object has been created.
  * Overridden to update the list of representations currently available.
  */
  void initialize() override;

  /**
  * Subclasses must override this method to create a widget for the view.
  */
  virtual QWidget* createWidget() = 0;

private:
  Q_DISABLE_COPY(pqView)

  pqViewInternal* Internal;
  QString ViewType;
};

#endif
