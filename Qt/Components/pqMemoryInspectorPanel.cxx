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
#include "pqMemoryInspectorPanel.h"
#include "pqRemoteCommandDialog.h"
#include "ui_pqMemoryInspectorPanelForm.h"
using Ui::pqMemoryInspectorPanelForm;

// #define pqMemoryInspectorPanelDEBUG

#include "pqComponentsExport.h"
#include "pqActiveObjects.h"

#include "vtkProcessModule.h"
#include "vtkPVSystemConfigInformation.h"
#include "vtkPVMemoryUseInformation.h"
#include "vtkPVEnableStackTraceSignalHandler.h"
#include "vtkPVDisableStackTraceSignalHandler.h"
#include "vtkSMSession.h"
#include "vtkPVInformation.h"
#include "vtkClientServerStream.h"

#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QMessageBox>
#include <QProgressBar>
#include <QPalette>
#include <QFont>
#include <QPlastiqueStyle>
#include <QPoint>
#include <QMenu>
#include <QProcess>
#include <QDebug>

#include <map>
using std::map;
using std::pair;
#include <vector>
using std::vector;
#include <string>
using std::string;
#include <sstream>
using std::ostringstream;
using std::istringstream;
#include <iostream>
using std::cerr;
using std::endl;

#define pqErrorMacro(estr)\
  qDebug()\
      << "Error in:" << endl\
      << __FILE__ << ", line " << __LINE__ << endl\
      << "" estr << endl;

// User interface
//=============================================================================
class pqMemoryInspectorPanelUI
    :
  public Ui::pqMemoryInspectorPanelForm
    {};


// QProgress bar makes use of integer type. Each power of 10
// greater than 100 result in a decimal place of precision in
// the UI.
#define SQPM_PROGBAR_MAX 1000

enum {
  PROCESS_TYPE=Qt::UserRole,
  PROCESS_TYPE_INVALID,
  PROCESS_TYPE_CLIENT_GROUP,
  PROCESS_TYPE_CLIENT_HOST,
  PROCESS_TYPE_CLIENT_RANK,
  PROCESS_TYPE_SERVER_GROUP,
  PROCESS_TYPE_SERVER_HOST,
  PROCESS_TYPE_SERVER_RANK,
  PROCESS_ID,                 // unsigned long long
  PROCESS_HOST_NAME,          // string, hostname
  PROCESS_RANK_INVALID,       //
  PROCESS_RANK,               // int
  PROCESS_HOST_OS,            // descriptive string
  PROCESS_HOST_CPU,           // descriptive string
  PROCESS_HOST_MEM,           // descriptive string
  PROCESS_FQDN,          // string, network address
  PROCESS_SYSTEM_TYPE         // int (0 unix like, 1 win)
};

namespace
{

// ****************************************************************************
QPlastiqueStyle *getLoadWidgetStyle()
{
  // this sets the style for the progress bar used to
  // display % memory usage. If we didn't do this the
  // display will look different on each OS. The ownership
  // of the style does not change hands when it's set to
  // the widget thus a single static instance is convenient.
  static QPlastiqueStyle style;
  return &style;
}

//*****************************************************************************
QString translateUnits(float memUse)
{
  QString fmt("%1 %2");

  float p210=pow(2.0,10.0);
  float p220=pow(2.0,20.0);
  float p230=pow(2.0,30.0);
  float p240=pow(2.0,40.0);
  float p250=pow(2.0,50.0);

  // were dealing with kiB
  memUse *= 1024;

  if (memUse<p210)
    {
    return fmt.arg(memUse, 0,'f',2).arg("B");
    }
  else
  if (memUse<p220)
    {
    return fmt.arg(memUse/p210, 0,'f',2).arg("KiB");
    }
  else
  if (memUse<p230)
    {
    return fmt.arg(memUse/p220, 0,'f',2).arg("MiB");
    }
  else
  if (memUse<p240)
    {
    return fmt.arg(memUse/p230, 0,'f',2).arg("GiB");
    }
  else
  if (memUse<p250)
    {
    return fmt.arg(memUse/p240, 0,'f',2).arg("TiB");
    }

  return fmt.arg(memUse/p250, 0,'f',2).arg("PiB");
}

//*****************************************************************************
template<typename T>
void ClearVectorOfPointers(vector<T *> data)
{
  size_t n=data.size();
  for (size_t i=0; i<n; ++i)
    {
    if (data[i])
      {
      delete data[i];
      }
    }
  data.clear();
}


//*****************************************************************************
void unescapeWhitespace(string &s, char escChar)
{
  size_t n=s.size();
  for (size_t i=0; i<n; ++i)
    {
    if (s[i]==escChar)
      {
      s[i]=' ';
      }
    }
}

};

/// data associated with a rank
//=============================================================================
class RankData
{
public:
  RankData(
      int rank,
      int pid,
      unsigned long long load,
      unsigned long long capacity);

  ~RankData();

