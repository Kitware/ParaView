/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef pqMemoryInspectorPanel_h
#define pqMemoryInspectorPanel_h

#include "pqComponentsModule.h"
#include <QMenu>
#include <QProcess>
#include <QWidget>

#include <map>
using std::map;
#include <string>
using std::string;
#include <vector>
using std::vector;

class pqMemoryInspectorPanelUI;
class HostData;
class RankData;
class QTreeWidgetItem;
class vtkPVSystemConfigInformation;
class pqView;

class PQCOMPONENTS_EXPORT pqMemoryInspectorPanel : public QWidget
{
  Q_OBJECT
public:
  pqMemoryInspectorPanel(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags{});
  ~pqMemoryInspectorPanel() override;

  // Description:
  // Test for successful initialization.
  int Initialized() { return this->ClientHost != nullptr; }

protected:
  // Description:
  // Update when the panel is made visible.
  void showEvent(QShowEvent* event) override;

protected Q_SLOTS:

  // Description:
  // Configure the UI based on conneccted servers.
  void ServerDisconnected();
  void ServerConnected();

  // Description:
  // The panel will update itself after render events end. Render events are
  // used because they occur only after all server side action is complete
  // and rendering initself can use significant resources. The update is
  // enabled only after pqView::dataUpdatedEvent.
  void ConnectToView(pqView* view);
  void RenderCompleted();
  void EnableUpdate();

  // Description:
  // Clear all member variables and models.
  void Clear();

  // Description:
  // Update the UI with values from the server(s).
  int Initialize();

  // Description:
  // Update the UI with the latest values from the server(s).
  void Update();

  // Description:
  // Enable auto update.
  void SetAutoUpdate(bool state) { this->AutoUpdate = state; }

  // Description:
  // enable/disable stack trace.
  void EnableStackTraceOnClient(bool enable);
  void EnableStackTraceOnServer(bool enable);
  void EnableStackTraceOnDataServer(bool enable);
  void EnableStackTraceOnRenderServer(bool enable);

  // Description:
  // run remote command on one of the client or server ranks.
  void ExecuteRemoteCommand();
  void RemoteCommandFailed(QProcess::ProcessError code);

  // Description:
  // Display host properties
  void ShowHostPropertiesDialog();

  // Description:
  // Create a context menu for the config view.
  void ConfigViewContextMenu(const QPoint& pos);

  // Description:
  // Collapse or expand the view for easier navigation
  // when larger jobs are in play.
  void ShowOnlyNodes();
  void ShowAllRanks();

private:
  void ClearClient();
  void ClearServers();
  void ClearServer(map<string, HostData*>& hosts, vector<RankData*>& ranks);

  void UpdateRanks();
  void UpdateHosts();
  void UpdateHosts(map<string, HostData*>& hosts);

  void InitializeServerGroup(long long clientPid, vtkPVSystemConfigInformation* configs,
    int validProcessType, QTreeWidgetItem* group, string groupName, map<string, HostData*>& hosts,
    vector<RankData*>& ranks, int& systemType);

  void EnableStackTrace(bool enable, int group);
  void AddEnableStackTraceMenuAction(int serverType, QMenu& context);

  QWidget* NewGroupWidget(string name, string icon);

private:
  pqMemoryInspectorPanelUI* Ui;

  bool ClientOnly;
  HostData* ClientHost;
  int ClientSystemType;
  bool StackTraceOnClient;

  map<string, HostData*> ServerHosts;
  vector<RankData*> ServerRanks;
  int ServerSystemType;
  bool StackTraceOnServer;

  map<string, HostData*> DataServerHosts;
  vector<RankData*> DataServerRanks;
  int DataServerSystemType;
  bool StackTraceOnDataServer;

  map<string, HostData*> RenderServerHosts;
  vector<RankData*> RenderServerRanks;
  int RenderServerSystemType;
  bool StackTraceOnRenderServer;

  bool UpdateEnabled;
  bool PendingUpdate;
  bool AutoUpdate;
};

#endif
