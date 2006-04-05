/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef _pqServer_h
#define _pqServer_h

class vtkProcessModule;
class vtkPVOptions;
class vtkSMApplication;
class vtkSMProxyManager;
class vtkSMMultiViewRenderModuleProxy;

#include "pqWidgetsExport.h"
#include <QObject>
#include <QString>
#include "vtkConnectionID.h" // needed for connection Id.
#include "vtkSmartPointer.h"

/// Abstracts the concept of a "server connection" so that ParaQ clients may: have more than one connect at a time / open and close connections at-will
class PQWIDGETS_EXPORT pqServer : public QObject
{
public:
  /// Constructs a standalone or "built-in" server connection, returns NULL on failure
  static pqServer* CreateStandalone();
  /// Constructs a server connection to a remote host, returns NULL on failure
  static pqServer* CreateConnection(const char* const hostName, const int portNumber);
  static pqServer* CreateConnection(const char* const ds_hostName, const int ds_portNumber,
    const char* const rs_hostName, const int rs_portNumber);

  virtual ~pqServer();

  /// getAddress() will start reporting the correct address once, there is a separate 
  /// vtkPVOptions per connection, until then, avoid using it.
  QString getAddress() const;

  const QString& getFriendlyName() const {return this->FriendlyName;}
  void setFriendlyName(const QString& name);

  /// Returns the multi view manager proxy for this connection.
  vtkSMMultiViewRenderModuleProxy* GetRenderModule();

  vtkConnectionID GetConnectionID() { return this->ConnectionID; }
protected:
  pqServer(vtkConnectionID, vtkPVOptions*);

  /// Creates vtkSMMultiViewRenderModuleProxy for this connection and 
  /// initializes it to create render modules of correct type 
  /// depending upon the connection.
  void CreateRenderModule();
private:
  pqServer(const pqServer&);  // Not implemented.
  pqServer& operator=(const pqServer&); // Not implemented.

  vtkConnectionID ConnectionID;
  QString FriendlyName;
  vtkSmartPointer<vtkSMMultiViewRenderModuleProxy> RenderModule;
  // TODO:
  // Each connection will eventually have a PVOptions object. 
  // For now, this is same as the vtkProcessModule::Options.
  vtkSmartPointer<vtkPVOptions> Options;
};

#endif // !_pqServer_h