  void SetRank(int rank){ this->Rank=rank; }
  int GetRank(){ return this->Rank; }

  void SetPid(int pid){ this->Pid=pid; }
  int GetPid(){ return this->Pid; }

  void OverrideCapacity(unsigned long long capacity);
  void SetCapacity(unsigned long long capacity){ this->Capacity=capacity; }
  unsigned long long GetCapacity(){ return this->Capacity; }

  void SetLoad(unsigned long long load){ this->Load=load; }
  unsigned long long GetLoad(){ return this->Load; }
  float GetLoadFraction(){ return (float)this->Load/(float)this->Capacity; }

  void SetLoadWidget(QProgressBar *widget){ this->LoadWidget=widget; }
  QProgressBar *GetLoadWidget(){ return this->LoadWidget; }
  void UpdateLoadWidget();
  void InitializeLoadWidget();

private:
  int Rank;
  int Pid;
  unsigned long long Load;
  unsigned long long Capacity;
  QProgressBar *LoadWidget;
  //HostData *Host;
};

//-----------------------------------------------------------------------------
RankData::RankData(
      int rank,
      int pid,
      unsigned long long load,
      unsigned long long capacity)
        :
    Rank(rank),
    Pid(pid),
    Load(load),
    Capacity(capacity)
{
  this->InitializeLoadWidget();
}

//-----------------------------------------------------------------------------
RankData::~RankData()
{
  delete this->LoadWidget;
}

//-----------------------------------------------------------------------------
void RankData::UpdateLoadWidget()
{
  float used = this->GetLoad();
  float fracUsed = this->GetLoadFraction();
  float percUsed = fracUsed*100.0;
  int progVal = fracUsed*SQPM_PROGBAR_MAX;

  this->LoadWidget->setValue(progVal);
  this->LoadWidget->setFormat(
    QString("%1 %2%").arg(::translateUnits(used)).arg(percUsed, 0,'f',2));

  QPalette palette(this->LoadWidget->palette());
  if (fracUsed>0.75)
    {
    // danger -> red
    palette.setColor(QPalette::Highlight,QColor(232,40,40));
    }
  else
    {
    // ok -> green
    palette.setColor(QPalette::Highlight,QColor(66,232,20));
    }
  this->LoadWidget->setPalette(palette);
}

//-----------------------------------------------------------------------------
void RankData::InitializeLoadWidget()
{
  this->LoadWidget=new QProgressBar;
  this->LoadWidget->setStyle(::getLoadWidgetStyle());
  this->LoadWidget->setMaximumHeight(15);
  this->LoadWidget->setMinimum(0);
  this->LoadWidget->setMaximum(SQPM_PROGBAR_MAX);
  QFont font(this->LoadWidget->font());
  font.setPointSize(8);
  this->LoadWidget->setFont(font);

  this->UpdateLoadWidget();
}

//-----------------------------------------------------------------------------
void RankData::OverrideCapacity(unsigned long long capacity)
{
  this->Capacity=capacity;
  this->UpdateLoadWidget();
}


/// data associated with the host
//=============================================================================
class HostData
{
public:
  HostData(string hostName,unsigned long long capacity);
  HostData(const HostData &other){ *this=other; }
  const HostData &operator=(const HostData &other);
  ~HostData();

  void SetHostName(string name){ this->HostName=name; }
  string &GetHostName(){ return this->HostName; }

  void SetRealCapacity(unsigned long long capacity){ this->RealCapacity=capacity; }
  unsigned long long GetRealCapacity(){ return this->RealCapacity; }

  void SetCapacity(unsigned long long capacity){ this->Capacity=capacity; }
  unsigned long long GetCapacity(){ return this->Capacity; }

  /**
  Set the capacity based on some artificial restriction on
  per-process memory use. eg. shared memory machine. Updates
  all ranks.
  */
  void OverrideCapacity(unsigned long long capacity);

  /**
  Reset the capacity to the available amount of ram. Updates
  all ranks.
  */
  void ResetCapacity();

  void SetTreeItem(QTreeWidgetItem *item){ this->TreeItem=item; }
  QTreeWidgetItem *GetTreeItem(){ return this->TreeItem; }

  void SetLoadWidget(QProgressBar *widget){ this->LoadWidget=widget; }
  QProgressBar *GetLoadWidget(){ return this->LoadWidget; }
  void InitializeLoadWidget();
  void UpdateLoadWidget();

  RankData *AddRankData(int rank, int pid);
  RankData *GetRankData(int i){ return this->Ranks[i]; }
  void ClearRankData();

  unsigned long long GetLoad();
  float GetLoadFraction(){ return (float)this->GetLoad()/(float)this->Capacity; }

private:
  string HostName;
  unsigned long long Capacity;     // host memory available to us.
  unsigned long long RealCapacity; // host memory available.
  QProgressBar *LoadWidget;
  QTreeWidgetItem *TreeItem;
  vector<RankData *> Ranks;
};

