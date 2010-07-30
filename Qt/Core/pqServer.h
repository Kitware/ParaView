/*=========================================================================

   Program: ParaView
   Module:    pqServer.h

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

#ifndef _pqServer_h
#define _pqServer_h

class pqTimeKeeper;
class vtkProcessModule;
class vtkPVOptions;
class vtkPVServerInformation;
class vtkSMApplication;
class vtkSMProxy;
class vtkSMProxyManager;
class vtkSMRenderViewProxy;

#include "pqCoreExport.h"
#include "pqServerManagerModelItem.h"
#include "pqServerResource.h"
#include "vtkSmartPointer.h"
#include "vtkWeakPointer.h"
#include <QPointer>

/// Abstracts the concept of a "server connection" so that ParaView clients may: 
/// have more than one connect at a time / open and close connections at-will
class PQCORE_EXPORT pqServer : public pqServerManagerModelItem 
{
  Q_OBJECT
public:  
  pqServer(vtkIdType connectionId, vtkPVOptions*, QObject* parent = NULL);
  virtual ~pqServer();

  const pqServerResource& getResource();
  void setResource(const pqServerResource &server_resource);

  /// Returns the connection id for the server connection.
  vtkIdType GetConnectionID() const;

  /// Return the number of data server partitions on this 
  /// server connection. A convenience method.
  int getNumberOfPartitions();

  /// Returns is this connection is a connection to a remote
  /// server or a built-in server.
  bool isRemote() const;

  /// Returns the time keeper for this connection.
  pqTimeKeeper* getTimeKeeper() const;

  /// Initializes the pqServer, must be called as soon as pqServer 
  /// is created.
  void initialize();

  /// Every server can potentially be compiled with different compile time options
  /// while could lead to certain filters/sources/writers being non-instantiable
  /// on that server. For all proxies in the \c xmlgroup that the client 
  /// server manager is aware of, this method populates \c names with only the names
  /// for those proxies that can be instantiated on the given \c server.
  /// \todo Currently, this method does not actually validate if the server
  /// can instantiate the proxies.
  void getSupportedProxies(const QString& xmlgroup, QList<QString>& names);

  /// Returns the PVOptions for this connection. These are client side options.
  vtkPVOptions* getOptions() const;

  /// Returns the vtkPVServerInformation object which contains information about
  /// the command line options specified on the remote server, if any.
 vtkPVServerInformation* getServerInformation() const;

  /// Get/Set the application wide heart beat timeout setting.
  /// Heartbeats are used in case of remote server connections to avoid the
  /// connection timing out due to inactivity. When set, the client send a
  /// heartbeat message to all servers every \c msec milliseconds.
  static void setHeartBeatTimeoutSetting(int msec);
  static int getHeartBeatTimeoutSetting();

  // Get/Set the application wide coincident topology resolution settings.
  static void setCoincidentTopologyResolutionModeSetting(int mode);
  static int coincidentTopologyResolutionModeSetting();
  static void setPolygonOffsetParametersSetting(double factor, double value);
  static void polygonOffsetParametersSetting(double &factor, double &value);
  static void setPolygonOffsetFacesSetting(bool);
  static bool polygonOffsetFacesSetting();
  static void setZShiftSetting(double shift);
  static double zShiftSetting();

  /// Convenience method to obtain the renderview xml name for the given
  /// connection type. This is deprecated and will soon be removed.
  QString getRenderViewXMLName() const;

signals:
  /// Fired when the name of the proxy is changed.
  void nameChanged(pqServerManagerModelItem*);

  /// Fired about 5 minutes before the server timesout. This signal will not be
  /// fired at all if server timeout < 5 minutes. The server timeout is
  /// specified by --timeout option on the server process.
  /// This is not fired if timeout is not specified on the server process.
  void fiveMinuteTimeoutWarning();

  /// Fired about 1 minute before the server timesout. 
  /// The server timeout is specified by --timeout option on the server process.
  /// This is not fired if timeout is not specified on the server process.
  void finalTimeoutWarning();

protected:
  // Creates the TimeKeeper proxy for this connection.
  void createTimeKeeper();

  /// Returns the string key used for the heart beat time interval.
  static const char* HEARBEAT_TIME_SETTING_KEY();

  /// Set the heartbeat timeout for this instance of pqServer.
  void setHeartBeatTimeout(int msec);
  void setCoincidentTopologyResolutionMode(int);
  void setPolygonOffsetParameters(double factor, double units);
  void setPolygonOffsetFaces(bool offset_faces);
  void setZShift(double shift);

  // updates all servers with the current settings.
  static void updateCoincidentTopologySettings();

protected slots:
  /// Called to send a heartbeat to the server.
  void heartBeat();

private:
  pqServer(const pqServer&);  // Not implemented.
  pqServer& operator=(const pqServer&); // Not implemented.

  pqServerResource Resource;
  vtkIdType ConnectionID;
  vtkWeakPointer<vtkSMProxy> CoincidentTopologyResolutionProxy;

  // TODO:
  // Each connection will eventually have a PVOptions object. 
  // For now, this is same as the vtkProcessModule::Options.
  vtkSmartPointer<vtkPVOptions> Options;

  class pqInternals;
  pqInternals* Internals;
};

#endif // !_pqServer_h
