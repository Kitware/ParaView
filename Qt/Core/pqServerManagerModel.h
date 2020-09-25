/*=========================================================================

   Program: ParaView
   Module:    pqServerManagerModel.h

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
#ifndef pqServerManagerModel_h
#define pqServerManagerModel_h

#include "pqCoreModule.h"
#include "vtkType.h" // for vtkIdType.
#include <QList>
#include <QObject>

class pqExtractor;
class pqPipelineSource;
class pqProxy;
class pqRepresentation;
class pqServer;
class pqServerManagerModelItem;
class pqServerManagerObserver;
class pqServerResource;
class pqView;
class vtkPVXMLElement;
class vtkSession;
class vtkSMProxy;
class vtkSMProxyLocator;
class vtkSMSession;

class pqServerManagerModel;

template <class T>
inline QList<T> pqFindItems(const pqServerManagerModel* const model);
template <class T>
inline QList<T> pqFindItems(const pqServerManagerModel* const model, pqServer* server);
template <class T>
inline T pqFindItem(const pqServerManagerModel* const model, const QString& name);
template <class T>
inline T pqFindItem(const pqServerManagerModel* const model, vtkSMProxy* proxy);
template <class T>
inline T pqFindItem(const pqServerManagerModel* const model, vtkTypeUInt32 id);
template <class T>
inline int pqGetNumberOfItems(const pqServerManagerModel* const model);
template <class T>
inline T pqGetItemAtIndex(const pqServerManagerModel* const model, int index);

/**
* pqServerManagerModel is the model for the Server Manager.
* All the pipelines in the Server Manager need a GUI representation
* to obtain additional information about their connections etc.
* This class collects that. This is merely representation of all the
* information available in the Server Manager in a more GUI friendly
* way. Simplicity is the key here.
*/
class PQCORE_EXPORT pqServerManagerModel : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
  * Constructor:
  * \c observer  :- instance of pqServerManagerObserver observing the server
  *                 manager.
  */
  pqServerManagerModel(pqServerManagerObserver* observer, QObject* parent = 0);
  ~pqServerManagerModel() override;

  /**
  * Given a session Id, returns the pqServer instance for that session,
  * if any.
  */
  pqServer* findServer(vtkIdType cid) const;

  /**
  * Given a vtkSession*, returns the pqServer instance for that session, if
  * any.
  */
  pqServer* findServer(vtkSession*) const;
  pqServer* findServer(vtkSMSession*) const;

  /**
  * Given a server resource, locates the pqServer instance for it, if any.
  */
  pqServer* findServer(const pqServerResource& resource) const;

  /**
  * Book end events for removing a server.
  */
  void beginRemoveServer(pqServer* server);
  void endRemoveServer();

  /**
  * This method to called by any code that's requesting ServerManager to
  * create a new connection (viz. pqObjectBuilder) to set the resource to be
  * used for the newly create pqServer instance. The active resource is
  * automatically cleared one a new pqServer instance is created. Refer to
  * pqObjectBuilder::createServer for details.
  */
  void setActiveResource(const pqServerResource& resource);

  /**
  * Given a proxy, locates a pqServerManagerModelItem subclass for the given
  * proxy.
  */
  template <class T>
  T findItem(vtkSMProxy* proxy) const
  {
    return ::pqFindItem<T>(this, proxy);
  }

  /**
  * Given the global id for a proxy,
  * locates a pqServerManagerModelItem subclass for the proxy.
  */
  template <class T>
  T findItem(vtkTypeUInt32 id) const
  {
    return ::pqFindItem<T>(this, id);
  }

  /**
  * Returns a list of pqServerManagerModelItem of the given type.
  */
  template <class T>
  QList<T> findItems() const
  {
    return ::pqFindItems<T>(this);
  }

  /**
  * Returns the number of items of the given type.
  */
  template <class T>
  int getNumberOfItems() const
  {
    return ::pqGetNumberOfItems<T>(this);
  }

  /**
  * Returns the item of the given type and the given index.
  * The index is determined by collecting all the items of the given type in a
  * list (findItems()).
  */
  template <class T>
  T getItemAtIndex(int index) const
  {
    return ::pqGetItemAtIndex<T>(this, index);
  }

  /**
  * Same as findItems<T>() except that this returns only those items that are
  * on the indicated server. If server == 0, then all items are returned.
  */
  template <class T>
  QList<T> findItems(pqServer* server) const
  {
    return ::pqFindItems<T>(this, server);
  }

  /**
  * Returns an item with the given name. The type can be pqProxy
  * subclass, since these are the ones that can have a name.
  * Note that since names need not be unique, using this method is not
  * recommended. This is provided for backwards compatibility alone.
  */
  template <class T>
  T findItem(const QString& name) const
  {
    return ::pqFindItem<T>(this, name);
  }

  /**
  * Internal method.
  */
  static void findItemsHelper(const pqServerManagerModel* const model, const QMetaObject& mo,
    QList<void*>* list, pqServer* server = 0);

  /**
  * Internal method.
  */
  static pqServerManagerModelItem* findItemHelper(
    const pqServerManagerModel* const model, const QMetaObject& mo, vtkSMProxy* proxy);

  /**
  * Internal method.
  */
  static pqServerManagerModelItem* findItemHelper(
    const pqServerManagerModel* const model, const QMetaObject& mo, vtkTypeUInt32 id);

  /**
  * Internal method.
  */
  static pqServerManagerModelItem* findItemHelper(
    const pqServerManagerModel* const model, const QMetaObject& mo, const QString& name);

