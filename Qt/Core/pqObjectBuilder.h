/*=========================================================================

   Program: ParaView
   Module:    pqObjectBuilder.h

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

========================================================================*/
#ifndef __pqObjectBuilder_h
#define __pqObjectBuilder_h

#include <QObject>
#include "pqCoreExport.h"

class pqConsumerDisplay;
class pqDisplay;
class pqGenericViewModule;
class pqNameCount;
class pqPipelineSource;
class pqProxy;
class pqScalarBarDisplay;
class pqScalarsToColors;
class pqServer;
class vtkSMProxy;

/// pqObjectBuilder is loosely based on the \c Builder design pattern.
/// It is used to create as well as destroy complex objects such as 
/// views, displays, sources etc. Since most of the public API is 
/// virtual, it is possible for custom applications to subclass
/// pqObjectBuilder to provide their own implementation for creation/
/// destroying these objects. The application layer accesses the
/// ObjectBuilder through the pqApplicationCore singleton.
/// NOTE: pqObjectBuilder replaces the previously supported 
/// pqPipelineBuilder. Unlink, pqPipelineBuilder, this class
/// no longer deals with undo/redo stack. The application layer
/// components that use the ObjectBuilder are supposed to bother
/// about the undo stack i.e. call begin/end on the undo stack
/// before/after calling creation/destruction method on the ObjectBuilder.
class PQCORE_EXPORT pqObjectBuilder : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqObjectBuilder(QObject* parent=0);
  virtual ~pqObjectBuilder();

  /// Creates a source of the given server manager group (\c sm_group) and 
  /// name (\c sm_name) on the given \c server. On success, returns the
  /// pqPipelineSource for the created proxy.
  virtual pqPipelineSource* createSource(const QString& sm_group,
    const QString& sm_name, pqServer* server);

  /// Creates a filter with the given Server Manager group (\c sm_group) and
  /// name (\c sm_name). The filter's input is set as \c input. The filter
  /// is created on the same server as \c input.
  virtual pqPipelineSource* createFilter(const QString& sm_group,
    const QString& sm_name, pqPipelineSource* input);

  /// Creates a filter with the given Server Manager group (\c sm_group) and
  /// name (\c sm_name). If the filter accepts multiple inputs, all the inputs
  /// provided in the list are set as input, instead only the first one
  /// is set as the input. All inputs must be on the same server.
  virtual pqPipelineSource* createFilter(const QString& sm_group,
    const QString& sm_name, const QList<pqPipelineSource*>& inputs);

  /// Creates a custom filter with the given Server Manager name (\c sm_name)
  /// on the given \c server. If the custom filter takes an input,
  /// an option, input may be provided. It is required that \c input
  /// is on the same server as the custom filter to create.
  virtual pqPipelineSource* createCustomFilter(const QString& sm_name,
    pqServer* server, pqPipelineSource* input=0);

  /// Creates a reader of the given server manager group (\c sm_group) and 
  /// name (\c sm_name) on the given \c server. On success, returns the
  /// pqPipelineSource for the created proxy.
  virtual pqPipelineSource* createReader(const QString& sm_group,
    const QString& sm_name, const QString& filename, pqServer* server);

  /// Creates a new view module of the given type on the given server.
  virtual pqGenericViewModule* createView(const QString& type, pqServer* server);

  /// Destroys the view module. This destroys the view module
  /// as well as all the displays in the view module.
  virtual void destroy(pqGenericViewModule* view);

  /// Creates a display to show the data from the given \c source
  /// in the given \c view. This uses the pqDisplayPolicy provided
  /// by the application core to create the right kind of display
  /// for the (source, view) pair.
  virtual pqConsumerDisplay* createDataDisplay(pqPipelineSource* source,
    pqGenericViewModule* view);

  /// Destroys the data display. It will remove the display from any 
  /// view modules it is added to and then unregister it.
  virtual void destroy(pqDisplay* display);

  /// Creates a scalar bar display to show a lookup table
  /// in the view.
  virtual pqScalarBarDisplay* createScalarBarDisplay(
    pqScalarsToColors* lookupTable, pqGenericViewModule* view);

  /// Convenience method to create a proxy of any type on the given server.
  /// One can alternatively use the vtkSMProxyManager to create new proxies 
  /// directly. This method additionally set the connection ID on
  /// the new proxy. If reg_name is empty, then a new name is assigned.
  virtual vtkSMProxy* createProxy(const QString& sm_group, 
    const QString& sm_name, pqServer* server, 
    const QString& reg_group, const QString& reg_name=QString());

  /// Destroys a source/filter. Removing a source involves the following:
  // \li removing all displays belonging to the source,
  // \li breaking any input connections.
  // \li unregistering the source.
  // Note that the source must have no consumers, otherwise,
  // one cannot delete the source.
  virtual void destroy(pqPipelineSource* source);

  /// Convenience method, simply unregisters the Server Manager proxy 
  /// which the pqProxy represents.
  virtual void destroy(pqProxy* proxy);

  /// This method unregisters all proxies on the given server. 
  /// This is usually done in anticipate of a disconnect
  /// or starting afresh.
  virtual void destroyAllProxies(pqServer* server);

  /// Create a connection between a source and a sink. This method ensures
  /// that the UndoState is recoreded. Remember this "connection" is not a 
  /// server connection, but connection between two pipeline objects.
  void addConnection(pqPipelineSource* source, pqPipelineSource* filter);

  /// Removes a connection between a source and a sink. This method ensures that 
  /// the UndoState is recorded.
  void removeConnection(pqPipelineSource* source, pqPipelineSource* sink);

  /// This is a convenience method to return the name of the
  /// property on the proxy, if any, which can be used to set the filename.
  /// If no such property exists, this retruns a null string.
  QString getFileNamePropertyName(vtkSMProxy*)  const;

