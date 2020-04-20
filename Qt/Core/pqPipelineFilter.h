/*=========================================================================

   Program: ParaView
   Module:    pqPipelineFilter.h

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
* \file pqPipelineFilter.h
* \date 4/17/2006
*/

#ifndef _pqPipelineFilter_h
#define _pqPipelineFilter_h

#include "pqCoreModule.h"
#include "pqPipelineSource.h"
#include <QMap>

class pqOutputPort;

class PQCORE_EXPORT pqPipelineFilter : public pqPipelineSource
{
  Q_OBJECT
  typedef pqPipelineSource Superclass;

public:
  pqPipelineFilter(QString name, vtkSMProxy* proxy, pqServer* server, QObject* parent = NULL);
  ~pqPipelineFilter() override;

  /**
  * Returns the inputs ports on any proxy.
  */
  static QList<const char*> getInputPorts(vtkSMProxy*);

  /**
  * Returns the required inputs ports on any proxy.
  * This is generally same as getInputPorts() except if the property has a
  * "hint" saying it's optional.
  */
  static QList<const char*> getRequiredInputPorts(vtkSMProxy*);

  /**
  * Returns the number of input ports available on this filter.
  */
  int getNumberOfInputPorts() const;

  /**
  * Returns the name of the input port at a given index.
  */
  QString getInputPortName(int index) const;

  /**
  * Returns the number of input proxies connected to given named input port.
  */
  int getNumberOfInputs(const QString& portname) const;

  /**
  * Returns the input proxies connected to the given named input port.
  */
  QList<pqOutputPort*> getInputs(const QString& portname) const;

  /**
  * Returns inputs connected to all input ports connections.
  */
  QList<pqOutputPort*> getAllInputs() const;

  /**
  * Returns a map of input port name to the list of output ports that are set
  * as the input to that port.
  */
  QMap<QString, QList<pqOutputPort*> > getNamedInputs() const;

  /**
  * Returns a pair (input, output port) at the given index on the given named
  * input port.
  */
  pqOutputPort* getInput(const QString& portname, int index) const;

  /**
  * Get number of inputs.
  */
  int getInputCount() const { return this->getNumberOfInputs(this->getInputPortName(0)); }

  /**
  * Get a list of all inputs.
  */
  QList<pqOutputPort*> getInputs() const { return this->getInputs(this->getInputPortName(0)); }

  /**
  * Get input at given index.
  */
  pqPipelineSource* getInput(int index) const;

  /**
  * Get first available input, any port, any index.
  * Return NULL if none are available.
  */
  pqOutputPort* getAnyInput() const;

  /**
  * "Replace input" is a hint given to the GUI to turn off input visibility
  * when the filter is created.
  * Returns if this proxy replaces input on creation.
  * This checks the "Hints" for the proxy, if any. If a `<Visibility>`
  * element is present with replace_input="0", then this method
  * returns false, otherwise true.
  */
  int replaceInput() const;

Q_SIGNALS:
  /**
  * fired whenever an input connection changes.
  */
  void producerChanged(const QString& inputportname);

protected Q_SLOTS:
  /**
  * process some change in the input property for the proxy.
  */
  void inputChanged(vtkObject*, unsigned long, void* client_data);

protected:
  /**
  * Use this method to initialize the pqObject state using the
  * underlying vtkSMProxy. This needs to be done only once,
  * after the object has been created.
  */
  void initialize() override;

  /**
  * Called when a input property changes. \c portname is the name of the input
  * port represented by the property.
  */
  void inputChanged(const QString& portname);

private:
  class pqInternal;
  pqInternal* Internal; ///< Stores the input connections.
};

#endif
