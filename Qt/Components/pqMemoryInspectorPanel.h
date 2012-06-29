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
#ifndef __pqMemoryInspectorPanel_h
#define __pqMemoryInspectorPanel_h

#include "pqComponentsExport.h"
#include <QWidget>
#include <QProcess>

#include <map>
using std::map;
#include <string>
using std::string;
#include  <vector>
using std::vector;

class pqMemoryInspectorPanelUI;
class HostData;
class RankData;

class PQCOMPONENTS_EXPORT pqMemoryInspectorPanel : public QWidget
{
  Q_OBJECT
public:
  pqMemoryInspectorPanel(QWidget* parent=0, Qt::WindowFlags f=0);
  ~pqMemoryInspectorPanel();

protected slots:
  // Description:
  // Update the UI with values from the server(s).
  void Initialize();

  // Description:
  // Refresh the UI with the latest values from the server(s).
  void Refresh();

  // Description:
  // enable/disable stack trace.
  void EnableStackTraceSignalHandler(int enable);

  // Description:
  // run remote command on one of the client or server ranks.
  void ExecuteRemoteCommand();
  void RemoteCommandFailed(QProcess::ProcessError code);

  // Description:
  // Display host properties
  void ShowHostPropertiesDialog();

  // Description:
  // Enable/disable buttons based on current selction.
  void UpdateConfigViewButtons();

  // Description:
  // Use when an artificial limit on per-process memory
  // consumption is in play, such as on a shared memory system.
  void OverrideCapacity();

  // Description:
  // Create a context menu for the config view.
  void ConfigViewContextMenu(const QPoint &pos);

  // Description:
  // Collapse or expand the view for easier navigation
  // when larger jobs are in play.
  void ShowOnlyNodes();
  void ShowAllRanks();

private:
  void ClearClient();
  void ClearServers();
  void ClearServer(
      map<string,HostData *> &hosts,
      vector<RankData *> &ranks);

  void OverrideCapacity(map<string,HostData*> &hosts);

  void RefreshRanks();
  void RefreshHosts();
  void RefreshHosts(map<string,HostData*> &hosts);

private:
  pqMemoryInspectorPanelUI *Ui;

  int ClientOnly;
  HostData *ClientHost;
  RankData *ClientRank;
  int ClientSystemType;

  map<string,HostData *> ServerHosts;
  vector<RankData *> ServerRanks;

  map<string,HostData *> DataServerHosts;
  vector<RankData *> DataServerRanks;

  map<string,HostData *> RenderServerHosts;
  vector<RankData *> RenderServerRanks;
};

#endif
