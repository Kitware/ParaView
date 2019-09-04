/*=========================================================================

   Program: ParaView
   Module:    pqOutputPort.h

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

========================================================================*/
#ifndef pqOutputPort_h
#define pqOutputPort_h

#include "pqCoreModule.h"
#include "pqServerManagerModelItem.h"

class pqDataRepresentation;
class pqPipelineSource;
class pqServer;
class pqView;
class vtkPVDataInformation;
class vtkPVTemporalDataInformation;
class vtkSMOutputPort;
class vtkSMSourceProxy;

/**
* pqOutputPort is a server manager model item for an output port of any
* pqPipelineSource item. This makes it possible to refer to a particular
* output port in the server manager model. The pqPipelineSource keeps
* references to all its output ports. The only way to access pqOutputPort
* items is through the pqPipelineSource. One can obtain the pqPipelineSource
* item from a pqOutputPort using getSource().
* Once the outputs can be named, we will change this class to use output port
* names instead of numbers.
*/
class PQCORE_EXPORT pqOutputPort : public pqServerManagerModelItem
{
  Q_OBJECT
  typedef pqServerManagerModelItem Superclass;

public:
  pqOutputPort(pqPipelineSource* source, int portno);
  ~pqOutputPort() override;

  /**
  * Returns the vtkSMOutputPort proxy for this port.
  */
  vtkSMOutputPort* getOutputPortProxy() const;

  /**
  * Returns the pqPipelineSource whose output port this is.
  */
  pqPipelineSource* getSource() const { return this->Source; }

  /**
  * Return the vtkSMSourceProxy for the source.
  */
  vtkSMSourceProxy* getSourceProxy() const;

  /**
  * Returns the server connection on which this output port exists.
  */
  pqServer* getServer() const;

  /**
  * Returns the port number of the output port which this item represents.
  */
  int getPortNumber() const { return this->PortNumber; }

  /**
  * Returns the port name for this output port.
  */
  QString getPortName() const;

  /**
  * Returns the number of consumers connected to this output port.
  */
  int getNumberOfConsumers() const;

  /**
  * Get the consumer at a particular index on this output port.
  */
  pqPipelineSource* getConsumer(int index) const;

  /**
  * Returns a list of consumers.
  */
  QList<pqPipelineSource*> getConsumers() const;

  /**
  * Returns a list of representations for this output port in the given view.
  * If view == NULL, returns all representations of this port.
  */
  QList<pqDataRepresentation*> getRepresentations(pqView* view) const;

  /**
  * Returns the first representation for this output port in the given view.
  * If view is NULL, returns 0.
  */
  pqDataRepresentation* getRepresentation(pqView* view) const;

  /**
  * Returns a list of render modules in which this output port
  * has representations added (the representations may not be visible).
  */
  QList<pqView*> getViews() const;

  /**
  * Returns the current data information at this output port.
  * This does not update the pipeline, it simply returns the data information
  * for data currently present on the output port on the server.
  */
  vtkPVDataInformation* getDataInformation() const;

  /**
  * Collects data information over time. This can potentially be a very slow
  * process, so use with caution.
  */
  vtkPVTemporalDataInformation* getTemporalDataInformation();

  /**
   * Returns the current data information for the selected data from this
   * output port.
   *
   * \c es_port is the output port from the internal vtkPVExtractSelection
   * proxy.
   *
   */
  vtkPVDataInformation* getSelectedDataInformation(int es_port = 0) const;

  /**
  * Returns the class name of the output data.
  */
  const char* getDataClassName() const;

  /**
  * Calls vtkSMSourceProxy::GetSelectionInput() on the underlying source
  * proxy.
  */
  vtkSMSourceProxy* getSelectionInput();

  /**
  * Calls vtkSMSourceProxy::GetSelectionInputPort() on the underlying source
  * proxy.
  */
  unsigned int getSelectionInputPort();

  /**
  * Set the selection input.
  */
  void setSelectionInput(vtkSMSourceProxy* src, int port);

public slots:
  /**
  * This method updates all render modules to which all
  * representations for this source belong, if force is true, it for an
  * immediate render otherwise render on idle.
  */
  void renderAllViews(bool force = false);

signals:
  /**
  * Fired when a connection is added between this output port and a consumer.
  */
  void connectionAdded(pqOutputPort* port, pqPipelineSource* consumer);
  void preConnectionAdded(pqOutputPort* port, pqPipelineSource* consumer);

  /**
  * Fired when a connection is removed between this output port and a consumer.
  */
  void connectionRemoved(pqOutputPort* port, pqPipelineSource* consumer);
  void preConnectionRemoved(pqOutputPort* port, pqPipelineSource* consumer);

  /**
  * fired when a representation is added.
  */
  void representationAdded(pqOutputPort* source, pqDataRepresentation* repr);

  /**
  * fired when a representation is removed.
  */
  void representationRemoved(pqOutputPort* source, pqDataRepresentation* repr);

  /**
  * Fired when the visbility of a representation for the source changes.
  * Also fired when representationAdded or representationRemoved is fired
  * since that too implies change in source visibility.
  */
  void visibilityChanged(pqOutputPort* source, pqDataRepresentation* repr);

protected slots:
  void onRepresentationVisibilityChanged();

protected:
  friend class pqPipelineFilter;
  friend class pqDataRepresentation;

  /**
  * called by pqPipelineSource when the connections change.
  */
  void removeConsumer(pqPipelineSource*);
  void addConsumer(pqPipelineSource*);

  /**
  * Called by pqPipelineSource when the representations are added/removed.
  */
  void addRepresentation(pqDataRepresentation*);
  void removeRepresentation(pqDataRepresentation*);

  pqPipelineSource* Source;
  int PortNumber;

private:
  Q_DISABLE_COPY(pqOutputPort)

  class pqInternal;
  pqInternal* Internal;
};

#endif
