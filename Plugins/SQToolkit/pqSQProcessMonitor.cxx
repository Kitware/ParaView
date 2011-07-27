/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "pqSQProcessMonitor.h"

// #define pqSQProcessMonitorDEBUG

#include "pqComponentsExport.h"
#include "pqProxy.h"
#include "pqActiveObjects.h"
#include "pqRenderView.h"

#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"

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
#include <QDebug>

#include "PrintUtils.h"
#include "FsUtils.h"
#include "SystemType.h"
#include "SystemInterface.h"
#include "SystemInterfaceFactory.h"

#include <vtkstd/map>
using std::map;
using std::pair;
#include <vtkstd/vector>
using vtkstd::vector;
#include <vtkstd/string>
using vtkstd::string;
#include <sstream>
using std::ostringstream;
using std::istringstream;
#include <iostream>
using std::cerr;
using std::endl;

enum {
  PROCESS_TYPE=Qt::UserRole,
  PROCESS_TYPE_INVALID,
  PROCESS_TYPE_CLIENT_GROUP,
  PROCESS_TYPE_CLIENT_HOST,
  PROCESS_TYPE_CLIENT_RANK,
  PROCESS_TYPE_SERVER_GROUP,
  PROCESS_TYPE_SERVER_HOST,
  PROCESS_TYPE_SERVER_RANK,
  PROCESS_ID,
  PROCESS_HOST_NAME,
  PROCESS_RANK_INVALID,
  PROCESS_RANK
};

#define SQPM_PROGBAR_MAX 1000

//*****************************************************************************
QString FormatMemUsage(float memUse)
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
void RankData::UpdateLoadWidget()
{
  float used = this->GetLoad();
  float fracUsed = this->GetLoadFraction();
  float percUsed = fracUsed*100.0;
  int progVal = fracUsed*SQPM_PROGBAR_MAX;

  this->LoadWidget->setValue(progVal);
  this->LoadWidget->setFormat(
    QString("%1 %2%").arg(FormatMemUsage(used)).arg(percUsed, 0,'f',2));

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
  this->LoadWidget->setStyle(new QPlastiqueStyle);
  this->LoadWidget->setMaximumSize(128,15);
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
  ~HostData(){ this->ClearRankData(); }

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
  ClearVectorOfPointers<RankData>(this->Ranks);
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
  this->LoadWidget->setStyle(new QPlastiqueStyle);
  this->LoadWidget->setMaximumSize(128,15);
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
    QString("%1 %2%").arg(FormatMemUsage(used)).arg(percUsed, 0,'f',2));

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
pqSQProcessMonitor::pqSQProcessMonitor(
        pqProxy* l_proxy,
        QWidget* l_parent)
  : pqNamedObjectPanel(l_proxy, l_parent)
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::pqSQProcessMonitor" << endl;
  #endif
  this->ClientOnly=0;
  this->ClientHost=0;
  this->ClientRank=0;
  this->ClientSystem=SystemInterfaceFactory::NewSystemInterface();

  this->ServerType=SYSTEM_TYPE_UNDEFINED;

  // Construct Qt form.
  this->Form=new pqSQProcessMonitorForm;
  this->Form->setupUi(this);
  // this->Form->addCommand->setIcon(QPixmap(":/pqWidgets/Icons/pqNewItem16.png"));
  // this->Form->execCommand->setIcon(QPixmap(":/pqWidgets/Icons/pqVcrPlay16.png"));
  this->Form->updateMemUse->setIcon(QPixmap(":/pqWidgets/Icons/pqRedo24.png"));
  this->Restore();

  vtkSMProxy* pmProxy=this->referenceProxy()->getProxy();

  // Set up configuration viewer
  this->PullServerConfig();

  // Connect to server side pipeline's UpdateInformation events.
  this->InformationMTime=-1;
  this->VTKConnect=vtkEventQtSlotConnect::New();
  this->VTKConnect->Connect(
      pmProxy,
      vtkCommand::UpdateInformationEvent,
      this, SLOT(UpdateInformationEvent()));
  this->UpdateInformationEvent();

  // force an update by our button
  vtkSMIntVectorProperty *infoMTimeProp
    =dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("GetInformationMTime"));
  infoMTimeProp->SetImmediateUpdate(1);

  // apply capacity override
  QObject::connect(
        this->Form->overrideCapacity,
        SIGNAL(editingFinished()),
        this,
        SLOT(OverrideCapacity()));

  QObject::connect(
        this->Form->enableOverrideCapacity,
        SIGNAL(toggled(bool)),
        this,
        SLOT(OverrideCapacity()));


  // set command add,edit,del,exec buttons
  QObject::connect(
        this->Form->execCommand,
        SIGNAL(clicked()),
        this,
        SLOT(ExecCommand()));

  // QObject::connect(
  //       this->Form->addCommand,
  //       SIGNAL(clicked()),
  //       this,
  //       SLOT(AddCommand()));

  QObject::connect(
        this->Form->delCommand,
        SIGNAL(clicked()),
        this,
        SLOT(DelCommand()));

  QObject::connect(
        this->Form->editCommand,
        SIGNAL(toggled(bool)),
        this,
        SLOT(EditCommand(bool)));

  // connect execption handing checks to pv apply button
  QObject::connect(
        this->Form->btSignalHandler,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeDisableAll,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeTrapOverflow,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeTrapUnderflow,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeTrapDivByZero,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeTrapInvalid,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  QObject::connect(
        this->Form->fpeTrapInexact,
        SIGNAL(toggled(bool)),
        this,
        SLOT(setModified()));

  //QObject::connect(
  //      this->Form->updateMemUse,
  //      SIGNAL(released()),
  //      this,
  //      SLOT(setModified()));

  QObject::connect(
        this->Form->updateMemUse,
        SIGNAL(released()),
        this,
        SLOT(UpdateServerLoad()));

  // Let the superclass do the undocumented stuff that needs to hapen.
  pqNamedObjectPanel::linkServerManagerProperties();
}