Q_SIGNALS:
  /**
  * Signals emitted when a new pqServer object is created.
  */
  void preServerAdded(pqServer*);
  void serverAdded(pqServer*);

  /**
  * Signals emitted when a new pqServer object is properly initialized.
  * This happen just before serverAdded() but should not be used by user.
  * serverAdded() should be the signal to bind against unless you need to be
  * notified before the serverAdded() like the plugin loader do.
  */
  void serverReady(pqServer*);

  /**
  * Signals emitted when a pqServer instance is being destroyed.
  */
  void preServerRemoved(pqServer*);
  void serverRemoved(pqServer*);

  /**
  * Fired when beginRemoveServer is called.
  */
  void aboutToRemoveServer(pqServer* server);

  /**
  * Fired when endRemoveServer is called.
  */
  void finishedRemovingServer();

  /**
  * Signals emitted when any pqServerManagerModelItem subclass is created.
  */
  void preItemAdded(pqServerManagerModelItem*);
  void itemAdded(pqServerManagerModelItem*);

  /**
  * Signals emitted when any new pqServerManagerModelItem subclass is
  * being destroyed.
  */
  void preItemRemoved(pqServerManagerModelItem*);
  void itemRemoved(pqServerManagerModelItem*);

  void preProxyAdded(pqProxy*);
  void proxyAdded(pqProxy*);

  void preProxyRemoved(pqProxy*);
  void proxyRemoved(pqProxy*);

  /**
  * Signals emitted when a source/filter is created.
  */
  void preSourceAdded(pqPipelineSource* source);
  void sourceAdded(pqPipelineSource* source);

  /**
  * Signals emitted when a source/filter is destroyed.
  */
  void preSourceRemoved(pqPipelineSource*);
  void sourceRemoved(pqPipelineSource*);

  /**
  * Signals emitted when a view is created
  */
  void preViewAdded(pqView* view);
  void viewAdded(pqView* view);

  /**
  * Signals emitted when a view is destroyed.
  */
  void preViewRemoved(pqView*);
  void viewRemoved(pqView*);

  /**
  * Signals emitted when a representation is created.
  */
  void preRepresentationAdded(pqRepresentation* rep);
  void representationAdded(pqRepresentation* rep);

  /**
  * Signals emitted when a representation is destroyed.
  */
  void preRepresentationRemoved(pqRepresentation*);
  void representationRemoved(pqRepresentation*);

  //@{
  /**
   * Signals figured for pqExtractor.
   */
  void preExtractorAdded(pqExtractor*);
  void extractorAdded(pqExtractor*);
  void preExtractorRemoved(pqExtractor*);
  void extractorRemoved(pqExtractor*);
  //@}
  /**
   * Fired when the name of an item changes.
   */
  void nameChanged(pqServerManagerModelItem* item);

  /**
  * Fired when the state of the model item changes
  */
  void modifiedStateChanged(pqServerManagerModelItem* item);

  /**
  * Fired when a connection between two pqPipelineSources is created.
  */
  void connectionAdded(pqPipelineSource* source, pqPipelineSource* consumer, int srcOutputPort);
  void preConnectionAdded(pqPipelineSource* source, pqPipelineSource* consumer, int srcOutputPort);

  /**
  * Fired when a connection between tow pqPipelineSources is broken.
  */
  void connectionRemoved(pqPipelineSource* source, pqPipelineSource* consumer, int srcOutputPort);
  void preConnectionRemoved(
    pqPipelineSource* source, pqPipelineSource* consumer, int srcOutputPort);

  //@{
  /**
   * Signals fired to notify changes to extractor connections.
   */
  void connectionAdded(pqServerManagerModelItem* source, pqExtractor* consumer);
  void connectionRemoved(pqServerManagerModelItem* source, pqExtractor* consumer);
  //@}

  /**
  * Fired when a source indicates that data was updated i.e. the pipeline was
  * updated.
  */
  void dataUpdated(pqPipelineSource*);

