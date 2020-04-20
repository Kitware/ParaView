/*=========================================================================

   Program: ParaView
   Module:    pqObjectBuilder.h

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
#ifndef pqObjectBuilder_h
#define pqObjectBuilder_h

#include "pqCoreModule.h"
#include "vtkSetGet.h" // for VTK_LEGACY
#include <QMap>
#include <QObject>
#include <QVariant>

class pqAnimationCue;
class pqDataRepresentation;
class pqOutputPort;
class pqPipelineSource;
class pqProxy;
class pqRepresentation;
class pqScalarBarRepresentation;
class pqScalarsToColors;
class pqServer;
class pqServerResource;
class pqView;
class vtkSMProxy;

/**
* pqObjectBuilder is loosely based on the \c Builder design pattern.
* It is used to create as well as destroy complex objects such as
* views, displays, sources etc. Since most of the public API is
* virtual, it is possible for custom applications to subclass
* pqObjectBuilder to provide their own implementation for creation/
* destroying these objects. The application layer accesses the
* ObjectBuilder through the pqApplicationCore singleton.
* NOTE: pqObjectBuilder replaces the previously supported
* pqPipelineBuilder. Unlink, pqPipelineBuilder, this class
* no longer deals with undo/redo stack. The application layer
* components that use the ObjectBuilder are supposed to bother
* about the undo stack i.e. call begin/end on the undo stack
* before/after calling creation/destruction method on the ObjectBuilder.
*/
class PQCORE_EXPORT pqObjectBuilder : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqObjectBuilder(QObject* parent = 0);
  ~pqObjectBuilder() override;

  /**
  * Create a server connection give a server resource.
  * By default, this method does not create a new connection if one already
  * exists. Also it disconnects from any existing server connection before
  * connecting to a new one. This behavior can be changed by setting
  * MultipleConnectionsSupport to true. In that case
  * this will always try to connect the server using the details specified in
  * the resource irrespective if the server is already connected or any other
  * server connections exists and will wait for timeout seconds for direct connection.
  * 0 means no retry, -1 means infinite retries.
  * Calling this method while waiting for a previous server connection to be
  * established raises errors.
  */
  pqServer* createServer(const pqServerResource& resource, int connectionTimeout = 60);

  /**
  * Destroy a server connection
  */
  void removeServer(pqServer* server);

  /**
  * Resets the server by destroying the old pqServer instance and creating
  * a new one while keeping the same underlying session alive.
  */
  pqServer* resetServer(pqServer* server);

  /**
  * Creates a source of the given server manager group (\c sm_group) and
  * name (\c sm_name) on the given \c server. On success, returns the
  * pqPipelineSource for the created proxy.
  */
  virtual pqPipelineSource* createSource(
    const QString& sm_group, const QString& sm_name, pqServer* server);

  /**
  * Creates a filter with the given Server Manager group (\c sm_group) and
  * name (\c sm_name). If the filter accepts multiple inputs, all the inputs
  * provided in the list are set as input, instead only the first one
  * is set as the input. All inputs must be on the same server.
  */
  virtual pqPipelineSource* createFilter(const QString& group, const QString& name,
    QMap<QString, QList<pqOutputPort*> > namedInputs, pqServer* server);

  /**
  * Convenience method that takes a single input source.
  */
  virtual pqPipelineSource* createFilter(
    const QString& group, const QString& name, pqPipelineSource* input, int output_port = 0);

  /**
  * Creates a reader of the given server manager group (\c sm_group) and
  * name (\c sm_name) on the given \c server. On success, returns the
  * pqPipelineSource for the created proxy.
  */
  virtual pqPipelineSource* createReader(
    const QString& sm_group, const QString& sm_name, const QStringList& files, pqServer* server);

  /**
  * Creates a new view module of the given type on the given server.
  */
  virtual pqView* createView(const QString& type, pqServer* server);

  /**
   * Deprecated in ParaView 5.7. `detachedFromLayout` argument is not longer
   * applicable. All views are now created *detached* by default.
   */
  VTK_LEGACY(pqView* createView(const QString& type, pqServer* server, bool detachedFromLayout));

  /**
  * Destroys the view module. This destroys the view module
  * as well as all the displays in the view module.
  */
  virtual void destroy(pqView* view);

  /**
   * Assigns the view to the layout. If layout is nullptr, this method will
   * use an arbitrary layout on the same server as the view, if any, or create a
   * new layout and assign the view to it.
   */
  virtual void addToLayout(pqView* view, pqProxy* layout = nullptr);

  /**
  * Creates a representation to show the data from the given output port of a
  * source in the given \c view.
  */
  virtual pqDataRepresentation* createDataRepresentation(
    pqOutputPort* source, pqView* view, const QString& representationType = "");

  /**
  * Convenience method to create a proxy of any type on the given server.
  * One can alternatively use the vtkSMProxyManager to create new proxies
  * directly. This method additionally set the connection ID on
  * the new proxy. If reg_name is empty, then a new name is assigned.
  */
  virtual vtkSMProxy* createProxy(
    const QString& sm_group, const QString& sm_name, pqServer* server, const QString& reg_group);

  /**
  * Destroys the data display. It will remove the display from any
  * view modules it is added to and then unregister it.
  */
  virtual void destroy(pqRepresentation* repr);

  /**
  * Destroys a source/filter. Removing a source involves the following:
  */
  // \li removing all displays belonging to the source,
  // \li breaking any input connections.
  // \li unregistering the source.
  // Note that the source must have no consumers, otherwise,
  // one cannot delete the source.
  virtual void destroy(pqPipelineSource* source);

  /**
  * Destroys an animation cue and all keyframe objects, if any in that cue.
  */
  virtual void destroy(pqAnimationCue* cue);

  /**
  * Convenience method, simply unregisters the Server Manager proxy
  * which the pqProxy represents.
  */
  virtual void destroy(pqProxy* proxy);

  /**
  * Destroy all sources/filters on a server.
  */
  virtual void destroySources(pqServer* server = 0);

  /**
  * Destroy all lookup tables and scalar bars associated with them.
  */
  virtual void destroyLookupTables(pqServer* server = 0);

  /**
  * Destroys all proxies that are involved in pipelines i.e. simply calls
  * destroySources(), destroyLookupTables().
  */
  virtual void destroyPipelineProxies(pqServer* server = 0);

  /**
  * This method unregisters all proxies on the given server.
  * This is usually done in anticipate of a disconnect
  * or starting afresh.
  */
  virtual void destroyAllProxies(pqServer* server);

  /**
  * This is a convenience method to return the name of the
  * property on the proxy, if any, which can be used to set the filename.
  * If no such property exists, this returns a null string.
  * If there are more than 1 properties with FileListDomain, then it looks at
  * the Hints for the proxy for the XML of the form
  * `<DefaultFileNameProperty name="<propertyname>" />` and uses that property
  * otherwise simply returns the first one encountered.
  */
  static QString getFileNamePropertyName(vtkSMProxy*);

  /**
  * Returns true while pqObjectBuilder is in createServer() call.
  */
  bool waitingForConnection() const
  {
    return this->ForceWaitingForConnection ? true : this->WaitingForConnection;
  }

  /**
  * Set ForceWaitingForConnection
  * When set to true, WaitingForConnection() will always return true
  * When set to false, no effect.
  * Returns the previous state
  */
  bool forceWaitingForConnection(bool force);