//-----------------------------------------------------------------------------
pqSQProcessMonitor::~pqSQProcessMonitor()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::~pqSQProcessMonitor" << endl;
  #endif

  this->Save();

  delete this->Form;

  this->VTKConnect->Delete();

  delete this->ClientSystem;

  this->ClearClientHost();
  this->ClearServerHosts();
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::ClearClientHost()
{
  if (this->ClientHost)
    {
    delete this->ClientHost;
    }
  this->ClientHost=0;
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::ClearServerHosts()
{
  map<string,HostData*>::iterator it=this->ServerHosts.begin();
  map<string,HostData*>::iterator end=this->ServerHosts.end();
  while (it!=end)
    {
    if ((*it).second)
      {
      delete (*it).second;
      }
    ++it;
    }
  this->ServerHosts.clear();
  this->ServerRanks.clear();
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::Restore()
{
  QStringList defaults;
  defaults
       << ""
       << "-geometry 200x40 -fg white -bg black -T @HOST@:@PID@"
       << "xterm @XTOPTS@ -e gdb --pid=@PID@"
       << "xterm @XTOPTS@ -e ssh -t @FEURL@ ssh -t @HOST@ gdb --pid=@PID@"
       << "xterm @XTOPTS@ -e ssh -t @FEURL@ ssh -t @HOST@ top"
       << "xterm @XTOPTS@ -e ssh -t @HOST@ gdb --pid=@PID@"
       << "xterm @XTOPTS@ -e ssh -t @HOST@ top"
       << "xterm -e ssh @HOST@ kill -TERM @PID@"
       << "xterm -e ssh @HOST@ kill -KILL @PID@";

  QSettings settings("SciberQuest", "SciVisToolKit");

  QStringList defs=settings.value("ProcessMonitor/defaults",defaults).toStringList();

  this->Form->feURL->setText(defs.at(0));
  this->Form->xtOpts->setText(defs.at(1));
  int n=defs.size();
  for (int i=2; i<n; ++i)
    {
    this->Form->commandCombo->addItem(defs.at(i));
    }
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::Save()
{
  QStringList defs;

  defs
    << this->Form->feURL->text()
    << this->Form->xtOpts->text();

  int nCmds=this->Form->commandCombo->count();
  for (int i=0; i<nCmds; ++i)
    {
    defs << this->Form->commandCombo->itemText(i);
    }

  QSettings settings("SciberQuest","SciVisToolKit");

  settings.setValue("ProcessMonitor/defaults",defs);
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::PullServerConfig()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::PullServerConfig" << endl;
  #endif
  vtkSMProxy* pmProxy=this->referenceProxy()->getProxy();

  // client
  QTreeWidgetItem *clientGroupItem=new QTreeWidgetItem(this->Form->configView);
  clientGroupItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientGroupItem->setExpanded(true);
  clientGroupItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_CLIENT_GROUP));
  clientGroupItem->setText(0,QString("paraview"));

  string clientHostName=this->ClientSystem->GetHostName();
  int clientPid=this->ClientSystem->GetProcessId();
  this->ClearClientHost();
  this->ClientHost
    = new HostData(clientHostName,this->ClientSystem->GetMemoryTotal());
  this->ClientRank=this->ClientHost->AddRankData(0,clientPid);
  this->ClientRank->SetLoad(this->ClientSystem->GetMemoryUsed());
  this->ClientRank->UpdateLoadWidget();

  QTreeWidgetItem *clientHostItem=new QTreeWidgetItem(clientGroupItem);
  clientHostItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientHostItem->setExpanded(true);
  clientHostItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_CLIENT_HOST));
  clientHostItem->setText(0,clientHostName.c_str());

  QTreeWidgetItem *clientRankItem=new QTreeWidgetItem(clientHostItem);
  clientRankItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientRankItem->setExpanded(true);
  clientRankItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_CLIENT_RANK));
  clientRankItem->setData(1,PROCESS_HOST_NAME,QVariant(clientHostName.c_str()));
  clientRankItem->setData(2,PROCESS_ID,QVariant(clientPid));
  clientRankItem->setText(0,QString("0:%1").arg(clientPid));
  this->Form->configView->setItemWidget(clientRankItem,1,this->ClientRank->GetLoadWidget());

  // server group
  this->ClientOnly=0;

  QTreeWidgetItem *serverGroup=new QTreeWidgetItem(this->Form->configView,QStringList("pvserver"));
  serverGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  serverGroup->setExpanded(true);
  serverGroup->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_SERVER_GROUP));

  // Pull run time configuration from server. The values are transfered
  // in the form of an ascii stream.
  vtkSMStringVectorProperty *csProp
    = dynamic_cast<vtkSMStringVectorProperty*>(pmProxy->GetProperty("ConfigStream"));
  pmProxy->UpdatePropertyInformation(csProp);
  string csBytes=csProp->GetElement(0);

  // cerr << csBytes << endl;

  istringstream is(csBytes);
  if (csBytes.size()>0 && is.good())
    {
    is >> this->ServerType;
    if (this->ServerType==SYSTEM_TYPE_WIN)
      {
      this->Form->groupSignal->hide();
      this->Form->groupFPE->hide();
      }

    int commSize;
    is >> commSize;

    this->ClearServerHosts();
    this->ServerRanks.clear();

    this->ServerRanks.resize(commSize,0);

    for (int i=0; i<commSize; ++i)
      {
      string serverHostName;
      unsigned long long serverCapacity;
      int serverPid;

      is >> serverHostName;
      is >> serverPid;
      is >> serverCapacity;

      // don't build a redundant process tree when
      // running on the builtin connection
      if (clientPid==serverPid)
        {
        this->ClientOnly=1;
        this->Form->configView->takeTopLevelItem(1);
        break;
        }

      HostData *serverHost;

      pair<string,HostData*> ins(serverHostName,0);
      pair<map<string,HostData*>::iterator,bool> ret;
      ret=this->ServerHosts.insert(ins);
      if (ret.second)
        {
        // new host
        serverHost=new HostData(serverHostName,serverCapacity);
        ret.first->second=serverHost;

        QTreeWidgetItem *serverHostItem=new QTreeWidgetItem(serverGroup);
        serverHostItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
        serverHostItem->setExpanded(true);
        serverHostItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_SERVER_HOST));
        serverHostItem->setText(0,serverHostName.c_str());
        this->Form->configView->setItemWidget(serverHostItem,1,serverHost->GetLoadWidget());

        serverHost->SetTreeItem(serverHostItem);
        }
      else
        {
        serverHost=ret.first->second;
        }
      RankData *serverRank=serverHost->AddRankData(i,serverPid);
      this->ServerRanks[i]=serverRank;

      QTreeWidgetItem *serverRankItem=new QTreeWidgetItem(serverHost->GetTreeItem());
      serverRankItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
      serverRankItem->setExpanded(false);
      serverRankItem->setData(0,PROCESS_TYPE,QVariant(PROCESS_TYPE_SERVER_RANK));
      serverRankItem->setData(1,PROCESS_HOST_NAME,QVariant(serverHostName.c_str()));
      serverRankItem->setData(2,PROCESS_ID,QVariant(serverPid));
      serverRankItem->setText(0,QString("%1:%2").arg(i).arg(serverPid));
      this->Form->configView->setItemWidget(serverRankItem,1,serverRank->GetLoadWidget());

      cerr << i << " " << serverHostName << " " << serverPid << endl;
      }
    }
  else
    {
    cerr << "Error: failed to get configuration stream. Aborting." << endl;
    }

  this->Form->configView->resizeColumnToContents(0);
  this->Form->configView->resizeColumnToContents(1);

  if (!this->ClientOnly)
    {
    // update host laod to reflect all of its ranks.
    map<string,HostData*>::iterator it=this->ServerHosts.begin();
    map<string,HostData*>::iterator end=this->ServerHosts.end();
    while (it!=end)
      {
      it->second->UpdateLoadWidget();
      ++it;
      }
    }
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::UpdateInformationEvent()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::UpdateInformationEvent" << endl;
  #endif

  this->ClientRank->SetLoad(this->ClientSystem->GetMemoryUsed());
  this->ClientRank->UpdateLoadWidget();
  if (this->ClientOnly)
    {
    return;
    }

  // see if there has been an update to the server side.
  vtkSMProxy* pmProxy=this->referenceProxy()->getProxy();

  vtkSMIntVectorProperty *infoMTimeProp
    =dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("GetInformationMTime"));
  pmProxy->UpdatePropertyInformation(infoMTimeProp);
  int infoMTime=infoMTimeProp->GetElement(0);

  // cerr << "infoMTime=" << infoMTime << endl;

  if (infoMTime>this->InformationMTime)
    {
    this->InformationMTime=infoMTime;

    vtkSMStringVectorProperty *memProp
      =dynamic_cast<vtkSMStringVectorProperty *>(pmProxy->GetProperty("MemoryUseStream"));

    pmProxy->UpdatePropertyInformation(memProp);

    string stream=memProp->GetElement(0);
    istringstream is(stream);

    // cerr << stream << endl;

    if (stream.size()>0 && is.good())
      {
      int commSize;
      is >> commSize;

      for (int i=0; i<commSize; ++i)
        {
        unsigned long long memUse;
        is >> memUse;
        this->ServerRanks[i]->SetLoad(memUse);
        this->ServerRanks[i]->UpdateLoadWidget();
        }
      // update host laod to reflect all of its ranks.
      map<string,HostData*>::iterator it=this->ServerHosts.begin();
      map<string,HostData*>::iterator end=this->ServerHosts.end();
      while (it!=end)
        {
        (*it).second->UpdateLoadWidget();
        ++it;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::OverrideCapacity()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::OverrideCapacity" << endl;
  #endif

  bool applyOverride = this->Form->enableOverrideCapacity->isChecked();

  map<string,HostData*>::iterator it=this->ServerHosts.begin();
  map<string,HostData*>::iterator end=this->ServerHosts.end();

  if (applyOverride)
    {
    // capacity and usage are reported in kiB, but UI request
    // in GiB.
    unsigned long long capacity
      = this->Form->overrideCapacity->text().toDouble()*pow(2.0,20);

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
void pqSQProcessMonitor::IncrementInformationMTime()
{
  vtkSMProxy* pmProxy=this->referenceProxy()->getProxy();

  ++this->InformationMTime;

  vtkSMIntVectorProperty *infoMTimeProp
    =dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("GetInformationMTime"));
  infoMTimeProp->SetElement(0,this->InformationMTime);

  pmProxy->UpdateVTKObjects();
//   pqNamedObjectPanel::accept();
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::AddCommand()
{
//   int idx=this->Form->commandCombo->count();
//   this->Form->commandCombo->addItem("NEW COMMAND");
//   this->Form->commandCombo->setCurrentIndex(idx);
//   if (!this->Form->editCommand->isChecked())
//     {
//     this->Form->editCommand->click();
//     }
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::DelCommand()
{
  int idx=this->Form->commandCombo->currentIndex();
  this->Form->commandCombo->removeItem(idx);
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::EditCommand(bool state)
{
  this->Form->commandCombo->setEditable(state);
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::ExecCommand()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::ExecCommand" << endl;
  #endif

  QTreeWidgetItem *item=this->Form->configView->currentItem();
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
        string cmd=(const char*)this->Form->commandCombo->currentText().toAscii();

        string feURLStr(this->Form->feURL->text().toAscii());
        SearchAndReplace(string("@FEURL@"),feURLStr,cmd);

        string xtOptsStr(this->Form->xtOpts->text().toAscii());
        SearchAndReplace(string("@XTOPTS@"),xtOptsStr,cmd);

        string hostNameStr((const char *)item->data(1,PROCESS_HOST_NAME).toString().toAscii());
        SearchAndReplace(string("@HOST@"),hostNameStr,cmd);

        string pidStr((const char *)item->data(2,PROCESS_ID).toString().toAscii());
        SearchAndReplace(string("@PID@"),pidStr,cmd);

        int iErr=this->ClientSystem->Exec(cmd);
        if (iErr)
          {
          QMessageBox ebx;
          ebx.setText("Exec failed.");
          ebx.exec();
          return;
          }
        }
        break;
      default:
        QMessageBox ebx;
        ebx.setText("Could not exec. No process selected.");
        ebx.exec();
        return;
        break;
      }
    }
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::accept()
{
  #if defined pqSQProcessMonitorDEBUG
  cerr << ":::::::::::::::::::::::::::::::pqSQProcessMonitor::accept" << endl;
  #endif

  // Let our superclass do the undocumented stuff that needs to be done.
  pqNamedObjectPanel::accept();


  vtkSMProxy* pmProxy=this->referenceProxy()->getProxy();

  vtkSMIntVectorProperty *prop=0;

  prop=dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("EnableBacktraceHandler"));
  prop->SetElement(0,this->Form->btSignalHandler->isChecked());
  //pmProxy->UpdateProperty("EnableBacktraceHandler");


  bool disableFPE=this->Form->fpeDisableAll->isChecked();
  if (disableFPE)
    {
    prop=dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("EnableFE_ALL"));
    prop->SetElement(0,0);
    prop->Modified();
    }
  else
    {
    prop=dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("EnableFE_DIVBYZERO"));
    prop->SetElement(0,this->Form->fpeTrapDivByZero->isChecked());
    prop->Modified(); // force push

    prop=dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("EnableFE_INEXACT"));
    prop->SetElement(0,this->Form->fpeTrapInexact->isChecked());
    prop->Modified();

    prop=dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("EnableFE_INVALID"));
    prop->SetElement(0,this->Form->fpeTrapInvalid->isChecked());
    prop->Modified();

    prop=dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("EnableFE_OVERFLOW"));
    prop->SetElement(0,this->Form->fpeTrapOverflow->isChecked());
    prop->Modified();

    prop=dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("EnableFE_UNDERFLOW"));
    prop->SetElement(0,this->Form->fpeTrapUnderflow->isChecked());
    prop->Modified();
    }

  // force a refresh
  ++this->InformationMTime;
  prop=dynamic_cast<vtkSMIntVectorProperty *>(pmProxy->GetProperty("SetInformationMTime"));
  prop->SetElement(0,this->InformationMTime);
  prop->Modified();

  pmProxy->UpdateVTKObjects();

  // mark dirty
  // The idea was that if it was dirty then it would be
  // updated when the puipeline ran providing an autoomated
  // reporting mechanism but that didn't hold true, when
  // advancing through timesteps.
  // this->setModified();
}

