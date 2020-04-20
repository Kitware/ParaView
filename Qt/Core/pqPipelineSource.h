/*=========================================================================

   Program: ParaView
   Module:    pqPipelineSource.h

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

/**
* \file pqPipelineSource.h
* \date 4/17/2006
*/

#ifndef _pqPipelineSource_h
#define _pqPipelineSource_h

#include "pqCoreModule.h"
#include "pqProxy.h"

class pqDataRepresentation;
class pqOutputPort;
class pqPipelineSourceInternal;
class pqView;
class vtkCommand;
class vtkObject;
class vtkPVDataInformation;
class vtkSMSourceProxy;

/**
* PQ representation for a vtkSMProxy that can be involved in a pipeline.
* i.e that can have input and/or output. The public API is to observe
* the object, changes to the pipeline structure are only through
* protected function. These changes happen automatically as a reflection
* of the SM state.
*/
class PQCORE_EXPORT pqPipelineSource : public pqProxy
{
  Q_OBJECT
  typedef pqProxy Superclass;

public:
  pqPipelineSource(
    const QString& name, vtkSMProxy* proxy, pqServer* server, QObject* parent = NULL);
  ~pqPipelineSource() override;

  /**
  * A source may have multiple output ports. This method returns the number of
  * output ports supported by this source.
  */
  int getNumberOfOutputPorts() const;

  /**
  * Returns the pqOutputPort for the given output port.
  */
  pqOutputPort* getOutputPort(int outputport) const;

  /**
  * Returns the pqOutputPort given the name of the port.
  */
  pqOutputPort* getOutputPort(const QString& portName) const;

  /**
  * Returns all the output ports.
  */
  QList<pqOutputPort*> getOutputPorts() const;

  /**
  * Returns the number of consumers connected to the given output port.
  */
  int getNumberOfConsumers(int outputport) const;

  /**
  * Get the number of consumers connected to output port 0.
  * Equivalent to calling getNumberOfConsumers(0);
  */
  int getNumberOfConsumers() const { return this->getNumberOfConsumers(0); }

  /**
  * Get the consumer at a particulat index on a given output port.
  */
  pqPipelineSource* getConsumer(int outputport, int index) const;

  /**
  * Get consumer at a particular index on output port 0.
  * Equivalent to calling getConsumer(0, index);
  */
  pqPipelineSource* getConsumer(int index) const { return this->getConsumer(0, index); }

  /**
  * Returns a list of consumers for all output ports.
  */
  QList<pqPipelineSource*> getAllConsumers() const;

  /**
  * Returns a list of representations for this source in the given view.
  * If view == NULL, returns all representations of this source.
  */
  QList<pqDataRepresentation*> getRepresentations(int outputport, pqView* view) const;
  QList<pqDataRepresentation*> getRepresentations(pqView* view) const
  {
    return this->getRepresentations(0, view);
  }

  /**
  * Returns the first representation for this source in the given view.
  * If view is NULL, returns 0.
  */
  pqDataRepresentation* getRepresentation(int outputport, pqView* view) const;
  pqDataRepresentation* getRepresentation(pqView* view) const
  {
    return this->getRepresentation(0, view);
  }

  /**
  * Returns a list of render modules in which this source
  * has representations added (the representations may not be visible).
  */
  QList<pqView*> getViews() const;

  /**
  * This method updates all render modules to which all
  * representations for this source belong, if force is true, it for an
  * immediate render otherwise render on idle.
  */
  void renderAllViews(bool force = false);

  /**
  * Update the pipeline with the current time
  */
  void updatePipeline();

  /**
  * Return the vtkSMSourceProxy instance corresponding to this
  * pqPipelineSource.
  */
  vtkSMSourceProxy* getSourceProxy();

Q_SIGNALS:
  /**
  * fired when a connection is created between two pqPipelineSources.
  */
  void connectionAdded(pqPipelineSource* source, pqPipelineSource* consumer, int srcOutputPort);
  void preConnectionAdded(pqPipelineSource* source, pqPipelineSource* consumer, int srcOutputPort);

  /**
  * fired when a connection is broken between two pqPipelineSources.
  */
  void connectionRemoved(pqPipelineSource* source, pqPipelineSource* consumer, int srcOutputPort);
  void preConnectionRemoved(
    pqPipelineSource* source, pqPipelineSource* consumer, int srcOutputPort);

  /**
  * fired when a representation is added.
  */
  void representationAdded(pqPipelineSource* source, pqDataRepresentation* repr, int srcOutputPort);

  /**
  * fired when a representation is removed.
  */
  void representationRemoved(
    pqPipelineSource* source, pqDataRepresentation* repr, int srcOutputPort);

  /**
  * Fired when the visbility of a representation for the source changes.
  * Also fired when representationAdded or representationRemoved is fired
  * since that too implies change in source visibility.
  */
  void visibilityChanged(pqPipelineSource* source, pqDataRepresentation* repr);

  /**
  * Fired right after the underlying algorithm updates (executes).
  * This can be used to update data information and other similar tasks.
  */
  void dataUpdated(pqPipelineSource* source);

  /**
   * Fired when the selection on a port has changed.
   */
  void selectionChanged(pqOutputPort* port);

protected Q_SLOTS:
  /**
  * Called when the visibility of any representation for this source changes.
  */
  void onRepresentationVisibilityChanged();

private Q_SLOTS:
  /**
  * Slots called when corresponding signals are fired from pqOutputPort.
  * These slots simply fire the appropriate signals.
  */
  void prePortConnectionAdded(pqOutputPort* op, pqPipelineSource* cons);
  void portConnectionAdded(pqOutputPort* op, pqPipelineSource* cons);
  void prePortConnectionRemoved(pqOutputPort* op, pqPipelineSource* cons);
  void portConnectionRemoved(pqOutputPort* op, pqPipelineSource* cons);
  void portRepresentationAdded(pqOutputPort* op, pqDataRepresentation* cons);
  void portRepresentationRemoved(pqOutputPort* op, pqDataRepresentation* cons);
  void portVisibilityChanged(pqOutputPort* op, pqDataRepresentation* cons);
  void dataUpdated();

  void onSelectionChanged(
    vtkObject*, unsigned long, void* client_data, void* call_data, vtkCommand*);

protected:
  friend class pqPipelineFilter;

  /**
  * called by pqPipelineFilter when the connections change.
  */
  void removeConsumer(int outputport, pqPipelineSource*);
  void addConsumer(int outputport, pqPipelineSource*);

private:
  pqPipelineSourceInternal* Internal; ///< Stores the output connections.
};

#endif