//-----------------------------------------------------------------------------
HostData::HostData(
      string hostName,
      unsigned long long capacity)
        :
    HostName(hostName),
    Capacity(capacity),
    RealCapacity(capacity)
{
  this->InitializeLoadWidget();
}

//-----------------------------------------------------------------------------
HostData::~HostData()
{
  this->ClearRankData();
  delete this->LoadWidget;
}

//-----------------------------------------------------------------------------
const HostData &HostData::operator=(const HostData &other)
{
  if (this==&other) return *this;

  this->HostName=other.HostName;
  this->Capacity=other.Capacity;
  this->RealCapacity=other.RealCapacity;
  this->LoadWidget=other.LoadWidget;
  this->TreeItem=other.TreeItem;
  this->Ranks=other.Ranks;

  return *this;
}

//-----------------------------------------------------------------------------
void HostData::ClearRankData()
{
  ::ClearVectorOfPointers<RankData>(this->Ranks);
}

//-----------------------------------------------------------------------------
RankData *HostData::AddRankData(int rank, int pid)
{
  RankData *newRank=new RankData(rank,pid,0,this->Capacity);
  this->Ranks.push_back(newRank);
  return newRank;
}

//-----------------------------------------------------------------------------
unsigned long long HostData::GetLoad()
{
  unsigned long long load=0;
  size_t n=this->Ranks.size();
  for (size_t i=0; i<n; ++i)
    {
    load+=this->Ranks[i]->GetLoad();
    }
  return load;
}

//-----------------------------------------------------------------------------
void HostData::ResetCapacity()
{
  this->Capacity = this->RealCapacity;

  size_t n=this->Ranks.size();
  for (size_t i=0; i<n; ++i)
    {
    this->Ranks[i]->OverrideCapacity(this->RealCapacity);
    }

  this->UpdateLoadWidget();
}

//-----------------------------------------------------------------------------
void HostData::OverrideCapacity(unsigned long long capacity)
{
  size_t n=this->Ranks.size();
  this->Capacity = n*capacity;

  for (size_t i=0; i<n; ++i)
    {
    this->Ranks[i]->OverrideCapacity(capacity);
    }

  this->UpdateLoadWidget();
}

//-----------------------------------------------------------------------------
void HostData::InitializeLoadWidget()
{

  this->LoadWidget=new QProgressBar;
  this->LoadWidget->setStyle(::getLoadWidgetStyle());
  this->LoadWidget->setMaximumHeight(15);
  this->LoadWidget->setMinimum(0);
  this->LoadWidget->setMaximum(SQPM_PROGBAR_MAX);
  QFont font(this->LoadWidget->font());
  font.setPointSize(8);
  this->LoadWidget->setFont(font);

  this->UpdateLoadWidget();
}

//-----------------------------------------------------------------------------
void HostData::UpdateLoadWidget()
{
  float used = this->GetLoad();
  float fracUsed = this->GetLoadFraction();
  float percUsed = fracUsed*100.0;
  int progVal = fracUsed*SQPM_PROGBAR_MAX;

  this->LoadWidget->setValue(progVal);
  this->LoadWidget->setFormat(
    QString("%1 %2%").arg(::translateUnits(used)).arg(percUsed, 0,'f',2));

  QPalette palette(this->LoadWidget->palette());
  if (fracUsed>0.75)
    {
    // danger -> red
    palette.setColor(QPalette::Highlight,QColor(232,40,40));
    }
  else
    {
    // ok -> green
    palette.setColor(QPalette::Highlight,QColor(66,232,20));
    }
  this->LoadWidget->setPalette(palette);
}



//-----------------------------------------------------------------------------
pqMemoryInspectorPanel::pqMemoryInspectorPanel(
        QWidget* parent,
        Qt::WindowFlags flags)
            :
        QWidget(parent,flags)
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::pqMemoryInspectorPanel" << endl;
  #endif

  this->ClientOnly=0;
  this->ClientHost=0;
  this->ClientRank=0;

  // Construct Qt form.
  this->Ui=new pqMemoryInspectorPanelUI;
  this->Ui->setupUi(this);
  this->Ui->updateMemUse->setIcon(QPixmap(":/pqWidgets/Icons/pqRedo24.png"));

  // Set up configuration viewer
  this->Initialize();

  // apply capacity override
  QObject::connect(
        this->Ui->overrideCapacity,
        SIGNAL(editingFinished()),
        this,
        SLOT(OverrideCapacity()));

  QObject::connect(
        this->Ui->enableOverrideCapacity,
        SIGNAL(toggled(bool)),
        this,
        SLOT(OverrideCapacity()));

  // execute remote command
  QObject::connect(
        this->Ui->configView,
        SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
        this,
        SLOT(UpdateConfigViewButtons()));

  QObject::connect(
        this->Ui->openCommandDialog,
        SIGNAL(clicked()),
        this,
        SLOT(ExecuteRemoteCommand()));

  // host props
  QObject::connect(
        this->Ui->configView,
        SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
        this,
        SLOT(UpdateConfigViewButtons()));

  QObject::connect(
        this->Ui->openPropertiesDialog,
        SIGNAL(clicked()),
        this,
        SLOT(ShowHostPropertiesDialog()));

  // intialize new remote server
  QObject::connect(
        &pqActiveObjects::instance(),
        SIGNAL(serverChanged(pqServer*)),
        this,
        SLOT(Initialize()));

  // refresh
  QObject::connect(
        this->Ui->updateMemUse,
        SIGNAL(released()),
        this,
        SLOT(Refresh()));

  // context menu
  QObject::connect(
        this->Ui->configView,
        SIGNAL(customContextMenuRequested(const QPoint &)),
        this,
        SLOT(ConfigViewContextMenu(const QPoint &)));

  this->Ui->configView->setContextMenuPolicy(Qt::CustomContextMenu);

  // stack trace signal handler
  QObject::connect(
        this->Ui->btSignalHandler,
        SIGNAL(stateChanged(int)),
        this,
        SLOT(EnableStackTraceSignalHandler(int)));

  this->Ui->configView->expandAll();
}

