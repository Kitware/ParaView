/*=========================================================================

   Program: ParaView
   Module:    pqProxy.h

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

/// \file pqProxy.h
///
/// \date 11/16/2005

#ifndef _pqPipelineObject_h
#define _pqPipelineObject_h

#include "pqServerManagerModelItem.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
class pqServer;
class vtkSMProxy;
class pqProxyInternal;

// This class represents any registered Server Manager proxy.
// It keeps essential information to locate the proxy as well as
// additional metadata such as user-specified label.
class PQCORE_EXPORT pqProxy : public pqServerManagerModelItem
{
  Q_OBJECT
public:
  pqProxy(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* parent=NULL);
  virtual ~pqProxy();

  /// Get the server on which this proxy exists.
  pqServer *getServer() const
    { return this->Server; }

  /// This method renames this proxy. It registers the proxy
  /// under the same old group but with new name, and unregisters
  /// the old name. This must be distinguished from setProxyName()
  /// which merely changes the client side name.
  void rename(const QString& newname);

  // Get/Set the name for the pqProxy. This is the same name
  // with which the vtkSMProxy is registered with the proxy manager.
  // This does not affect the name with which the proxy is registered.
  // Emit nameChanged() signal when the name changes. 
  void setProxyName(const QString& name)
    {
    if (this->ProxyName != name)
      {
      this->ProxyName = name; 
      emit this->nameChanged(this);
      }
    }
  const QString& getProxyName()
    { return this->ProxyName; }
  
  /// Get the name with which this proxy is registered on the
  /// server manager. A proxy can be registered with more than
  /// one name on the Server Manager. This is the name/group which
  /// this pqProxy stands for. 
  const QString& getSMName()
    { return this->SMName; }
  const QString& getSMGroup()
    { return this->SMGroup; }

  /// Get the vtkSMProxy this object stands for.
  vtkSMProxy* getProxy() const
    {return this->Proxy; }
  /// Returns a list of all the internal proxies added with a given key.
  QList<vtkSMProxy*> getInternalProxies(const QString& key) const;

signals:
  /// Fired when the name of the proxy is changed.
  void nameChanged(pqServerManagerModelItem*);

protected:
  /// Concept of internal proxies:
  // A pqProxy is created for every important vtkSMProxy registered. Many a times, 
  // there may be other proxies associated with that proxy, eg. lookup table proxies,
  // implicit function proxies may be associated with a filter/source proxy. 
  // The GUI can create "associated" proxies and add them as internal proxies.
  // Internal proxies get registered under special groups, so that they are 
  // undo/redo-able, and state save-restore-able. The pqProxy makes sure that 
  // the internal proxies are unregistered when the main proxy is unregistered.
  void addInternalProxy(const QString& key, vtkSMProxy*);
  void removeInternalProxy(const QString& key, vtkSMProxy*);

  // Unregisters all internal proxies.
  void clearInternalProxies();

private:
  pqServer *Server;           ///< Stores the parent server.
  QString ProxyName;
  QString SMName;
  QString SMGroup;
  vtkSmartPointer<vtkSMProxy> Proxy;
  pqProxyInternal* Internal;
};

#endif
