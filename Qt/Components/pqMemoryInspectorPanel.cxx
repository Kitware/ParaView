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

// print a process table to stderr during initialization
// #define MIP_PROCESS_TABLE

#include "pqActiveObjects.h"

#include "vtkProcessModule.h"
#include "vtkPVSystemConfigInformation.h"
#include "vtkPVMemoryUseInformation.h"
#include "vtkPVEnableStackTraceSignalHandler.h"
#include "vtkPVDisableStackTraceSignalHandler.h"
#include "vtkSMSession.h"
#include "vtkSMSessionClient.h"
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
using std::left;
using std::right;
#include <iomanip>
using std::setw;
using std::setfill;

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
#define MIP_PROGBAR_MAX 1000

// keys for tree items
enum {
  ITEM_KEY_PROCESS_TYPE=Qt::UserRole,
  ITEM_KEY_PVSERVER_TYPE,      // vtkProcessModule::ProcessTypes
  ITEM_KEY_PID,                // unsigned long long
  ITEM_KEY_HOST_NAME,          // string, hostname
  ITEM_KEY_MPI_RANK,           // int
  ITEM_KEY_HOST_OS,            // descriptive string
  ITEM_KEY_HOST_CPU,           // descriptive string
  ITEM_KEY_HOST_MEM,           // descriptive string
  ITEM_KEY_FQDN,               // string, network address
  ITEM_KEY_SYSTEM_TYPE         // int (0 unix like, 1 win)
};

// data for tree items
enum {
  ITEM_DATA_CLIENT_GROUP,
  ITEM_DATA_CLIENT_HOST,
  ITEM_DATA_CLIENT_RANK,
  ITEM_DATA_SERVER_GROUP,
  ITEM_DATA_SERVER_HOST,
  ITEM_DATA_SERVER_RANK,
  ITEM_DATA_UNIX_SYSTEM=0,
  ITEM_DATA_WIN_SYSTEM=1
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
      data[i]=NULL;
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
      unsigned long long pid,
      unsigned long long load,
      unsigned long long capacity);

  ~RankData();

  void SetRank(int rank){ this->Rank=rank; }
  int GetRank(){ return this->Rank; }

  void SetPid(unsigned long long pid){ this->Pid=pid; }
  unsigned long long GetPid(){ return this->Pid; }

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

  void Print(ostream &os);

private:
  int Rank;
  unsigned long long Pid;
  unsigned long long Load;
  unsigned long long Capacity;
  QProgressBar *LoadWidget;
  //HostData *Host;
};