//-----------------------------------------------------------------------------
pqMemoryInspectorPanel::~pqMemoryInspectorPanel()
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::~pqMemoryInspectorPanel" << endl;
  #endif

  this->ClearClient();
  this->ClearServers();

  delete this->Ui;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ClearClient()
{
  if (this->ClientHost)
    {
    delete this->ClientHost;
    }
  this->ClientHost=0;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ClearServer(
      map<string,HostData *> &hosts,
      vector<RankData *> &ranks)
{
  map<string,HostData*>::iterator it=hosts.begin();
  map<string,HostData*>::iterator end=hosts.end();
  while (it!=end)
    {
    if ((*it).second)
      {
      delete (*it).second;
      }
    ++it;
    }
  hosts.clear();
  ranks.clear();
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ClearServers()
{
  this->ClearServer(this->ServerHosts,this->ServerRanks);
  this->ClearServer(this->DataServerHosts,this->DataServerRanks);
  this->ClearServer(this->RenderServerHosts,this->RenderServerRanks);
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::Initialize()
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::Initialize" << endl;
  #endif

  vtkPVSystemConfigInformation *configs;

  this->ClearClient();
  this->ClearServers();

  // remove all branches from process tree
  this->Ui->configView->clear();

  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
    {
    // this is not necessarilly an error as the panel may be created
    // before the server is connected.
    return;
    }

  // add branches in the view for various pv types
  // clients
  QTreeWidgetItem *clientGroup
    = new QTreeWidgetItem(this->Ui->configView,QStringList("paraview"));
  clientGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientGroup->setExpanded(true);
  clientGroup->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_CLIENT_GROUP));

  configs=vtkPVSystemConfigInformation::New();
  server->session()->GatherInformation(vtkSMSession::CLIENT,configs,0);

  int nConfigs=configs->GetSize();
  if (nConfigs<1)
    {
    pqErrorMacro("There should always be a client.");
    return;
    }

  unsigned long long clientPid=configs->GetPid(0);

  this->ClientHost
    = new HostData(configs->GetHostName(0),configs->GetCapacity(0));

  this->ClientRank
   = this->ClientHost->AddRankData(0,configs->GetPid(0));

  this->ClientSystemType=configs->GetSystemType(0);

  QTreeWidgetItem *clientHostItem=new QTreeWidgetItem(clientGroup);
  clientHostItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientHostItem->setExpanded(true);
  clientHostItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_CLIENT_HOST));
  clientHostItem->setData(0,PROCESS_HOST_OS,QString(configs->GetOSDescriptor(0)));
  clientHostItem->setData(0,PROCESS_HOST_CPU,QString(configs->GetCPUDescriptor(0)));
  clientHostItem->setData(0,PROCESS_HOST_MEM,QString(configs->GetMemoryDescriptor(0)));
  clientHostItem->setData(0,PROCESS_FQDN,QString(configs->GetFullyQualifiedDomainName(0)));
  clientHostItem->setText(0,configs->GetHostName(0));

  QTreeWidgetItem *clientRankItem=new QTreeWidgetItem(clientHostItem);
  clientRankItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientRankItem->setExpanded(true);
  clientRankItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_CLIENT_RANK));
  clientRankItem->setData(0,PROCESS_HOST_NAME,QVariant(configs->GetHostName(0)));
  clientRankItem->setData(0,PROCESS_ID,QVariant(configs->GetPid(0)));
  clientRankItem->setData(0,PROCESS_SYSTEM_TYPE,QVariant(configs->GetSystemType(0)));
  clientHostItem->setData(0,PROCESS_FQDN,QString(configs->GetFullyQualifiedDomainName(0)));
  clientRankItem->setText(0,QString("0:%1").arg(configs->GetPid(0)));
  this->Ui->configView->setItemWidget(clientRankItem,1,this->ClientRank->GetLoadWidget());

  configs->Delete();

  // servers
  QTreeWidgetItem *serverGroup
    = new QTreeWidgetItem(this->Ui->configView,QStringList("pvserver"));
  serverGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  serverGroup->setExpanded(true);
  serverGroup->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_SERVER_GROUP));
  int serverGroupSize=0;

  QTreeWidgetItem *dataServerGroup
    = new QTreeWidgetItem(this->Ui->configView,QStringList("pvdataserver"));
  dataServerGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  dataServerGroup->setExpanded(true);
  dataServerGroup->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_SERVER_GROUP));
  int dataServerGroupSize=0;

  QTreeWidgetItem *renderServerGroup
    = new QTreeWidgetItem(this->Ui->configView,QStringList("pvrenderserver"));
  renderServerGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  renderServerGroup->setExpanded(true);
  renderServerGroup->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_SERVER_GROUP));
  int renderServerGroupSize=0;

  configs=vtkPVSystemConfigInformation::New();
  server->session()->GatherInformation(vtkSMSession::SERVERS,configs,0);

  nConfigs=configs->GetSize();
  this->ServerRanks.resize(nConfigs);
  this->DataServerRanks.resize(nConfigs);
  this->RenderServerRanks.resize(nConfigs);
  for (int i=0; i<nConfigs; ++i)
    {
    // select a group on the tree
    QTreeWidgetItem *group=0;
    int *groupSize=0;
    map<string,HostData *> *hosts=0;
    vector<RankData *> *ranks=0;
    switch (configs->GetProcessType(i))
      {
      case vtkProcessModule::PROCESS_SERVER:
        group=serverGroup;
        groupSize=&serverGroupSize;
        hosts=&this->ServerHosts;
        ranks=&this->ServerRanks;
        break;

      case vtkProcessModule::PROCESS_DATA_SERVER:
        group=dataServerGroup;
        groupSize=&dataServerGroupSize;
        hosts=&this->DataServerHosts;
        ranks=&this->DataServerRanks;
        break;

      case vtkProcessModule::PROCESS_RENDER_SERVER:
        group=renderServerGroup;
        groupSize=&renderServerGroupSize;
        hosts=&this->RenderServerHosts;
        ranks=&this->RenderServerRanks;
        break;

      case vtkProcessModule::PROCESS_INVALID:
      case vtkProcessModule::PROCESS_CLIENT:
      default:
        continue;
      }
    // add this entry
    string os=configs->GetOSDescriptor(i);
    string cpu=configs->GetCPUDescriptor(i);
    string mem=configs->GetMemoryDescriptor(i);
    string hostName=configs->GetHostName(i);
    string fqdn=configs->GetFullyQualifiedDomainName(i);
    int systemType=configs->GetSystemType(i);
    int rank=configs->GetRank(i);
    unsigned long long pid=configs->GetPid(i);
    unsigned long long capacity=configs->GetCapacity(i);

    // this is useful when debugging. the client hangs and you
    // can then get server pids from the terminal output.
    cerr << rank << " " << fqdn << " " << pid << endl;

    if (pid==clientPid)
      {
      // don't create entries of server with same pid as
      // the cleint
      continue;
      }
    *groupSize+=1;

    // host
    HostData *serverHost;

    pair<string,HostData*> ins(hostName,(HostData*)0);
    pair<map<string,HostData*>::iterator,bool> ret;
    ret=hosts->insert(ins);
    if (ret.second)
      {
      // new host
      serverHost=new HostData(hostName,capacity);
      ret.first->second=serverHost;

      QTreeWidgetItem *serverHostItem=new QTreeWidgetItem(group);
      serverHostItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
      serverHostItem->setExpanded(true);
      serverHostItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_SERVER_HOST));
      serverHostItem->setData(0,PROCESS_HOST_OS,QString(os.c_str()));
      serverHostItem->setData(0,PROCESS_HOST_CPU,QString(cpu.c_str()));
      serverHostItem->setData(0,PROCESS_HOST_MEM,QString(mem.c_str()));
      serverHostItem->setData(0,PROCESS_FQDN,QString(fqdn.c_str()));
      serverHostItem->setText(0,hostName.c_str());
      this->Ui->configView->setItemWidget(serverHostItem,1,serverHost->GetLoadWidget());

      serverHost->SetTreeItem(serverHostItem);
      }
    else
      {
      serverHost=ret.first->second;
      }

    // rank
    RankData *serverRank=serverHost->AddRankData(rank,pid);
    (*ranks)[rank]=serverRank;

    QTreeWidgetItem *serverRankItem=new QTreeWidgetItem(serverHost->GetTreeItem());
    serverRankItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    serverRankItem->setExpanded(false);
    serverRankItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_SERVER_RANK));
    serverRankItem->setData(0,PROCESS_HOST_NAME,QVariant(hostName.c_str()));
    serverRankItem->setData(0,PROCESS_ID,QVariant(pid));
    serverRankItem->setData(0,PROCESS_SYSTEM_TYPE,QVariant(systemType));
    serverRankItem->setData(0,PROCESS_FQDN,QString(fqdn.c_str()));
    serverRankItem->setText(0,QString("%1:%2").arg(i).arg(pid));
    this->Ui->configView->setItemWidget(serverRankItem,1,serverRank->GetLoadWidget());
    }
  configs->Delete();

  // fix vector sizes
  this->ServerRanks.resize(serverGroupSize);
  this->DataServerRanks.resize(dataServerGroupSize);
  this->RenderServerRanks.resize(renderServerGroupSize);

  // remove empty server groups
  int serverGroupIdx=this->Ui->configView->topLevelItemCount();
  serverGroupIdx-=1;
  if (renderServerGroupSize==0)
    {
    delete this->Ui->configView->takeTopLevelItem(serverGroupIdx);
    serverGroupIdx-=1;
    }

  if (dataServerGroupSize==0)
    {
    delete this->Ui->configView->takeTopLevelItem(serverGroupIdx);
    serverGroupIdx-=1;
    }

  if (serverGroupSize==0)
    {
    delete this->Ui->configView->takeTopLevelItem(serverGroupIdx);
    serverGroupIdx-=1;
    }

  //
  this->ClientOnly=0;
  if ( (serverGroupSize==0)
    && (dataServerGroupSize==0)
    && (renderServerGroupSize==0) )
    {
    this->ClientOnly=1;
    }

  if (this->ClientOnly)
    {
    this->Ui->enableOverrideCapacity->setEnabled(false);
    this->Ui->overrideCapacity->setEnabled(false);
    #if defined(WIN32)
    this->Ui->btSignalHandler->setEnabled(false);
    #endif
    }
  else
    {
    this->Ui->enableOverrideCapacity->setEnabled(true);
    this->Ui->overrideCapacity->setEnabled(
        this->Ui->enableOverrideCapacity->isChecked());

    #if defined(WIN32)
    this->Ui->btSignalHandler->setEnabled(true);
    #endif

    // update host load to reflect all of its ranks.
    map<string,HostData*>::iterator it;
    map<string,HostData*>::iterator end;
    it=this->DataServerHosts.begin();
    end=this->DataServerHosts.end();
    while (it!=end)
      {
      it->second->UpdateLoadWidget();
      ++it;
      }

    it=this->RenderServerHosts.begin();
    end=this->RenderServerHosts.end();
    while (it!=end)
      {
      it->second->UpdateLoadWidget();
      ++it;
      }
    }

  //
  this->Ui->configView->resizeColumnToContents(0);
  this->Ui->configView->resizeColumnToContents(1);

  //
  this->Refresh();

  //
  this->Ui->openCommandDialog->setEnabled(false);
  this->Ui->openPropertiesDialog->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::Refresh()
{
  this->RefreshRanks();
  this->RefreshHosts();
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::RefreshRanks()
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::Refresh" << endl;
  #endif

  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
    {
    pqErrorMacro("failed to get active server");
    return;
    }

  // fectch latest numbers
  vtkPVMemoryUseInformation *infos=0;
  int nInfos=0;

  // client
  infos=vtkPVMemoryUseInformation::New();
  server->session()->GatherInformation(vtkSMSession::CLIENT,infos,0);

  nInfos=infos->GetSize();
  if (nInfos==0)
    {
    pqErrorMacro("failed to get client info");
    return;
    }

  this->ClientRank->SetLoad(infos->GetMemoryUse(0));
  this->ClientRank->UpdateLoadWidget();

  infos->Delete();

  if (this->ClientOnly)
    {
    return;
    }

  // servers
  infos=vtkPVMemoryUseInformation::New();
  server->session()->GatherInformation(vtkSMSession::SERVERS,infos,0);

  nInfos=infos->GetSize();
  for (int i=0; i<nInfos; ++i)
    {
    // select a group on the tree
    vector<RankData *> *ranks=0;
    switch (infos->GetProcessType(i))
      {
      case vtkProcessModule::PROCESS_SERVER:
        ranks=&this->ServerRanks;
        break;

      case vtkProcessModule::PROCESS_DATA_SERVER:
        ranks=&this->DataServerRanks;
        break;

      case vtkProcessModule::PROCESS_RENDER_SERVER:
        ranks=&this->RenderServerRanks;
        break;

      case vtkProcessModule::PROCESS_INVALID:
      case vtkProcessModule::PROCESS_CLIENT:
      default:
        continue;
      }

    // update this rank's entry
    if (ranks->size())
      {
      int rank=infos->GetRank(i);
      unsigned long long use=infos->GetMemoryUse(i);

      (*ranks)[rank]->SetLoad(use);
      (*ranks)[rank]->UpdateLoadWidget();
      }
    }
  infos->Delete();
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::RefreshHosts()
{
  this->RefreshHosts(this->ServerHosts);
  this->RefreshHosts(this->DataServerHosts);
  this->RefreshHosts(this->RenderServerHosts);
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::RefreshHosts(map<string,HostData*> &hosts)
{
  // update host laod to reflect all of its ranks.
  map<string,HostData*>::iterator it=hosts.begin();
  map<string,HostData*>::iterator end=hosts.end();
  while (it!=end)
    {
    (*it).second->UpdateLoadWidget();
    ++it;
    }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::OverrideCapacity()
{
  this->OverrideCapacity(this->ServerHosts);
  this->OverrideCapacity(this->DataServerHosts);
  this->OverrideCapacity(this->RenderServerHosts);
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::OverrideCapacity(map<string,HostData*> &hosts)
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::OverrideCapacity" << endl;
  #endif

  bool applyOverride = this->Ui->enableOverrideCapacity->isChecked();

  map<string,HostData*>::iterator it=hosts.begin();
  map<string,HostData*>::iterator end=hosts.end();

  if (applyOverride)
    {
    // capacity and usage are reported in kiB, but UI request
    // in GiB.
    unsigned long long capacity
      = this->Ui->overrideCapacity->text().toDouble()*pow(2.0,20);

    if (capacity<=0)
      {
      return; // short circuit invalid entry
      }

    // apply the constrained capacity to each host.
    while (it!=end)
      {
      (*it).second->OverrideCapacity(capacity);
      ++it;
      }
    }
  else
    {
    // reset each host to use real capacity
    while (it!=end)
      {
      (*it).second->ResetCapacity();
      ++it;
      }
    }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::EnableStackTraceSignalHandler(int enable)
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
    {
    pqErrorMacro("Failed to get the server");
    return;
    }

  if (enable)
    {
    vtkPVEnableStackTraceSignalHandler *esh
      = vtkPVEnableStackTraceSignalHandler::New();

    server->session()->GatherInformation(
        vtkSMSession::CLIENT_AND_SERVERS,esh,0);
    }
  else
    {
    vtkPVDisableStackTraceSignalHandler *dsh
      = vtkPVDisableStackTraceSignalHandler::New();

    server->session()->GatherInformation(
        vtkSMSession::CLIENT_AND_SERVERS,dsh,0);
    }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::UpdateConfigViewButtons()
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::SetExecuteButtonState" << endl;
  #endif

  this->Ui->openCommandDialog->setEnabled(false);
  this->Ui->openPropertiesDialog->setEnabled(false);

  QTreeWidgetItem *item=this->Ui->configView->currentItem();
  if (item)
    {
    bool ok;
    int type=item->data(0,PROCESS_TYPE).toInt(&ok);
    if (!ok) return;
    switch (type)
      {
      case PROCESS_TYPE_CLIENT_HOST:
      case PROCESS_TYPE_SERVER_HOST:
        this->Ui->openPropertiesDialog->setEnabled(true);
        break;

      case PROCESS_TYPE_CLIENT_RANK:
      case PROCESS_TYPE_SERVER_RANK:
        this->Ui->openCommandDialog->setEnabled(true);
        break;
      }
    }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ExecuteRemoteCommand()
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::ExecCommand" << endl;
  #endif


  QTreeWidgetItem *item=this->Ui->configView->currentItem();
  if (item)
    {
    bool ok;
    int type=item->data(0,PROCESS_TYPE).toInt(&ok);
    if (!ok) return;
    switch (type)
      {
      case PROCESS_TYPE_CLIENT_RANK:
      case PROCESS_TYPE_SERVER_RANK:
        {
        string host((const char *)item->data(0,PROCESS_HOST_NAME).toString().toAscii());
        string fqdn((const char *)item->data(0,PROCESS_FQDN).toString().toAscii());
        string pid((const char *)item->data(0,PROCESS_ID).toString().toAscii());

        int serverSystemType=item->data(0,PROCESS_SYSTEM_TYPE).toInt();

        bool localServer=(this->ClientHost->GetHostName()==host);
        if (localServer)
          {
          serverSystemType=-1;
          }

        // select and configure a command
        pqRemoteCommandDialog dialog(this,0,this->ClientSystemType,serverSystemType);

        dialog.SetActiveHost(fqdn);
        dialog.SetActivePid(pid);

        if (dialog.exec()==QDialog::Accepted)
          {
          string command=dialog.GetCommand();

          string tmp;

          QString exe;
          QStringList args;

          istringstream is(command);

          is >> tmp;
          ::unescapeWhitespace(tmp,'^');
          exe = tmp.c_str();

          while (is.good())
            {
            is >> tmp;
            ::unescapeWhitespace(tmp,'^');
            args << tmp.c_str();
            }

          QProcess *proc = new QProcess(this);
          QObject::connect(
              proc, SIGNAL(error(QProcess::ProcessError)),
              this, SLOT(RemoteCommandFailed(QProcess::ProcessError)));
          proc->setProcessChannelMode(QProcess::ForwardedChannels);
          proc->closeWriteChannel();
          proc->startDetached(exe,args);
          }
        }
        break;

      default:
        QMessageBox ebx(
            QMessageBox::Information,
            "Error",
            "No process selected. Select a specific process first by "
            "clicking on its entry in the tree view widget.");
        ebx.exec();
        break;
      }
    }
  else
    {
    QMessageBox ebx(
        QMessageBox::Information,
        "Error",
        "No process selected. Select a specific process first by "
        "clicking on its entry in the tree view widget.");
    ebx.exec();
    }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::RemoteCommandFailed(
      QProcess::ProcessError code)
{
  switch (code)
    {
  case QProcess::FailedToStart:
    qCritical() <<
      "The process failed to start. Either the invoked program is missing, "
      "or you may have insufficient permissions to invoke the program.";
    break;

  case QProcess::Crashed:
    qCritical() << "The process crashed some time after starting successfully.";
    break;

  default:
    qCritical() << "Process failed with error";
    }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ShowOnlyNodes()
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::ShowOnlyNodes" << endl;
  #endif

  this->Ui->configView->collapseAll();

  QTreeWidgetItem *item;
  QTreeWidgetItemIterator it(this->Ui->configView);
  while ((item=*it)!=(QTreeWidgetItem*)0)
    {
    bool ok;
    int type=item->data(0,PROCESS_TYPE).toInt(&ok);
    if (!ok) return;
    switch (type)
      {
      case PROCESS_TYPE_CLIENT_GROUP:
      case PROCESS_TYPE_SERVER_GROUP:
      case PROCESS_TYPE_CLIENT_HOST:
        item->setExpanded(true);
        break;

      case PROCESS_TYPE_SERVER_HOST:
        item->setExpanded(false);
        break;

      case PROCESS_TYPE_CLIENT_RANK:
      case PROCESS_TYPE_SERVER_RANK:
        break;
      }
    ++it;
    }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ShowAllRanks()
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::ShowAllRanks" << endl;
  #endif

  this->Ui->configView->expandAll();
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ShowHostPropertiesDialog()
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::ShowHostPropertiesDialog" << endl;
  #endif

  QTreeWidgetItem *item=this->Ui->configView->currentItem();
  if (!item)
    {
    return;
    }

  int type=item->data(0,PROCESS_TYPE).toInt();
  if ( (type==PROCESS_TYPE_CLIENT_HOST)
    || (type==PROCESS_TYPE_SERVER_HOST) )
    {
    QString host=item->text(0);

    QString os=item->data(0,PROCESS_HOST_OS).toString();
    QString cpu=item->data(0,PROCESS_HOST_CPU).toString();
    QString mem=item->data(0,PROCESS_HOST_MEM).toString();
    QString fqdn=item->data(0,PROCESS_FQDN).toString();

    QString descr;
    descr+="<h2>";
    descr+=(type==PROCESS_TYPE_CLIENT_HOST?"Client":"Server");
    descr+=" System Properties</h2><hr><table>";
    descr+="<tr><td><b>Host:</b></td><td>";
    descr+=fqdn;
    descr+="</td></tr>";
    descr+="<tr><td><b>OS:</b></td><td>";
    descr+=os;
    descr+="</td></tr>";
    descr+="<tr><td><b>CPU:</b></td><td>";
    descr+=cpu;
    descr+="</td></tr>";
    descr+="<tr><td><b>Memory:</b></td><td>";
    descr+=mem;
    descr+="</td></tr></table><hr>";

    QMessageBox props(QMessageBox::Information,"",descr,QMessageBox::Ok,this);
    props.exec();
    }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ConfigViewContextMenu(const QPoint &pos)
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::ConfigContextMenu" << endl;
  #endif

  QMenu context;
  context.addAction("show only nodes",this,SLOT(ShowOnlyNodes()));
  context.addAction("show all ranks",this,SLOT(ShowAllRanks()));

  QTreeWidgetItem * item=this->Ui->configView->itemAt(pos);
  if (item)
    {
    bool ok;
    int type=item->data(0,PROCESS_TYPE).toInt(&ok);
    if (!ok) return;
    switch (type)
      {
      case PROCESS_TYPE_CLIENT_GROUP:
      case PROCESS_TYPE_SERVER_GROUP:
        break;

      case PROCESS_TYPE_CLIENT_HOST:
      case PROCESS_TYPE_SERVER_HOST:
        context.addAction("properties...",this,SLOT(ShowHostPropertiesDialog()));
        break;

      case PROCESS_TYPE_CLIENT_RANK:
      case PROCESS_TYPE_SERVER_RANK:
        context.addAction("remote command...",this,SLOT(ExecuteRemoteCommand()));
        break;
      }
    }

  context.exec(this->Ui->configView->mapToGlobal(pos));
}
