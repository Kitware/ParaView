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

// This class represents any registered Server Manager proxy.
// It keeps essential information to locate the proxy as well as
// additional metadata such as user-specified label.
class PQCORE_EXPORT pqProxy : public pqServerManagerModelItem
{
public:
  pqProxy(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* parent=NULL);
  virtual ~pqProxy() {}

  /// Get the server on which this proxy exists.
  pqServer *getServer() const
    { return this->Server; }

  /// Get/Set the user-sepecified name for this proxy.
  /// This name need not be unique, it's just the label
  /// used to identify this proxy for the user. By default,
  /// it is same as the registeration name.
  void setProxyName(const QString& name)
    { this->ProxyName = name; }
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
private:
  pqServer *Server;           ///< Stores the parent server.
  QString ProxyName;
  QString SMName;
  QString SMGroup;
  vtkSmartPointer<vtkSMProxy> Proxy;
};

#endif