protected Q_SLOTS:
  /**
  * Called when a proxy is registered.
  */
  virtual void onProxyRegistered(const QString& group, const QString& name, vtkSMProxy* proxy);

  /**
  * Called when a proxy is unregistered.
  */
  virtual void onProxyUnRegistered(const QString& group, const QString& name, vtkSMProxy* proxy);

  /**
  * Called when a new server connection is created.
  */
  virtual void onConnectionCreated(vtkIdType id);

  /**
  * Called when a server connection is closed.
  */
  virtual void onConnectionClosed(vtkIdType id);

  /**
  * Called when state file is loaded. We need to discover "helper proxies" and
  * set up the associations accordingly.
  */
  virtual void onStateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*);

private:
  Q_DISABLE_COPY(pqServerManagerModel)

  /**
  * Process the QSettings-only settings, setting the values in the
  * various settings proxies.
  */
  void updateSettingsFromQSettings(pqServer* server);

  class pqInternal;
  pqInternal* Internal;
};

//-----------------------------------------------------------------------------
template <class T>
inline QList<T> pqFindItems(const pqServerManagerModel* const model)
{
  QList<T> list;
  pqServerManagerModel::findItemsHelper(
    model, ((T)0)->staticMetaObject, reinterpret_cast<QList<void*>*>(&list), 0);
  return list;
}

//-----------------------------------------------------------------------------
template <class T>
inline QList<T> pqFindItems(const pqServerManagerModel* const model, pqServer* server)
{
  QList<T> list;
  pqServerManagerModel::findItemsHelper(
    model, ((T)0)->staticMetaObject, reinterpret_cast<QList<void*>*>(&list), server);
  return list;
}

//-----------------------------------------------------------------------------
template <class T>
inline T pqFindItem(const pqServerManagerModel* const model, vtkSMProxy* proxy)
{
  return qobject_cast<T>(
    pqServerManagerModel::findItemHelper(model, ((T)0)->staticMetaObject, proxy));
}

//-----------------------------------------------------------------------------
template <class T>
inline T pqFindItem(const pqServerManagerModel* const model, vtkTypeUInt32 id)
{
  return qobject_cast<T>(pqServerManagerModel::findItemHelper(model, ((T)0)->staticMetaObject, id));
}

//-----------------------------------------------------------------------------
template <class T>
inline T pqFindItem(const pqServerManagerModel* const model, const QString& name)
{
  return qobject_cast<T>(
    pqServerManagerModel::findItemHelper(model, ((T)0)->staticMetaObject, name));
}

//-----------------------------------------------------------------------------
template <class T>
inline int pqGetNumberOfItems(const pqServerManagerModel* const model)
{
  return pqFindItems<T>(model).size();
}

//-----------------------------------------------------------------------------
template <class T>
inline T pqGetItemAtIndex(const pqServerManagerModel* const model, int index)
{
  QList<T> items = pqFindItems<T>(model);
  if (index < items.size())
  {
    return items[index];
  }

  return 0;
}

#endif
