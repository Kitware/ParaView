/*=========================================================================

   Program: ParaView
   Module:    pqPipelineSource.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

/// \file pqPipelineSource.h
/// \date 4/17/2006

#ifndef _pqPipelineSource_h
#define _pqPipelineSource_h

#include "pqCoreExport.h"
#include "pqProxy.h"

class pqPipelineDisplay;
class pqPipelineSourceInternal;
class pqRenderModule;
class vtkObject;


/// PQ representation for a vtkSMProxy that can be involved in a pipeline.
/// i.e that can have input and/or output. The public API is to observe
/// the object, changes to the pipeline structure are only through
/// protected function. These changes happen automatically as a reflection
/// of the SM state. 
class PQCORE_EXPORT pqPipelineSource : public pqProxy
{
  Q_OBJECT

public:
  pqPipelineSource(const QString& name, vtkSMProxy *proxy, pqServer* server,
    QObject* parent=NULL);
  virtual ~pqPipelineSource();

  // Get the number of consumers.
  int getNumberOfConsumers() const;

  // Get consumer at a particular index.
  pqPipelineSource *getConsumer(int index) const;

  // Get index for a consumer.
  int getConsumerIndexFor(pqPipelineSource *) const;

  // Check if the object exists in the consumer set.
  bool hasConsumer(pqPipelineSource *) const;

  /// Get the display at given index.
  pqPipelineDisplay *getDisplay(int index) const;

  // Get number of displays.
  int getDisplayCount() const; 

  // Get the display(if any) that is present in the \c renderModule. 
  // NOTE: In case more than one display exists for this source
  // added to the render module, this returns the first one.
  pqPipelineDisplay* getDisplay(pqRenderModule* renderModule) const;

  // Use this method to initialize the pqObject state using the
  // underlying vtkSMProxy. This needs to be done only once,
  // after the object has been created. 
  virtual void initialize() { };

  // This method updates all render modules to which all  
  // displays for this source belong, if force is true, it for an immediate 
  // render otherwise render on idle.
  void renderAllViews(bool force=false);

signals:
  /// fired when a connection is created between two pqPipelineSources.
  void connectionAdded(pqPipelineSource* in, pqPipelineSource* out);
  void preConnectionAdded(pqPipelineSource*, pqPipelineSource*);

  /// fired when a connection is broken between two pqPipelineSources.
  void connectionRemoved(pqPipelineSource* in, pqPipelineSource* out);
  void preConnectionRemoved(pqPipelineSource* in, pqPipelineSource* out);

  /// fired when a display is added.
  void displayAdded(pqPipelineSource* source, pqPipelineDisplay* display);

  /// fired when a display is removed.
  void displayRemoved(pqPipelineSource* source, pqPipelineDisplay* display);

protected slots:
  // process some change in the input property for the proxy--needed for subclass
  // pqPipelineFilter.
  virtual void inputChanged() { ; }

protected:
  friend class pqPipelineFilter;
  friend class pqPipelineDisplay;

  // called by pqPipelineFilter when the connections change.
  void removeConsumer(pqPipelineSource *);
  void addConsumer(pqPipelineSource*);

  // called by pqPipelineDisplay when the connections change.
  void addDisplay(pqPipelineDisplay*);
  void removeDisplay(pqPipelineDisplay*);
private:
  pqPipelineSourceInternal *Internal; ///< Stores the output connections.
};

#endif