//-----------------------------------------------------------------------------
RankData::RankData(
      int rank,
      unsigned long long pid,
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
  float used = (float)this->GetLoad();
  float fracUsed = this->GetLoadFraction();
  float percUsed = fracUsed*100.0f;
  int progVal = (int)(fracUsed*MIP_PROGBAR_MAX);

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
  this->LoadWidget->setMaximum(MIP_PROGBAR_MAX);
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

//-----------------------------------------------------------------------------
void RankData::Print(ostream &os)
{
  os
    << "RankData(" << this << ")"
    << " Rank=" << this->Rank
    << " Pid=" << this->Pid
    << " Load=" << this->Load
    << " Capacity=" << this->Capacity
    << " LoadWidget=" << this->LoadWidget
    << endl;
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

  RankData *AddRankData(int rank, unsigned long long pid);
  RankData *GetRankData(int i){ return this->Ranks[i]; }
  void ClearRankData();

  unsigned long long GetLoad();
  float GetLoadFraction(){ return (float)this->GetLoad()/(float)this->Capacity; }

  void Print(ostream &os);

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
RankData *HostData::AddRankData(int rank, unsigned long long pid)
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
  this->LoadWidget->setMaximum(MIP_PROGBAR_MAX);
  QFont font(this->LoadWidget->font());
  font.setPointSize(8);
  this->LoadWidget->setFont(font);

  this->UpdateLoadWidget();
}

//-----------------------------------------------------------------------------
void HostData::UpdateLoadWidget()
{
  float used = (float)this->GetLoad();
  float fracUsed = this->GetLoadFraction();
  float percUsed = fracUsed*100.0f;
  int progVal = (int)(fracUsed*MIP_PROGBAR_MAX);

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
void HostData::Print(ostream &os)
{
  os
    << "HostData(" << this << ")"
    << " HostName=" << this->HostName
    << " Capacity=" << this->Capacity
    << " RealCapacity=" << this->RealCapacity
    << " LoadWidget=" << this->LoadWidget
    << " TreeItem=" << this->TreeItem
    << " Ranks=" << endl;
  size_t nRanks=this->Ranks.size();
  for (size_t i=0; i<nRanks; ++i)
    {
    this->Ranks[i]->Print(os);
    }
  os << endl;
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
      (*it).second=NULL;
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
void pqMemoryInspectorPanel::InitializeServerGroup(
    unsigned long long clientPid,
    vtkPVSystemConfigInformation *configs,
    int validProcessType,
    QTreeWidgetItem *group,
    map<string,HostData*> &hosts,
    vector<RankData*> &ranks,
    int &systemType)
{
  size_t nConfigs=configs->GetSize();
  for (size_t i=0; i<nConfigs; ++i)
    {
    // assume that we get what we ask for and nothing more
    // but verify that it's not the builtin
    unsigned long long pid=configs->GetPid(i);
    if (pid==clientPid)
      {
      // must be the builtin, do not duplicate.
      continue;
      }
    int processType=configs->GetProcessType(i);
    if (processType!=validProcessType)
      {
      // it's not the correct type.
      continue;
      }
    string os=configs->GetOSDescriptor(i);
    string cpu=configs->GetCPUDescriptor(i);
    string mem=configs->GetMemoryDescriptor(i);
    string hostName=configs->GetHostName(i);
    string fqdn=configs->GetFullyQualifiedDomainName(i);
    systemType=configs->GetSystemType(i);
    int rank=configs->GetRank(i);
    unsigned long long capacity=configs->GetCapacity(i);

    // it's useful to have hostname's rank's and pid's in
    // the terminal. if the client hangs you can attach
    // gdb and see where it's stuck without a lot of effort

    #ifdef MIP_PROCESS_TABLE
    cerr
      << setw(32) << fqdn
      << setw(16) << pid
      << setw(8)  << rank << endl
      << setw(1);
    #endif

    // host
    HostData *serverHost=NULL;

    pair<string,HostData*> ins(hostName,(HostData*)0);
    pair<map<string,HostData*>::iterator,bool> ret;
    ret=hosts.insert(ins);
    if (ret.second)
      {
      // new host
      serverHost=new HostData(hostName,capacity);
      ret.first->second=serverHost;

      QTreeWidgetItem *serverHostItem=new QTreeWidgetItem(group);
      serverHostItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
      serverHostItem->setExpanded(true);
      serverHostItem->setData(0,ITEM_KEY_PROCESS_TYPE,QVariant(ITEM_DATA_SERVER_HOST));
      serverHostItem->setData(0,ITEM_KEY_PVSERVER_TYPE,QVariant(processType));
      serverHostItem->setData(0,ITEM_KEY_SYSTEM_TYPE,QVariant(systemType));
      serverHostItem->setData(0,ITEM_KEY_HOST_OS,QString(os.c_str()));
      serverHostItem->setData(0,ITEM_KEY_HOST_CPU,QString(cpu.c_str()));
      serverHostItem->setData(0,ITEM_KEY_HOST_MEM,QString(mem.c_str()));
      serverHostItem->setData(0,ITEM_KEY_FQDN,QString(fqdn.c_str()));
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
    ranks.push_back(serverRank);

    QTreeWidgetItem *serverRankItem=new QTreeWidgetItem(serverHost->GetTreeItem());
    serverRankItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    serverRankItem->setExpanded(false);
    serverRankItem->setData(0,ITEM_KEY_PROCESS_TYPE,QVariant(ITEM_DATA_SERVER_RANK));
    serverRankItem->setData(0,ITEM_KEY_PVSERVER_TYPE,QVariant(processType));
    serverRankItem->setData(0,ITEM_KEY_HOST_NAME,QVariant(hostName.c_str()));
    serverRankItem->setData(0,ITEM_KEY_PID,QVariant(pid));
    serverRankItem->setData(0,ITEM_KEY_SYSTEM_TYPE,QVariant(systemType));
    serverRankItem->setData(0,ITEM_KEY_FQDN,QString(fqdn.c_str()));
    serverRankItem->setText(0,QString("%1:%2").arg(rank).arg(pid));
    this->Ui->configView->setItemWidget(serverRankItem,1,serverRank->GetLoadWidget());
    }

  // prune off the group if there were none added.
  if (group->childCount()==0)
    {
    int idx=this->Ui->configView->indexOfTopLevelItem(group);
    if (idx>=0)
      {
      this->Ui->configView->takeTopLevelItem(idx);
      }
    }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::Initialize()
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::Initialize" << endl;
  #endif

  this->ClearClient();
  this->ClearServers();

  this->ClientSystemType=0;
  this->ServerSystemType=0;
  this->DataServerSystemType=0;
  this->RenderServerSystemType=0;

  this->StackTraceOnClient=0;
  this->StackTraceOnServer=0;
  this->StackTraceOnDataServer=0;
  this->StackTraceOnRenderServer=0;

  this->Ui->configView->clear();

  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
    {
    // this is not necessarilly an error as the panel may be created
    // before the server is connected.
    return;
    }


  // client
  QTreeWidgetItem *clientGroup
    = new QTreeWidgetItem(this->Ui->configView,QStringList("paraview"));
  clientGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientGroup->setExpanded(true);
  clientGroup->setData(0,ITEM_KEY_PROCESS_TYPE,QVariant(ITEM_DATA_CLIENT_GROUP));

  vtkSMSession *session=NULL;
  vtkPVSystemConfigInformation *configs=NULL;

  configs=vtkPVSystemConfigInformation::New();
  session=server->session();
  session->GatherInformation(vtkSMSession::CLIENT,configs,0);

  size_t nConfigs=configs->GetSize();
  if (nConfigs!=1)
    {
    pqErrorMacro("There should always be one client.");
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
  clientHostItem->setData(0,ITEM_KEY_PROCESS_TYPE,QVariant(ITEM_DATA_CLIENT_HOST));
  clientHostItem->setData(0,ITEM_KEY_PVSERVER_TYPE,QVariant(configs->GetProcessType(0)));
  clientHostItem->setData(0,ITEM_KEY_SYSTEM_TYPE,QVariant(configs->GetSystemType(0)));
  clientHostItem->setData(0,ITEM_KEY_HOST_OS,QString(configs->GetOSDescriptor(0)));
  clientHostItem->setData(0,ITEM_KEY_HOST_CPU,QString(configs->GetCPUDescriptor(0)));
  clientHostItem->setData(0,ITEM_KEY_HOST_MEM,QString(configs->GetMemoryDescriptor(0)));
  clientHostItem->setData(0,ITEM_KEY_FQDN,QString(configs->GetFullyQualifiedDomainName(0)));
  clientHostItem->setText(0,configs->GetHostName(0));

  QTreeWidgetItem *clientRankItem=new QTreeWidgetItem(clientHostItem);
  clientRankItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientRankItem->setExpanded(true);
  clientRankItem->setData(0,ITEM_KEY_PROCESS_TYPE,QVariant(ITEM_DATA_CLIENT_RANK));
  clientRankItem->setData(0,ITEM_KEY_HOST_NAME,QVariant(configs->GetHostName(0)));
  clientRankItem->setData(0,ITEM_KEY_PID,QVariant(configs->GetPid(0)));
  clientRankItem->setData(0,ITEM_KEY_SYSTEM_TYPE,QVariant(configs->GetSystemType(0)));
  clientHostItem->setData(0,ITEM_KEY_FQDN,QString(configs->GetFullyQualifiedDomainName(0)));
  clientRankItem->setText(0,QString("0:%1").arg(configs->GetPid(0)));
  this->Ui->configView->setItemWidget(clientRankItem,1,this->ClientRank->GetLoadWidget());

  configs->Delete();

  #ifdef MIP_PROCESS_TABLE
  // print a process table to the terminal.
  cerr
    << endl
    << setw(32) << "Host"
    << setw(16) << "Pid"
    << setw(8)  << "Rank"
    << endl
    << left << setw(56) << setfill('=') << "client" << endl
    << right<< setw(1)  << setfill(' ')
    << setw(32) << configs->GetFullyQualifiedDomainName(0)
    << setw(16) << configs->GetPid(0)
    << setw(8)  << "x" << endl
    << setfill(' ') << setw(1);
  #endif

  // collect info from the server(s)

  configs=vtkPVSystemConfigInformation::New();

  vtkPVSystemConfigInformation *dsconfigs=vtkPVSystemConfigInformation::New();
  session->GatherInformation(vtkPVSession::DATA_SERVER,dsconfigs,0);
  configs->AddInformation(dsconfigs);
  dsconfigs->Delete();

  // don't attempt to communicate with a render server if it's
  // not connected which results in a duplicated gather as in that
  // case comm is routed to the data server.
  if (session->GetRenderClientMode()==vtkSMSession::RENDERING_SPLIT)
    {
    vtkPVSystemConfigInformation *rsconfigs=vtkPVSystemConfigInformation::New();
    session->GatherInformation(vtkPVSession::RENDER_SERVER,rsconfigs,0);
    configs->AddInformation(rsconfigs);
    rsconfigs->Delete();
    }

  nConfigs=configs->GetSize();
  if (nConfigs>0)
    {

    // servers
    #ifdef MIP_PROCESS_TABLE
    cerr
      << left << setw(56) << setfill('=') << "server" << endl
      << right << setw(1)  << setfill(' ');
    #endif
    QTreeWidgetItem *group=NULL;
    group = new QTreeWidgetItem(QStringList("pvserver"));
    group->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    group->setExpanded(true);
    group->setData(0,ITEM_KEY_PROCESS_TYPE,QVariant(ITEM_DATA_SERVER_GROUP));
    this->Ui->configView->addTopLevelItem(group);

    this->InitializeServerGroup(
        clientPid,
        configs,
        vtkProcessModule::PROCESS_SERVER,
        group,
        this->ServerHosts,
        this->ServerRanks,
        this->ServerSystemType);

    // dataservers
    #ifdef MIP_PROCESS_TABLE
    cerr
      << left << setw(56) << setfill('=') << "data server" << endl
      << right << setw(1)  << setfill(' ');
    #endif
    group = new QTreeWidgetItem(QStringList("pvdataserver"));
    group->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    group->setExpanded(true);
    group->setData(0,ITEM_KEY_PROCESS_TYPE,QVariant(ITEM_DATA_SERVER_GROUP));
    this->Ui->configView->addTopLevelItem(group);

    this->InitializeServerGroup(
        clientPid,
        configs,
        vtkProcessModule::PROCESS_DATA_SERVER,
        group,
        this->DataServerHosts,
        this->DataServerRanks,
        this->DataServerSystemType);

    // renderservers
    #ifdef MIP_PROCESS_TABLE
    cerr
      << left << setw(56) << setfill('=') << "render server" << endl
      << right << setw(1)  << setfill(' ');
    #endif
    group = new QTreeWidgetItem(QStringList("pvrenderserver"));
    group->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    group->setExpanded(true);
    group->setData(0,ITEM_KEY_PROCESS_TYPE,QVariant(ITEM_DATA_SERVER_GROUP));
    this->Ui->configView->addTopLevelItem(group);

    this->InitializeServerGroup(
        clientPid,
        configs,
        vtkProcessModule::PROCESS_RENDER_SERVER,
        group,
        this->RenderServerHosts,
        this->RenderServerRanks,
        this->RenderServerSystemType);

    #ifdef MIP_PROCESS_TABLE
    cerr << setw(56) << setfill('=') << "=" << endl << setw(1) << setfill(' ');
    #endif
    }
  configs->Delete();

  //
  this->ClientOnly=0;
  if ( (this->RenderServerHosts.size()==0)
    && (this->DataServerHosts.size()==0)
    && (this->ServerHosts.size()==0))
    {
    this->ClientOnly=1;
    this->Ui->enableOverrideCapacity->setEnabled(false);
    this->Ui->overrideCapacity->setEnabled(false);
    }
  else
    {
    this->Ui->enableOverrideCapacity->setEnabled(true);
    this->Ui->overrideCapacity->setEnabled(
         this->Ui->enableOverrideCapacity->isChecked());

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
  this->ShowAllRanks();
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
  vtkSMSession *session=NULL;
  vtkPVMemoryUseInformation *infos=NULL;
  size_t nInfos=0;

  // client
  infos=vtkPVMemoryUseInformation::New();
  session=server->session();
  session->GatherInformation(vtkSMSession::CLIENT,infos,0);

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

  vtkPVMemoryUseInformation *dsinfos=vtkPVMemoryUseInformation::New();
  session->GatherInformation(vtkPVSession::DATA_SERVER,dsinfos,0);
  infos->AddInformation(dsinfos);
  dsinfos->Delete();

  // don't attempt to communicate with a render server if it's
  // not connected which results in a duplicated gather as in that
  // case comm is routed to the data server.
  if (session->GetRenderClientMode()==vtkSMSession::RENDERING_SPLIT)
    {
    vtkPVMemoryUseInformation *rsinfos=vtkPVMemoryUseInformation::New();
    session->GatherInformation(vtkPVSession::RENDER_SERVER,rsinfos,0);
    infos->AddInformation(rsinfos);
    rsinfos->Delete();
    }


  nInfos=infos->GetSize();
  for (size_t i=0; i<nInfos; ++i)
    {
    int rank=infos->GetRank((int)i);
    unsigned long long use=infos->GetMemoryUse((int)i);

    switch (infos->GetProcessType((int)i))
      {
      case vtkProcessModule::PROCESS_SERVER:
        if (this->ServerRanks.size())
          {
          this->ServerRanks[rank]->SetLoad(use);
          this->ServerRanks[rank]->UpdateLoadWidget();
          }
        break;

      case vtkProcessModule::PROCESS_DATA_SERVER:
        if (this->DataServerRanks.size())
          {
          this->DataServerRanks[rank]->SetLoad(use);
          this->DataServerRanks[rank]->UpdateLoadWidget();
          }
        break;

      case vtkProcessModule::PROCESS_RENDER_SERVER:
        if (this->RenderServerRanks.size())
          {
          this->RenderServerRanks[rank]->SetLoad(use);
          this->RenderServerRanks[rank]->UpdateLoadWidget();
          }
        break;

      case vtkProcessModule::PROCESS_INVALID:
      case vtkProcessModule::PROCESS_CLIENT:
      default:
        continue;
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
      = (unsigned long long)(this->Ui->overrideCapacity->text().toDouble()*pow(2.0,20));

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
void pqMemoryInspectorPanel::EnableStackTraceOnClient(bool enable)
{
  this->EnableStackTrace(enable,vtkPVSession::CLIENT);
  this->StackTraceOnClient=enable;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::EnableStackTraceOnServer(bool enable)
{
  this->EnableStackTrace(enable,vtkPVSession::DATA_SERVER);
  this->StackTraceOnServer=enable;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::EnableStackTraceOnDataServer(bool enable)
{
  this->EnableStackTrace(enable,vtkPVSession::DATA_SERVER);
  this->StackTraceOnDataServer=enable;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::EnableStackTraceOnRenderServer(bool enable)
{
  this->EnableStackTrace(enable,vtkPVSession::RENDER_SERVER);
  this->StackTraceOnRenderServer=enable;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::EnableStackTrace(bool enable, int group)
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

    server->session()->GatherInformation(group,esh,0);
    }
  else
    {
    vtkPVDisableStackTraceSignalHandler *dsh
      = vtkPVDisableStackTraceSignalHandler::New();

    server->session()->GatherInformation(group,dsh,0);
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
    int type=item->data(0,ITEM_KEY_PROCESS_TYPE).toInt(&ok);
    if (!ok) return;
    switch (type)
      {
      case ITEM_DATA_CLIENT_RANK:
      case ITEM_DATA_SERVER_RANK:
        {
        string host((const char *)item->data(0,ITEM_KEY_HOST_NAME).toString().toAscii());
        string fqdn((const char *)item->data(0,ITEM_KEY_FQDN).toString().toAscii());
        string pid((const char *)item->data(0,ITEM_KEY_PID).toString().toAscii());

        int serverSystemType=item->data(0,ITEM_KEY_SYSTEM_TYPE).toInt();

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
    int type=item->data(0,ITEM_KEY_PROCESS_TYPE).toInt(&ok);
    if (!ok) return;
    switch (type)
      {
      case ITEM_DATA_CLIENT_GROUP:
      case ITEM_DATA_SERVER_GROUP:
      case ITEM_DATA_CLIENT_HOST:
        item->setExpanded(true);
        break;

      case ITEM_DATA_SERVER_HOST:
        item->setExpanded(false);
        break;

      case ITEM_DATA_CLIENT_RANK:
      case ITEM_DATA_SERVER_RANK:
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

  int type=item->data(0,ITEM_KEY_PROCESS_TYPE).toInt();
  if ( (type==ITEM_DATA_CLIENT_HOST)
    || (type==ITEM_DATA_SERVER_HOST) )
    {
    QString host=item->text(0);

    QString os=item->data(0,ITEM_KEY_HOST_OS).toString();
    QString cpu=item->data(0,ITEM_KEY_HOST_CPU).toString();
    QString mem=item->data(0,ITEM_KEY_HOST_MEM).toString();
    QString fqdn=item->data(0,ITEM_KEY_FQDN).toString();

    QString descr;
    descr+="<h2>";
    descr+=(type==ITEM_DATA_CLIENT_HOST?"Client":"Server");
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
void pqMemoryInspectorPanel::ConfigViewContextMenu(const QPoint &position)
{
  #if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::ConfigContextMenu" << endl;
  #endif

  QMenu context;

  QTreeWidgetItem * item=this->Ui->configView->itemAt(position);
  if (item)
    {
    bool ok;
    int procType=item->data(0,ITEM_KEY_PROCESS_TYPE).toInt(&ok);
    if (!ok) { return; }
    switch(procType)
      {
      case ITEM_DATA_CLIENT_GROUP:
        {
        QTreeWidgetItem *child=item->child(0);
        if (child==NULL) return;
        int sysType=child->data(0,ITEM_KEY_SYSTEM_TYPE).toInt(&ok);
        if (!ok) return;
        int serverType=child->data(0,ITEM_KEY_PVSERVER_TYPE).toInt(&ok);
        if (!ok) return;
        if (sysType==ITEM_DATA_UNIX_SYSTEM)
          {
          this->AddEnableStackTraceMenuAction(serverType,context);
          }
        }
        break;

      case ITEM_DATA_SERVER_GROUP:
        {
        context.addAction("show only nodes",this,SLOT(ShowOnlyNodes()));
        context.addAction("show all ranks",this,SLOT(ShowAllRanks()));

        QTreeWidgetItem *child=item->child(0);
        if (child==NULL) return;
        int sysType=child->data(0,ITEM_KEY_SYSTEM_TYPE).toInt(&ok);
        if (!ok) return;
        int serverType=child->data(0,ITEM_KEY_PVSERVER_TYPE).toInt(&ok);
        if (!ok) return;
        if (sysType==ITEM_DATA_UNIX_SYSTEM)
          {
          this->AddEnableStackTraceMenuAction(serverType,context);
          }
        }
        break;

      case ITEM_DATA_CLIENT_HOST:
      case ITEM_DATA_SERVER_HOST:
        {
        context.addAction("properties...",this,SLOT(ShowHostPropertiesDialog()));
        }
        break;

      case ITEM_DATA_CLIENT_RANK:
      case ITEM_DATA_SERVER_RANK:
        {
        int sysType=item->data(0,ITEM_KEY_SYSTEM_TYPE).toInt(&ok);
        if (!ok) return;
        if (sysType==ITEM_DATA_UNIX_SYSTEM)
          {
          context.addAction("remote command...",this,SLOT(ExecuteRemoteCommand()));
          }
        }
        break;
      }
    }

  context.exec(this->Ui->configView->mapToGlobal(position));
}
//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::AddEnableStackTraceMenuAction(
      int serverType,
      QMenu &context)
{
  QAction *action=new QAction(this);
  action->setText("stack trace signal handler");
  action->setCheckable(true);
  switch (serverType)
    {
    case vtkProcessModule::PROCESS_CLIENT:
      action->setChecked(this->StackTraceOnClient);
      connect(action,
            SIGNAL(toggled(bool)),
            this,
            SLOT(EnableStackTraceOnClient(bool)));
      break;

    case vtkProcessModule::PROCESS_SERVER:
      action->setChecked(this->StackTraceOnServer);
      connect(action,
            SIGNAL(toggled(bool)),
            this,
            SLOT(EnableStackTraceOnServer(bool)));
      break;

    case vtkProcessModule::PROCESS_DATA_SERVER:
      action->setChecked(this->StackTraceOnDataServer);
      connect(action,
            SIGNAL(toggled(bool)),
            this,
            SLOT(EnableStackTraceOnDataServer(bool)));
      break;

    case vtkProcessModule::PROCESS_RENDER_SERVER:
      action->setChecked(this->StackTraceOnRenderServer);
      connect(action,
            SIGNAL(toggled(bool)),
            this,
            SLOT(EnableStackTraceOnRenderServer(bool)));
      break;
    }
  context.addAction(action);
}
