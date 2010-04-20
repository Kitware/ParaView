/*=========================================================================

   Program: ParaView
   Module:    pqViewModuleInterface.h

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

#ifndef _pqViewModuleInterface_h
#define _pqViewModuleInterface_h

#include "pqCoreExport.h"

#include <QtPlugin>
#include <QStringList>
class vtkSMProxy;
class vtkSMViewProxy;
class pqServer;
class pqView;
class pqDataRepresentation;

/// interface class for plugins that create view modules
class PQCORE_EXPORT pqViewModuleInterface
{
public:
  pqViewModuleInterface();
  virtual ~pqViewModuleInterface();
  
  /// Return a list of view types supported by this interface
  virtual QStringList viewTypes() const = 0;
  
  /// Return a list of display types supported by this interface
  /// overload this if you have custom pqConsuerDisplay's
  virtual QStringList displayTypes() const { return QStringList(); }
 
  // TODO: remove (make virtual function in pqView)
  /// Return a friendly type name (e.g. "My Custom View" in place of
  /// "MyCustomView")
  virtual QString viewTypeName(const QString& viewtype) const = 0;

  /// Returns true if this can create a view of a given name
  /// The name corresponds to the Server Manager XML Hint such as
  /// @verbatim
  /// <SourceProxy name="MyCustomFilter" class="vtkMyCustomFilter">
  ///  ...
  ///  <Hints>
  ///   <View type="MyCustom" />
  ///  </Hints>
  /// </SourceProxy>
  /// @endverbatim
  virtual bool canCreateView(const QString& viewtype) const = 0;
  
  /// Creates the Server Manager view module
  /// For example:
  /// @verbatim
  /// <ProxyGroup name="plotmodules">
  ///  <ViewModuleProxy name="MyCustomViewModule">
  ///    base_proxygroup="rendermodules"
  ///    base_proxyname="ViewModule"
  ///    display_name="MyCustomViewDisplay"
  ///  </ViewModuleProxy>
  /// </ProxyGroup>
  /// @endverbatim
  ///
  /// implement this to call 
  /// vtkSMProxyManager::NewProxy("plotmodules", "MyCustomViewModule")
  virtual vtkSMProxy* createViewProxy(const QString& viewtype,
                                      pqServer *server) = 0;

  /// Creates the GUI view that corresponds with the server manager view
  /// viewtype is the type of view (e.g. MyCustomViewModule),
  /// group is the group that the viewmodule was registered with,
  /// name is the name that the view module was registered with,
  /// viewmodule is the server manager view module this GUI side view module
  /// corresponds with,
  /// server is the server it was created on,
  /// parent is the QObject parent.
  virtual pqView* createView(const QString& viewtypemodule,
    const QString& group,
    const QString& name,
    vtkSMViewProxy* viewmodule,
    pqServer* server,
    QObject* parent) = 0;

  /// Creates a pqDataRepresentation that corresponds with the ViewModuleProxy's
  /// display_name
  /// this is optional and only needs to be implemented if the pqDataRepresentation
  /// needs to do something special
  virtual pqDataRepresentation* createDisplay(const QString& /*display_type*/, 
    const QString& /*group*/,
    const QString& /*name*/,
    vtkSMProxy* /*proxy*/,
    pqServer* /*server*/,
    QObject* /*parent*/) { return NULL; }

};

Q_DECLARE_INTERFACE(pqViewModuleInterface, "com.kitware/paraview/viewmodule")

#endif