//-----------------------------------------------------------------------------
void pqSQProcessMonitor::UpdateServerLoad()
{
  this->setModified();
  this->accept();

  pqRenderView* renderView =
    qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());

  renderView->render();
}




/// VTK stuffs
//   // Connect to server side pipeline's UpdateInformation events.
//   this->VTKConnect=vtkEventQtSlotConnect::New();
//   this->VTKConnect->Connect(
//       dbbProxy,
//       vtkCommand::UpdateInformationEvent,
//       this, SLOT(UpdateInformationEvent()));
//   // Get our initial state from the server side. In server side RequestInformation
//   // the database view is encoded in vtkInformationObject. We are relying on the 
//   // fact that there is a pending event waiting for us.
//   this->UpdateInformationEvent();

// 
//   // These connection let PV know that we have changed, and makes the apply button
//   // is activated.
//   QObject::connect(
//       this->Form->DatabaseView,
//       SIGNAL(itemChanged(QTreeWidgetItem*, int)),
//       this, SLOT(setModified()));

// vtkSMProxy* dbbProxy=this->referenceProxy()->getProxy();
/// Example of how to get stuff from the server side.
// vtkSMIntVectorProperty *serverDVMTimeProp
//   =dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("DatabaseViewMTime"));
// dbbProxy->UpdatePropertyInformation(serverDVMTimeProp);
// const int *serverDVMTime=dvmtProp->GetElement(0);
/// Example of how to get stuff from the XML configuration.
// vtkSMStringVectorProperty *ppProp
//   =dynamic_cast<vtkSMStringVectorProperty *>(dbbProxy->GetProperty("PluginPath"));
// const char *pp=ppProp->GetElement(0);
// this->Form->PluginPath->setText(pp);
/// Imediate update example.
// These are bad because it gets updated more than when you call
// modified, see the PV guide.
//  iuiProp->SetImmediateUpdate(1); set this in constructor
// vtkSMProperty *iuiProp=dbbProxy->GetProperty("InitializeUI");
// iuiProp->Modified();
/// Do some changes and force a push.
// vtkSMIntVectorProperty *meshProp
//   = dynamic_cast<vtkSMIntVectorProperty *>(dbbProxy->GetProperty("PushMeshId"));
// meshProp->SetElement(meshIdx,meshId);
// ++meshIdx;
// dbbProxy->UpdateProperty("PushMeshId");
/// Catch all, update anything else that may need updating.
// dbbProxy->UpdateVTKObjects();
/// How a wiget in custom panel tells PV that things need to be applied
// QObject::connect(
//     this->Form->DatabaseView,
//     SIGNAL(itemChanged(QTreeWidgetItem*, int)),
//     this, SLOT(setModified()));
  // Pull run time configuration from server. The values are transfered
  // in the form of an ascii stream.
//   vtkSMIntVectorProperty *pidProp
//     = dynamic_cast<vtkSMIntVectorProperty*>(pmProxy->GetProperty("Pid"));
//   pmProxy->UpdatePropertyInformation(pidProp);
//   pid_t p=pidProp->GetElement(0);
//   cerr << "PID=" << p << endl;
/*
  vtkSMProxy* reader = this->referenceProxy()->getProxy();
  reader->UpdatePropertyInformation(reader->GetProperty("Pid"));
  int stamp = vtkSMPropertyHelper(reader, "Pid").GetAsInt();
  cerr << stamp;*/
/// how to get something from the server.
// dbbProxy->UpdatePropertyInformation(reader->GetProperty("SILUpdateStamp"));
// int stamp = vtkSMPropertyHelper(dbbProxy, "SILUpdateStamp").GetAsInt();