signals:
  /// Fired on successful completion of createSource().
  /// Remember that this signal is fired only when the creation of the object
  /// is requested by the GUI. It wont be triggered when the python client
  /// creates the source or when state is loaded or on undo/redo. 
  void sourceCreated(pqPipelineSource*);

  /// Fired on successful completion of createFilter().
  /// Remember that this signal is fired only when the creation of the object
  /// is requested by the GUI. It wont be triggered when the python client
  /// creates the source or when state is loaded or on undo/redo. 
  void filterCreated(pqPipelineSource*);

  /// Fired on successful completion of createReader().
  /// Remember that this signal is fired only when the creation of the object
  /// is requested by the GUI. It wont be triggered when the python client
  /// creates the source or when state is loaded or on undo/redo. 
  void readerCreated(pqPipelineSource*, const QString& filename);

  /// Fired on successful completion of createCustomFilter().
  /// Remember that this signal is fired only when the creation of the object
  /// is requested by the GUI. It wont be triggered when the python client
  /// creates the source or when state is loaded or on undo/redo. 
  void customFilterCreated(pqPipelineSource*);

  /// Fired on successful completion of createView().
  /// Remember that this signal is fired only when the creation of the object
  /// is requested by the GUI. It wont be triggered when the python client
  /// creates the source or when state is loaded or on undo/redo. 
  void viewCreated(pqGenericViewModule*);

  /// Fired on successful completion of createDataDisplay().
  /// Remember that this signal is fired only when the creation of the object
  /// is requested by the GUI. It wont be triggered when the python client
  /// creates the source or when state is loaded or on undo/redo. 
  void dataDisplayCreated(pqConsumerDisplay*);

  /// Fired on successful completion of createScalarBarDisplay().
  /// Remember that this signal is fired only when the creation of the object
  /// is requested by the GUI. It wont be triggered when the python client
  /// creates the source or when state is loaded or on undo/redo. 
  void scalarBarDisplayCreated(pqScalarBarDisplay*);

  /// Fired on successful completion of any method that creates a pqProxy
  /// or subclass including createScalarBarDisplay, createDataDisplay,
  /// createView, createCustomFilter, createFilter, createSource,
  /// createReader etc.
  void proxyCreated(pqProxy*);

  /// Fired on successful completion of createProxy().
  /// Remember that this signal is fired only when the creation of the object
  /// is requested by the GUI. It wont be triggered when the python client
  /// creates the source or when state is loaded or on undo/redo. 
  void proxyCreated(vtkSMProxy*);

  /// Fired when destroy(pqGenericViewModule*) is called. 
  /// This signal is fired before the process for destruction of the object 
  /// begins. Remember that this signal is fired only when the destruction of 
  /// the object is requested by the GUI. It wont be triggered when the python 
  /// client unregisters the source or when state is loaded or on undo/redo. 
  void destroying(pqGenericViewModule* view);
 
  /// Fired when destroy(pqDisplay*) is called. 
  /// This signal is fired before the process for destruction of the object 
  /// begins. Remember that this signal is fired only when the destruction of 
  /// the object is requested by the GUI. It wont be triggered when the python 
  /// client unregisters the source or when state is loaded or on undo/redo. 
  void destroying(pqDisplay* display);

  /// Fired when destroy(pqPipelineSource*) is called. 
  /// This signal is fired before the process for destruction of the object 
  /// begins. Remember that this signal is fired only when the destruction of 
  /// the object is requested by the GUI. It wont be triggered when the python 
  /// client unregisters the source or when state is loaded or on undo/redo. 
  void destroying(pqPipelineSource* source);

  /// Fired when destroy(pqProxy*) is called. 
  /// This signal is fired before the process for destruction of the object 
  /// begins. Remember that this signal is fired only when the destruction of 
  /// the object is requested by the GUI. It wont be triggered when the python 
  /// client unregisters the source or when state is loaded or on undo/redo. 
  void destroying(pqProxy* proxy);

protected:
  /// Create a proxy of the given type. If reg_name=QString(),
  /// a new name will be assigned to it.
  virtual vtkSMProxy* createProxyInternal(const QString& sm_group, 
    const QString& sm_name, pqServer* server, 
    const QString& reg_group, const QString& reg_name=QString());

  /// Unregisters a proxy.
  virtual void destroyProxyInternal(pqProxy* proxy);

  /// Used to create names for registering proxies.
  pqNameCount* NameGenerator;

private:
  pqObjectBuilder(const pqObjectBuilder&); // Not implemented.
  void operator=(const pqObjectBuilder&); // Not implemented.
};

#endif