public Q_SLOTS:
  /**
  * Closes any open connections for reverse-connection.
  */
  void abortPendingConnections();

Q_SIGNALS:

  /**
  * Emitted after a new server connection is created
  */
  void finishedAddingServer(pqServer* server);

  /**
  * Fired on successful completion of createSource().
  * Remember that this signal is fired only when the creation of the object
  * is requested by the GUI. It wont be triggered when the python client
  * creates the source or when state is loaded or on undo/redo.
  */
  void sourceCreated(pqPipelineSource*);

  /**
  * Fired on successful completion of createFilter().
  * Remember that this signal is fired only when the creation of the object
  * is requested by the GUI. It wont be triggered when the python client
  * creates the source or when state is loaded or on undo/redo.
  */
  void filterCreated(pqPipelineSource*);

  /**
  * Fired on successful completion of createReader().
  * Remember that this signal is fired only when the creation of the object
  * is requested by the GUI. It wont be triggered when the python client
  * creates the source or when state is loaded or on undo/redo.
  */
  void readerCreated(pqPipelineSource*, const QString& filename);
  void readerCreated(pqPipelineSource*, const QStringList& filename);

  /**
  * fired before attempting to create a view.
  */
  void aboutToCreateView(pqServer* server);

  /**
  * Fired on successful completion of createView().
  * Remember that this signal is fired only when the creation of the object
  * is requested by the GUI. It wont be triggered when the python client
  * creates the source or when state is loaded or on undo/redo.
  */
  void viewCreated(pqView*);

  /**
  * Fired on successful completion of createDataRepresentation().
  * Remember that this signal is fired only when the creation of the object
  * is requested by the GUI. It wont be triggered when the python client
  * creates the source or when state is loaded or on undo/redo.
  */
  void dataRepresentationCreated(pqDataRepresentation*);

  /**
  * Fired on successful completion of any method that creates a pqProxy
  * or subclass including createDataRepresentation,
  * createView, createFilter, createSource,
  * createReader etc.
  */
  void proxyCreated(pqProxy*);

  /**
  * Fired on successful completion of createProxy().
  * Remember that this signal is fired only when the creation of the object
  * is requested by the GUI. It wont be triggered when the python client
  * creates the source or when state is loaded or on undo/redo.
  */
  void proxyCreated(vtkSMProxy*);

  /**
  * Fired when destroy(pqView*) is called.
  * This signal is fired before the process for destruction of the object
  * begins. Remember that this signal is fired only when the destruction of
  * the object is requested by the GUI. It wont be triggered when the python
  * client unregisters the source or when state is loaded or on undo/redo.
  */
  void destroying(pqView* view);

  /**
  * Fired when destroy(pqRepresentation*) is called.
  * This signal is fired before the process for destruction of the object
  * begins. Remember that this signal is fired only when the destruction of
  * the object is requested by the GUI. It wont be triggered when the python
  * client unregisters the source or when state is loaded or on undo/redo.
  */
  void destroying(pqRepresentation* display);

  /**
  * Fired when destroy(pqPipelineSource*) is called.
  * This signal is fired before the process for destruction of the object
  * begins. Remember that this signal is fired only when the destruction of
  * the object is requested by the GUI. It wont be triggered when the python
  * client unregisters the source or when state is loaded or on undo/redo.
  */
  void destroying(pqPipelineSource* source);

  /**
  * Fired when destroy(pqProxy*) is called.
  * This signal is fired before the process for destruction of the object
  * begins. Remember that this signal is fired only when the destruction of
  * the object is requested by the GUI. It wont be triggered when the python
  * client unregisters the source or when state is loaded or on undo/redo.
  */
  void destroying(pqProxy* proxy);

protected:
  /**
  * Unregisters a proxy.
  */
  virtual void destroyProxyInternal(pqProxy* proxy);

private:
  Q_DISABLE_COPY(pqObjectBuilder)

  bool ForceWaitingForConnection;
  bool WaitingForConnection;
};

#endif
