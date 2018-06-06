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
#include "pqApplicationCore.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkSMRenderViewProxy.h"

#include "vtkClientServerStream.h"
#include "vtkPVDisableStackTraceSignalHandler.h"
#include "vtkPVEnableStackTraceSignalHandler.h"
#include "vtkPVInformation.h"
#include "vtkPVMemoryUseInformation.h"
#include "vtkPVSystemConfigInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMSession.h"
#include "vtkSMSessionClient.h"

#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPalette>
#include <QPoint>
#include <QProcess>
#include <QProgressBar>
#include <QProxyStyle>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QStyleFactory>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>

#if QT_VERSION < 0x050000
#include <QPlastiqueStyle>
#endif

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
#include <algorithm>
using std::min;

#define pqErrorMacro(estr)                                                                         \
  qDebug() << "Error in:" << endl << __FILE__ << ", line " << __LINE__ << endl << "" estr << endl;

// User interface
//=============================================================================
class pqMemoryInspectorPanelUI : public Ui::pqMemoryInspectorPanelForm
{
};

// QProgress bar makes use of integer type. Each power of 10
// greater than 100 result in a decimal place of precision in
// the UI.
#define MIP_PROGBAR_MAX 1000

// keys for tree items
enum
{
  ITEM_KEY_PROCESS_TYPE = Qt::UserRole,
  ITEM_KEY_PVSERVER_TYPE, // vtkProcessModule::ProcessTypes
  ITEM_KEY_PID,           // long long
  ITEM_KEY_HOST_NAME,     // string, hostname
  ITEM_KEY_MPI_RANK,      // int
  ITEM_KEY_HOST_OS,       // descriptive string
  ITEM_KEY_HOST_CPU,      // descriptive string
  ITEM_KEY_HOST_MEM,      // descriptive string
  ITEM_KEY_SYSTEM_TYPE    // int (0 unix like, 1 win)
};

// data for tree items
enum
{
  ITEM_DATA_CLIENT_GROUP,
  ITEM_DATA_CLIENT_HOST,
  ITEM_DATA_CLIENT_RANK,
  ITEM_DATA_SERVER_GROUP,
  ITEM_DATA_SERVER_HOST,
  ITEM_DATA_SERVER_RANK,
  ITEM_DATA_UNIX_SYSTEM = 0,
  ITEM_DATA_WIN_SYSTEM = 1
};

namespace
{
#if QT_VERSION >= 0x050000
// ****************************************************************************
QStyle* getMemoryUseWidgetStyle()
{
  // this sets the style for the progress bar used to
  // display % memory usage. If we didn't do this the
  // display will look different on each OS. The ownership
  // of the style does not change hands when it's set to
  // the widget thus a single static instance is convenient.
  static QStyle* style = QStyleFactory::create("fusion");
  return style;
}
#else
// ****************************************************************************
QPlastiqueStyle* getMemoryUseWidgetStyle()
{
  // this sets the style for the progress bar used to
  // display % memory usage. If we didn't do this the
  // display will look different on each OS. The ownership
  // of the style does not change hands when it's set to
  // the widget thus a single static instance is convenient.
  static QPlastiqueStyle style;
  return &style;
}
#endif
// ****************************************************************************
float getSystemWarningThreshold()
{
#ifdef _WIN32
  return 0.90f;
#else
  return 0.80f;
#endif
}

// ****************************************************************************
float getSystemCriticalThreshold()
{
#ifdef _WIN32
  return 0.95f;
#else
  return 0.90f;
#endif
}

// ****************************************************************************
float getProcessWarningThreshold()
{
  return 0.70f;
}

// ****************************************************************************
float getProcessCriticalThreshold()
{
  return 0.80f;
}

// ****************************************************************************
void setMemoryUseWidgetColor(QPalette& palette, float fracUsed, float fracWarn, float fracCrit)
{
  (void)fracWarn;

  if (fracUsed > fracCrit)
  {
    // danger -> red
    // palette.setColor(QPalette::Highlight,QColor(232,40,40)); // bright
    // palette.setColor(QPalette::Highlight,QColor(190,60,60)); // moderate
    // palette.setColor(QPalette::Highlight,QColor(147,57,57)); // moderate balanced
    palette.setColor(QPalette::Highlight, QColor(217, 84, 84)); // cdash red
  }
  /*
  else
  if (fracUsed>fracWarn)
    {
    //palette.setColor(QPalette::Highlight,QColor(219,219,80)); // moderate
    palette.setColor(QPalette::Highlight,QColor(252,168,84));   // cdash yellow
    }
  */
  else
  {
    // ok -> green
    // palette.setColor(QPalette::Highlight,QColor(66,232,20)); // bright
    // palette.setColor(QPalette::Highlight,QColor(90,160,70)); // moderate
    // palette.setColor(QPalette::Highlight,QColor(125,176,74));// cdash yellow
    palette.setColor(QPalette::Highlight, Qt::darkGray);
  }
}

// ****************************************************************************
void setWidgetContainerColor(QPalette& palette, int rank)
{
  if (rank % 2)
  {
    palette.setColor(QPalette::Base, QColor(250, 250, 250));
  }
  else
  {
    palette.setColor(QPalette::Base, QColor(237, 237, 237));
  }
}

// ****************************************************************************
QString translateUnits(float memUse)
{
  QString fmt("%1 %2");

  float p210 = pow(2.0, 10.0);
  float p220 = pow(2.0, 20.0);
  float p230 = pow(2.0, 30.0);
  float p240 = pow(2.0, 40.0);
  float p250 = pow(2.0, 50.0);

  // were dealing with kiB
  memUse *= 1024;

  if (memUse < p210)
  {
    return fmt.arg(memUse, 0, 'f', 2).arg("B");
  }
  else if (memUse < p220)
  {
    return fmt.arg(memUse / p210, 0, 'f', 2).arg("KiB");
  }
  else if (memUse < p230)
  {
    return fmt.arg(memUse / p220, 0, 'f', 2).arg("MiB");
  }
  else if (memUse < p240)
  {
    return fmt.arg(memUse / p230, 0, 'f', 2).arg("GiB");
  }
  else if (memUse < p250)
  {
    return fmt.arg(memUse / p240, 0, 'f', 2).arg("TiB");
  }

  return fmt.arg(memUse / p250, 0, 'f', 2).arg("PiB");
}

// ****************************************************************************
template <typename T>
void ClearVectorOfPointers(vector<T*> data)
{
  size_t n = data.size();
  for (size_t i = 0; i < n; ++i)
  {
    if (data[i])
    {
      delete data[i];
      data[i] = NULL;
    }
  }
  data.clear();
}

// ****************************************************************************
void unescapeWhitespace(string& s, char escChar)
{
  size_t n = s.size();
  for (size_t i = 0; i < n; ++i)
  {
    if (s[i] == escChar)
    {
      s[i] = ' ';
    }
  }
}
};

/// data associated with an mpi rank
//=============================================================================
class RankData
{
public:
  RankData(
    int rank, long long pid, long long procMemoryUse, long long hostMemoryUse, long long procAvail);

  ~RankData();

  void SetRank(int rank) { this->Rank = rank; }
  int GetRank() { return this->Rank; }

  void SetPid(long long pid) { this->Pid = pid; }
  long long GetPid() { return this->Pid; }

  void SetProcMemoryAvailable(long long capacity) { this->ProcMemoryAvailable = capacity; }
  long long GetProcMemoryAvailable() { return this->ProcMemoryAvailable; }

  void SetProcMemoryUse(long long load) { this->ProcMemoryUse = load; }
  long long GetProcMemoryUse() { return this->ProcMemoryUse; }

  void SetHostMemoryUse(long long load) { this->HostMemoryUse = load; }
  long long GetHostMemoryUse() { return this->HostMemoryUse; }

  float GetProcMemoryUseFraction()
  {
    return (float)this->ProcMemoryUse / (float)this->ProcMemoryAvailable;
  }

  QFrame* GetMemoryUseWidget() { return this->WidgetContainer; }

  void UpdateMemoryUseWidget();
  void InitializeMemoryUseWidget();

  void Print(ostream& os);

private:
  void InitializeMemoryUseWidget(QProgressBar*& loadWidget);

private:
  int Rank;
  long long Pid;
  long long ProcMemoryUse;
  long long HostMemoryUse;
  long long ProcMemoryAvailable;
  QProgressBar* MemoryUseWidget;
  QFrame* WidgetContainer;
};

//-----------------------------------------------------------------------------
RankData::RankData(
  int rank, long long pid, long long procMemoryUse, long long hostMemoryUse, long long procAvail)
  : Rank(rank)
  , Pid(pid)
  , ProcMemoryUse(procMemoryUse)
  , HostMemoryUse(hostMemoryUse)
  , ProcMemoryAvailable(procAvail)
{
  this->InitializeMemoryUseWidget();
}

//-----------------------------------------------------------------------------
RankData::~RankData()
{
  delete this->WidgetContainer;
}

//-----------------------------------------------------------------------------
void RankData::UpdateMemoryUseWidget()
{
  float used = (float)this->GetProcMemoryUse();
  float fracUsed = min(1.0f, this->GetProcMemoryUseFraction());
  float percUsed = fracUsed * 100.0f;
  int progVal = (int)(fracUsed * MIP_PROGBAR_MAX);

  this->MemoryUseWidget->setValue(progVal);
  this->MemoryUseWidget->setFormat(
    QString("%1 %2%").arg(::translateUnits(used)).arg(percUsed, 0, 'f', 2));

  QPalette palette(this->MemoryUseWidget->palette());
  ::setMemoryUseWidgetColor(
    palette, fracUsed, ::getProcessWarningThreshold(), ::getProcessCriticalThreshold());
  this->MemoryUseWidget->setPalette(palette);
}

//-----------------------------------------------------------------------------
void RankData::InitializeMemoryUseWidget()
{
  this->InitializeMemoryUseWidget(this->MemoryUseWidget);

  QLabel* rank = new QLabel;
  rank->setText(QString("%1").arg(this->Rank));

  QFrame* vline = new QFrame;
  vline->setFrameStyle(QFrame::VLine | QFrame::Plain);

  QFrame* vline2 = new QFrame;
  vline2->setFrameStyle(QFrame::VLine | QFrame::Plain);

  QLabel* pid = new QLabel;
  pid->setText(QString("%1").arg(this->Pid));

  QHBoxLayout* w = new QHBoxLayout;
  w->addWidget(this->MemoryUseWidget);
  w->setContentsMargins(2, 2, 2, 2);
  w->setSpacing(2);

  QHBoxLayout* l = new QHBoxLayout;
  l->addWidget(rank);
  l->addWidget(vline);
  l->addWidget(pid);
  l->addWidget(vline2);
  l->addLayout(w);
  l->setContentsMargins(1, 0, 1, 0);
  l->setSpacing(0);

  this->WidgetContainer = new QFrame;
  this->WidgetContainer->setLayout(l);
  this->WidgetContainer->setFrameStyle(QFrame::Box | QFrame::Plain);
  this->WidgetContainer->setLineWidth(1);
  QFont font(this->WidgetContainer->font());
  font.setPointSize(8);
  this->WidgetContainer->setFont(font);

  QPalette palette(rank->palette());
  ::setWidgetContainerColor(palette, this->Rank);

  rank->setPalette(palette);
  rank->setAutoFillBackground(true);

  pid->setPalette(palette);
  pid->setAutoFillBackground(true);

  QFontMetrics fontMet(font);
  int rankWid = fontMet.width("555555");
  rank->setMinimumWidth(rankWid);
  rank->setMaximumWidth(rankWid);

  int pidWid = fontMet.width("555555555");
  pid->setMinimumWidth(pidWid);
  pid->setMaximumWidth(pidWid);

  this->UpdateMemoryUseWidget();
}

//-----------------------------------------------------------------------------
void RankData::InitializeMemoryUseWidget(QProgressBar*& loadWidget)
{
  loadWidget = new QProgressBar;
  loadWidget->setStyle(::getMemoryUseWidgetStyle());
  loadWidget->setMaximumHeight(15);
  loadWidget->setMinimum(0);
  loadWidget->setMaximum(MIP_PROGBAR_MAX);
  QFont font(loadWidget->font());
  font.setPointSize(8);
  loadWidget->setFont(font);
}

//-----------------------------------------------------------------------------
void RankData::Print(ostream& os)
{
  os << "RankData(" << this << ")"
     << " Rank=" << this->Rank << " Pid=" << this->Pid << " ProcMemoryUse=" << this->ProcMemoryUse
     << " HostMemoryUse=" << this->HostMemoryUse
     << " ProcMemoryAvailable=" << this->ProcMemoryAvailable
     << " MemoryUseWidget=" << this->MemoryUseWidget << endl;
}

/// data associated with the host (a collection of ranks)
//=============================================================================
class HostData
{
public:
  HostData();
  HostData(string groupName, string hostName, long long memTotal, long long memAvail);
  HostData(const HostData& other) { *this = other; }
  const HostData& operator=(const HostData& other);
  ~HostData();

  RankData* AddRankData(int rank, long long pid, long long procMemAvail);
  RankData* GetRankData(int i);
  void ClearRankData();

  string& GetGroupName() { return this->GroupName; }
  string& GetHostName() { return this->HostName; }
  long long GetHostMemoryTotal() { return this->HostMemoryTotal; }
  long long GetHostMemoryAvailable() { return this->HostMemoryAvailable; }
  long long GetTotalSystemMemoryUse();
  long long GetProcessGroupMemoryUse();

  void SetTreeItem(QTreeWidgetItem* item) { this->TreeItem = item; }
  QTreeWidgetItem* GetTreeItem() { return this->TreeItem; }

  QWidget* GetMemoryUseWidget() { return (QWidget*)this->WidgetContainer; }
  QProgressBar* GetTotalMemoryUseWidget() { return this->TotalMemoryUseWidget; }
  QProgressBar* GetGroupMemoryUseWidget() { return this->GroupMemoryUseWidget; }

  void InitializeMemoryUseWidget();
  void InitializeMemoryUseWidget(QProgressBar*& loadWidget);

  void UpdateMemoryUseWidget();
  void UpdateMemoryUseWidget(
    QProgressBar* loadWidget, long long used, float fracUsed, float warnFrac, float critFrac);

  void Print(ostream& os);

private:
  string GroupName;
  string HostName;
  long long HostMemoryTotal;          // memory installed
  long long HostMemoryAvailable;      // memory available to us.
  QProgressBar* TotalMemoryUseWidget; // for displaying all process's use
  QProgressBar* GroupMemoryUseWidget; // for displaying only pv process's use
  QFrame* WidgetContainer;            // widget containing both all and process group
  QTreeWidgetItem* TreeItem;          // gui element
  vector<RankData*> Ranks;            // references to ranks local to this host
};

//-----------------------------------------------------------------------------
HostData::HostData()
  : GroupName("")
  , HostName("")
  , HostMemoryTotal(0)
  , HostMemoryAvailable(0)
{
}

//-----------------------------------------------------------------------------
HostData::HostData(
  string groupName, string hostName, long long hostMemTotal, long long hostMemAvail)
  : GroupName(groupName)
  , HostName(hostName)
  , HostMemoryTotal(hostMemTotal)
  , HostMemoryAvailable(hostMemAvail)
{
  this->InitializeMemoryUseWidget();
}

//-----------------------------------------------------------------------------
HostData::~HostData()
{
  this->ClearRankData();
  delete this->WidgetContainer;
}

//-----------------------------------------------------------------------------
const HostData& HostData::operator=(const HostData& other)
{
  if (this == &other)
    return *this;

  this->GroupName = other.GroupName;
  this->HostName = other.HostName;
  this->HostMemoryTotal = other.HostMemoryTotal;
  this->HostMemoryAvailable = other.HostMemoryAvailable;
  this->TotalMemoryUseWidget = other.TotalMemoryUseWidget;
  this->GroupMemoryUseWidget = other.GroupMemoryUseWidget;
  this->TreeItem = other.TreeItem;
  this->Ranks = other.Ranks;

  return *this;
}

//-----------------------------------------------------------------------------
RankData* HostData::GetRankData(int i)
{
  if ((size_t)i < this->Ranks.size())
  {
    return this->Ranks[i];
  }
  return NULL;
}

//-----------------------------------------------------------------------------
void HostData::ClearRankData()
{
  ::ClearVectorOfPointers<RankData>(this->Ranks);
}

//-----------------------------------------------------------------------------
RankData* HostData::AddRankData(int rank, long long pid, long long procMemAvail)
{
  RankData* newRank = new RankData(rank, pid, 0, 0, procMemAvail);
  this->Ranks.push_back(newRank);
  return newRank;
}

//-----------------------------------------------------------------------------
long long HostData::GetTotalSystemMemoryUse()
{
  long long load = 0;
  if (this->Ranks.size())
  {
    load = this->Ranks[0]->GetHostMemoryUse();
  }
  return load;
}

//-----------------------------------------------------------------------------
long long HostData::GetProcessGroupMemoryUse()
{
  long long load = 0;
  size_t n = this->Ranks.size();
  for (size_t i = 0; i < n; ++i)
  {
    load += this->Ranks[i]->GetProcMemoryUse();
  }
  return load;
}

//-----------------------------------------------------------------------------
void HostData::InitializeMemoryUseWidget()
{
  this->InitializeMemoryUseWidget(this->TotalMemoryUseWidget);
  this->InitializeMemoryUseWidget(this->GroupMemoryUseWidget);

  QFormLayout* l = new QFormLayout;
  l->setRowWrapPolicy(QFormLayout::DontWrapRows);
  l->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  l->setFormAlignment(Qt::AlignHCenter | Qt::AlignTop);
  l->setLabelAlignment(Qt::AlignRight);
  l->setContentsMargins(2, 2, 2, 2);
  l->setSpacing(2);

  l->addRow("System Total", this->TotalMemoryUseWidget);
  l->addRow(this->GroupName.c_str(), this->GroupMemoryUseWidget);

  QLabel* label = new QLabel;
  label->setText(this->HostName.c_str());

  QFrame* vline = new QFrame;
  vline->setFrameStyle(QFrame::VLine | QFrame::Plain);

  QHBoxLayout* h = new QHBoxLayout;
  h->addWidget(label);
  h->addWidget(vline);
  h->addLayout(l);
  h->setContentsMargins(1, 0, 1, 0);
  h->setSpacing(0);

  this->WidgetContainer = new QFrame;
  this->WidgetContainer->setLayout(h);
  this->WidgetContainer->setFrameStyle(QFrame::Box | QFrame::Plain);
  QFont font(this->WidgetContainer->font());
  font.setPointSize(8);
  this->WidgetContainer->setFont(font);

  this->UpdateMemoryUseWidget();
}

//-----------------------------------------------------------------------------
void HostData::InitializeMemoryUseWidget(QProgressBar*& loadWidget)
{
  loadWidget = new QProgressBar;
  loadWidget->setStyle(::getMemoryUseWidgetStyle());
  loadWidget->setMaximumHeight(15);
  loadWidget->setMinimum(0);
  loadWidget->setMaximum(MIP_PROGBAR_MAX);
  QFont font(loadWidget->font());
  font.setPointSize(8);
  loadWidget->setFont(font);
}

//-----------------------------------------------------------------------------
void HostData::UpdateMemoryUseWidget()
{
  long long load;
  float frac;

  load = this->GetTotalSystemMemoryUse();
  frac = min(1.0f, (float)load / (float)this->HostMemoryTotal);
  this->UpdateMemoryUseWidget(this->TotalMemoryUseWidget, load, frac, ::getSystemWarningThreshold(),
    ::getSystemCriticalThreshold());

  load = this->GetProcessGroupMemoryUse();
  frac = min(1.0f, (float)load / (float)this->HostMemoryAvailable);
  this->UpdateMemoryUseWidget(this->GroupMemoryUseWidget, load, frac,
    ::getProcessWarningThreshold(), ::getProcessCriticalThreshold());
}

//-----------------------------------------------------------------------------
void HostData::UpdateMemoryUseWidget(
  QProgressBar* loadWidget, long long used, float fracUsed, float warnFrac, float critFrac)
{
  float percUsed = fracUsed * 100.0f;
  int progVal = (int)(fracUsed * MIP_PROGBAR_MAX);

  loadWidget->setValue(progVal);
  loadWidget->setFormat(QString("%1 %2%").arg(::translateUnits(used)).arg(percUsed, 0, 'f', 2));

  QPalette palette(loadWidget->palette());
  ::setMemoryUseWidgetColor(palette, fracUsed, warnFrac, critFrac);
  loadWidget->setPalette(palette);
}

//-----------------------------------------------------------------------------
void HostData::Print(ostream& os)
{
  os << "HostData(" << this << ")"
     << " HostName=" << this->HostName << " HostMemoryTotal=" << this->HostMemoryTotal
     << " TotalMemoryUseWidget=" << this->TotalMemoryUseWidget
     << " GroupMemoryUseWidget=" << this->GroupMemoryUseWidget
     << " WidgetContainer=" << this->WidgetContainer << " TreeItem=" << this->TreeItem
     << " Ranks=" << endl;
  size_t nRanks = this->Ranks.size();
  for (size_t i = 0; i < nRanks; ++i)
  {
    this->Ranks[i]->Print(os);
  }
  os << endl;
}

//-----------------------------------------------------------------------------
pqMemoryInspectorPanel::pqMemoryInspectorPanel(QWidget* pWidget, Qt::WindowFlags flags)
  : QWidget(pWidget, flags)
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::pqMemoryInspectorPanel" << endl;
#endif

  this->ClientOnly = 1;
  this->ClientHost = 0;
  this->AutoUpdate = true;
  this->UpdateEnabled = 0;

  // Construct Qt form.
  this->Ui = new pqMemoryInspectorPanelUI;
  this->Ui->setupUi(this);
  this->Ui->updateMemUse->setIcon(QPixmap(":/pqWidgets/Icons/pqRedo24.png"));

  // attempt initialization here before we begin to listen
  // for event's that could trigger and update.
  this->Initialize();

  // listen for enable/disable auto update
  QObject::connect(this->Ui->autoUpdate, SIGNAL(toggled(bool)), this, SLOT(SetAutoUpdate(bool)));

  // listen for manual update request
  QObject::connect(this->Ui->updateMemUse, SIGNAL(released()), this, SLOT(Update()));

  // listen to context menu events
  QObject::connect(this->Ui->configView, SIGNAL(customContextMenuRequested(const QPoint&)), this,
    SLOT(ConfigViewContextMenu(const QPoint&)));
  this->Ui->configView->setContextMenuPolicy(Qt::CustomContextMenu);

  // connect to new views as they are created
  pqServerManagerModel* smm = pqApplicationCore::instance()->getServerManagerModel();

  // listen for new views, we want to update after render is finished.
  QObject::connect(smm, SIGNAL(viewAdded(pqView*)), this, SLOT(ConnectToView(pqView*)));

  // listen to the active view for render completed events.
  pqView* view = pqActiveObjects::instance().activeView();
  if (view)
  {
    this->ConnectToView(view);
  }

  // listen to servers connecting and disconnecting
  QObject::connect(smm, SIGNAL(serverAdded(pqServer*)), this, SLOT(ServerConnected()));

  QObject::connect(smm, SIGNAL(finishedRemovingServer()), this, SLOT(ServerDisconnected()));

  /*
  // TODO -- not sure if these are useful, maybe for responding
  // to python events?
  QObject::connect(
        smm,
        SIGNAL(dataUpdated(pqPipelineSource*)),
        this,
        SLOT(EnableUpdate()));

  QObject::connect(
        smm,
        SIGNAL(proxyAdded(pqProxy*)),
        this,
        SLOT(EnableUpdate()));

  QObject::connect(
        smm,
        SIGNAL(proxRemoved(pqProxy*)),
        this,
        SLOT(EnableUpdate()));
  */

  QPalette pal = this->Ui->configView->palette();
  pal.setColor(QPalette::Highlight, Qt::lightGray);
  this->Ui->configView->setPalette(pal);
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
  this->ClientHost = 0;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ClearServer(map<string, HostData*>& hosts, vector<RankData*>& ranks)
{
  map<string, HostData*>::iterator it = hosts.begin();
  map<string, HostData*>::iterator end = hosts.end();
  while (it != end)
  {
    if ((*it).second)
    {
      delete (*it).second;
      (*it).second = NULL;
    }
    ++it;
  }
  hosts.clear();
  ranks.clear();
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ClearServers()
{
  this->ClientOnly = 1;
  this->ClearServer(this->ServerHosts, this->ServerRanks);
  this->ClearServer(this->DataServerHosts, this->DataServerRanks);
  this->ClearServer(this->RenderServerHosts, this->RenderServerRanks);
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ServerDisconnected()
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel:;ServerDisconnected" << endl;
#endif

  this->ClearClient();
  this->ClearServers();
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ServerConnected()
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::ServerConnected" << endl;
#endif

  this->Initialize();
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ConnectToView(pqView* view)
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::ConnectToView" << endl;
#endif

  // rendering triggers the update
  QObject::connect(view, SIGNAL(endRender()), this, SLOT(RenderCompleted()));

  // update only after data is changed
  QObject::connect(view, SIGNAL(updateDataEvent()), this, SLOT(EnableUpdate()));
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::showEvent(QShowEvent* e)
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::showEvent" << endl;
#endif

  (void)e;
  this->Update();
}
//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::EnableUpdate()
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::EnableUpdate" << endl;
#endif

  this->UpdateEnabled = 1;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::RenderCompleted()
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::RenderCompleted" << endl;
#endif

  if (!this->AutoUpdate || !this->isVisible() || !this->UpdateEnabled)
  {
    return;
  }

  this->Update();
  return;

  /*
  // This code is for updating on endRender regardless of whether
  // or not data was updated.it works better than updating only
  // after data is updated. however is disabled because it's slight
  // over kill
  if (!this->AutoUpdate || !this->isVisible())
    {
    return;
    }

  // only going to respond to render view's events. other view's
  // events may be useful to track but need to consider case by
  // case and verify that we don't flood the server with update
  // requests.
  pqRenderView* view
    = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());

  if (view)
    {
    vtkSMRenderViewProxy *viewProxy
      = dynamic_cast<vtkSMRenderViewProxy*>(view->getProxy());
    if (viewProxy)
      {
      if (!viewProxy->LastRenderWasInteractive() && !this->PendingUpdate)
        {
        // there are two still renders after the last
        // interactive render.
        QTimer::singleShot(2000, this, SLOT(Update()));
        this->PendingUpdate=1;
        }
      }
    }
  */
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::InitializeServerGroup(long long clientPid,
  vtkPVSystemConfigInformation* configs, int validProcessType, QTreeWidgetItem* group,
  string groupName, map<string, HostData*>& hosts, vector<RankData*>& ranks, int& systemType)
{
  size_t nConfigs = configs->GetSize();
  for (size_t i = 0; i < nConfigs; ++i)
  {
    // assume that we get what we ask for and nothing more
    // but verify that it's not the builtin
    long long pid = configs->GetPid(i);
    if (pid == clientPid)
    {
      // must be the builtin, do not duplicate.
      continue;
    }
    int processType = configs->GetProcessType(i);
    if (processType != validProcessType)
    {
      // it's not the correct type.
      continue;
    }
    string os = configs->GetOSDescriptor(i);
    string cpu = configs->GetCPUDescriptor(i);
    string mem = configs->GetMemoryDescriptor(i);
    string hostName = configs->GetHostName(i);
    systemType = configs->GetSystemType(i);
    int rank = configs->GetRank(i);
    long long hostMemoryTotal = configs->GetHostMemoryTotal(i);
    long long hostMemoryAvailable = configs->GetHostMemoryAvailable(i);
    long long procMemoryAvailable = configs->GetProcMemoryAvailable(i);

// it's useful to have hostname's rank's and pid's in
// the terminal. if the client hangs you can attach
// gdb and see where it's stuck without a lot of effort

#ifdef MIP_PROCESS_TABLE
    cerr << setw(32) << hostName << setw(16) << pid << setw(8) << rank << endl << setw(1);
#endif

    // host
    HostData* serverHost = NULL;

    pair<string, HostData*> ins(hostName, (HostData*)0);
    pair<map<string, HostData*>::iterator, bool> ret;
    ret = hosts.insert(ins);
    if (ret.second)
    {
      // new host
      serverHost = new HostData(groupName, hostName, hostMemoryTotal, hostMemoryAvailable);
      ret.first->second = serverHost;

      QTreeWidgetItem* serverHostItem = new QTreeWidgetItem(group);
      serverHostItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
      serverHostItem->setExpanded(true);
      serverHostItem->setData(0, ITEM_KEY_PROCESS_TYPE, QVariant(ITEM_DATA_SERVER_HOST));
      serverHostItem->setData(0, ITEM_KEY_PVSERVER_TYPE, QVariant(processType));
      serverHostItem->setData(0, ITEM_KEY_SYSTEM_TYPE, QVariant(systemType));
      serverHostItem->setData(0, ITEM_KEY_HOST_NAME, QVariant(hostName.c_str()));
      serverHostItem->setData(0, ITEM_KEY_HOST_OS, QString(os.c_str()));
      serverHostItem->setData(0, ITEM_KEY_HOST_CPU, QString(cpu.c_str()));
      serverHostItem->setData(0, ITEM_KEY_HOST_MEM, QString(mem.c_str()));
      this->Ui->configView->setItemWidget(serverHostItem, 0, serverHost->GetMemoryUseWidget());

      serverHost->SetTreeItem(serverHostItem);
    }
    else
    {
      serverHost = ret.first->second;
    }
    // rank
    RankData* serverRank = serverHost->AddRankData(rank, pid, procMemoryAvailable);
    ranks.push_back(serverRank);

    QTreeWidgetItem* serverRankItem = new QTreeWidgetItem(serverHost->GetTreeItem());
    serverRankItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    serverRankItem->setExpanded(false);
    serverRankItem->setData(0, ITEM_KEY_PROCESS_TYPE, QVariant(ITEM_DATA_SERVER_RANK));
    serverRankItem->setData(0, ITEM_KEY_PVSERVER_TYPE, QVariant(processType));
    serverRankItem->setData(0, ITEM_KEY_HOST_NAME, QVariant(hostName.c_str()));
    serverRankItem->setData(0, ITEM_KEY_PID, QVariant(pid));
    serverRankItem->setData(0, ITEM_KEY_SYSTEM_TYPE, QVariant(systemType));
    this->Ui->configView->setItemWidget(serverRankItem, 0, serverRank->GetMemoryUseWidget());
  }

  // prune off the group if there were none added.
  if (group->childCount() == 0)
  {
    int idx = this->Ui->configView->indexOfTopLevelItem(group);
    if (idx >= 0)
    {
      this->Ui->configView->takeTopLevelItem(idx);
    }
  }
}

//-----------------------------------------------------------------------------
int pqMemoryInspectorPanel::Initialize()
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::Initialize" << endl;
#endif

  this->ClearClient();
  this->ClearServers();

  this->ClientSystemType = 0;
  this->ServerSystemType = 0;
  this->DataServerSystemType = 0;
  this->RenderServerSystemType = 0;

  this->StackTraceOnClient = 0;
  this->StackTraceOnServer = 0;
  this->StackTraceOnDataServer = 0;
  this->StackTraceOnRenderServer = 0;

  this->Ui->configView->clear();

  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    // this is not necessarily an error as the panel may be created
    // before the server is connected.
    return 0;
  }

  // client
  QTreeWidgetItem* clientGroup = new QTreeWidgetItem(this->Ui->configView);
  clientGroup->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientGroup->setExpanded(true);
  clientGroup->setData(0, ITEM_KEY_PROCESS_TYPE, QVariant(ITEM_DATA_CLIENT_GROUP));

  QWidget* groupWidget = this->NewGroupWidget("client", ":/pqWidgets/Icons/pqClient32.png");
  this->Ui->configView->setItemWidget(clientGroup, 0, groupWidget);

  vtkSMSession* session = NULL;
  vtkPVSystemConfigInformation* configs = NULL;

  configs = vtkPVSystemConfigInformation::New();
  session = server->session();
  session->GatherInformation(vtkPVSession::CLIENT, configs, 0);

  size_t nConfigs = configs->GetSize();
  if (nConfigs != 1)
  {
    pqErrorMacro("There should always be one client.");
    return 0;
  }

  long long clientPid = configs->GetPid(0);

  this->ClientHost = new HostData("paraview", configs->GetHostName(0),
    configs->GetHostMemoryTotal(0), configs->GetHostMemoryAvailable(0));

  this->ClientHost->AddRankData(0, configs->GetPid(0), configs->GetProcMemoryAvailable(0));

  this->ClientSystemType = configs->GetSystemType(0);

  QTreeWidgetItem* clientHostItem = new QTreeWidgetItem(clientGroup);
  clientHostItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
  clientHostItem->setExpanded(true);
  clientHostItem->setData(0, ITEM_KEY_PROCESS_TYPE, QVariant(ITEM_DATA_CLIENT_HOST));
  clientHostItem->setData(0, ITEM_KEY_PVSERVER_TYPE, QVariant(configs->GetProcessType(0)));
  clientHostItem->setData(0, ITEM_KEY_SYSTEM_TYPE, QVariant(configs->GetSystemType(0)));
  clientHostItem->setData(0, ITEM_KEY_HOST_OS, QString(configs->GetOSDescriptor(0)));
  clientHostItem->setData(0, ITEM_KEY_HOST_CPU, QString(configs->GetCPUDescriptor(0)));
  clientHostItem->setData(0, ITEM_KEY_HOST_MEM, QString(configs->GetMemoryDescriptor(0)));
  clientHostItem->setData(0, ITEM_KEY_HOST_NAME, QVariant(configs->GetHostName(0)));
  clientHostItem->setData(0, ITEM_KEY_PID, QVariant(configs->GetPid(0)));
  this->Ui->configView->setItemWidget(clientHostItem, 0, this->ClientHost->GetMemoryUseWidget());

  configs->Delete();

#ifdef MIP_PROCESS_TABLE
  // print a process table to the terminal.
  cerr << endl
       << setw(32) << "Host" << setw(16) << "Pid" << setw(8) << "Rank" << endl
       << left << setw(56) << setfill('=') << "client" << endl
       << right << setw(1) << setfill(' ') << setw(32) << configs->GetFullyQualifiedDomainName(0)
       << setw(16) << configs->GetPid(0) << setw(8) << "x" << endl
       << setfill(' ') << setw(1);
#endif

  // collect info from the server(s)

  configs = vtkPVSystemConfigInformation::New();

  vtkPVSystemConfigInformation* dsconfigs = vtkPVSystemConfigInformation::New();
  session->GatherInformation(vtkPVSession::DATA_SERVER, dsconfigs, 0);
  configs->AddInformation(dsconfigs);
  dsconfigs->Delete();

  // don't attempt to communicate with a render server if it's
  // not connected which results in a duplicated gather as in that
  // case comm is routed to the data server.
  if (session->GetRenderClientMode() == vtkSMSession::RENDERING_SPLIT)
  {
    vtkPVSystemConfigInformation* rsconfigs = vtkPVSystemConfigInformation::New();
    session->GatherInformation(vtkPVSession::RENDER_SERVER, rsconfigs, 0);
    configs->AddInformation(rsconfigs);
    rsconfigs->Delete();
  }

  nConfigs = configs->GetSize();
  if (nConfigs > 0)
  {
    configs->Sort();

// servers
#ifdef MIP_PROCESS_TABLE
    cerr << left << setw(56) << setfill('=') << "server" << endl
         << right << setw(1) << setfill(' ');
#endif
    QTreeWidgetItem* group = NULL;
    group = new QTreeWidgetItem(this->Ui->configView);
    group->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    group->setExpanded(true);
    group->setData(0, ITEM_KEY_PROCESS_TYPE, QVariant(ITEM_DATA_SERVER_GROUP));

    groupWidget = this->NewGroupWidget("server", ":/pqWidgets/Icons/pqDataServer32.png");
    this->Ui->configView->addTopLevelItem(group);
    this->Ui->configView->setItemWidget(group, 0, groupWidget);

    this->InitializeServerGroup(clientPid, configs, vtkProcessModule::PROCESS_SERVER, group,
      "pvserver", this->ServerHosts, this->ServerRanks, this->ServerSystemType);

// dataservers
#ifdef MIP_PROCESS_TABLE
    cerr << left << setw(56) << setfill('=') << "data server" << endl
         << right << setw(1) << setfill(' ');
#endif
    group = new QTreeWidgetItem(this->Ui->configView);
    group->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    group->setExpanded(true);
    group->setData(0, ITEM_KEY_PROCESS_TYPE, QVariant(ITEM_DATA_SERVER_GROUP));

    groupWidget = this->NewGroupWidget("data server", ":/pqWidgets/Icons/pqDataServer32.png");
    this->Ui->configView->addTopLevelItem(group);
    this->Ui->configView->setItemWidget(group, 0, groupWidget);

    this->InitializeServerGroup(clientPid, configs, vtkProcessModule::PROCESS_DATA_SERVER, group,
      "pvdataserver", this->DataServerHosts, this->DataServerRanks, this->DataServerSystemType);

// renderservers
#ifdef MIP_PROCESS_TABLE
    cerr << left << setw(56) << setfill('=') << "render server" << endl
         << right << setw(1) << setfill(' ');
#endif
    group = new QTreeWidgetItem(this->Ui->configView);
    group->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    group->setExpanded(true);
    group->setData(0, ITEM_KEY_PROCESS_TYPE, QVariant(ITEM_DATA_SERVER_GROUP));

    groupWidget = this->NewGroupWidget("render server", ":/pqWidgets/Icons/pqDataServer32.png");
    this->Ui->configView->addTopLevelItem(group);
    this->Ui->configView->setItemWidget(group, 0, groupWidget);

    this->InitializeServerGroup(clientPid, configs, vtkProcessModule::PROCESS_RENDER_SERVER, group,
      "pvrenderserver", this->RenderServerHosts, this->RenderServerRanks,
      this->RenderServerSystemType);

#ifdef MIP_PROCESS_TABLE
    cerr << setw(56) << setfill('=') << "=" << endl << setw(1) << setfill(' ');
#endif
  }
  configs->Delete();

  //
  this->ClientOnly = 0;
  if ((this->RenderServerHosts.size() == 0) && (this->DataServerHosts.size() == 0) &&
    (this->ServerHosts.size() == 0))
  {
    this->ClientOnly = 1;
  }
  else
  {
    // update host load to reflect all of its ranks.
    map<string, HostData*>::iterator it;
    map<string, HostData*>::iterator end;
    it = this->DataServerHosts.begin();
    end = this->DataServerHosts.end();
    while (it != end)
    {
      it->second->UpdateMemoryUseWidget();
      ++it;
    }

    it = this->RenderServerHosts.begin();
    end = this->RenderServerHosts.end();
    while (it != end)
    {
      it->second->UpdateMemoryUseWidget();
      ++it;
    }
  }

  //
  this->Ui->configView->resizeColumnToContents(0);
  this->Ui->configView->resizeColumnToContents(1);

  // fecth the remote memory usage
  this->Update();

  // don't expand by default when there are a lot of
  // sever hosts
  if ((this->RenderServerHosts.size() > 1) || (this->DataServerHosts.size() > 1) ||
    (this->ServerHosts.size() > 2))
  {
    this->ShowOnlyNodes();
  }
  else
  {
    this->ShowAllRanks();
  }

  return 1;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::Update()
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::Update" << endl;
#endif

  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    return;
  }

  if (!this->Initialized() && !this->Initialize())
  {
    return;
  }

  this->UpdateRanks();
  this->UpdateHosts();

  this->PendingUpdate = 0;
  this->UpdateEnabled = 0;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::UpdateRanks()
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::UpdateRanks" << endl;
#endif

  pqServer* server = pqActiveObjects::instance().activeServer();
  if (!server)
  {
    pqErrorMacro("failed to get active server");
    return;
  }

  // fectch latest numbers
  vtkSMSession* session = NULL;
  vtkPVMemoryUseInformation* infos = NULL;
  size_t nInfos = 0;

  // client
  infos = vtkPVMemoryUseInformation::New();
  session = server->session();
  session->GatherInformation(vtkPVSession::CLIENT, infos, 0);

  nInfos = infos->GetSize();
  if (nInfos == 0)
  {
    pqErrorMacro("failed to get client info");
    return;
  }

  RankData* clientRank = this->ClientHost->GetRankData(0);
  clientRank->SetProcMemoryUse(infos->GetProcMemoryUse(0));
  clientRank->SetHostMemoryUse(infos->GetHostMemoryUse(0));
  clientRank->UpdateMemoryUseWidget();

  infos->Delete();

  if (this->ClientOnly)
  {
    return;
  }

  // servers
  infos = vtkPVMemoryUseInformation::New();

  vtkPVMemoryUseInformation* dsinfos = vtkPVMemoryUseInformation::New();
  session->GatherInformation(vtkPVSession::DATA_SERVER, dsinfos, 0);
  infos->AddInformation(dsinfos);
  dsinfos->Delete();

  // don't attempt to communicate with a render server if it's
  // not connected which results in a duplicated gather as in that
  // case comm is routed to the data server.
  if (session->GetRenderClientMode() == vtkSMSession::RENDERING_SPLIT)
  {
    vtkPVMemoryUseInformation* rsinfos = vtkPVMemoryUseInformation::New();
    session->GatherInformation(vtkPVSession::RENDER_SERVER, rsinfos, 0);
    infos->AddInformation(rsinfos);
    rsinfos->Delete();
  }

  nInfos = infos->GetSize();
  for (size_t i = 0; i < nInfos; ++i)
  {
    int rank = infos->GetRank((int)i);
    long long procUse = infos->GetProcMemoryUse((int)i);
    long long hostUse = infos->GetHostMemoryUse((int)i);

    switch (infos->GetProcessType((int)i))
    {
      case vtkProcessModule::PROCESS_SERVER:
        if (this->ServerRanks.size())
        {
          this->ServerRanks[rank]->SetProcMemoryUse(procUse);
          this->ServerRanks[rank]->SetHostMemoryUse(hostUse);
          this->ServerRanks[rank]->UpdateMemoryUseWidget();
        }
        break;

      case vtkProcessModule::PROCESS_DATA_SERVER:
        if (this->DataServerRanks.size())
        {
          this->DataServerRanks[rank]->SetProcMemoryUse(procUse);
          this->DataServerRanks[rank]->SetHostMemoryUse(hostUse);
          this->DataServerRanks[rank]->UpdateMemoryUseWidget();
        }
        break;

      case vtkProcessModule::PROCESS_RENDER_SERVER:
        if (this->RenderServerRanks.size())
        {
          this->RenderServerRanks[rank]->SetProcMemoryUse(procUse);
          this->RenderServerRanks[rank]->SetHostMemoryUse(hostUse);
          this->RenderServerRanks[rank]->UpdateMemoryUseWidget();
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
void pqMemoryInspectorPanel::UpdateHosts()
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::UpdateHosts" << endl;
#endif

  this->ClientHost->UpdateMemoryUseWidget();

  if (this->ClientOnly)
  {
    return;
  }

  this->UpdateHosts(this->ServerHosts);
  this->UpdateHosts(this->DataServerHosts);
  this->UpdateHosts(this->RenderServerHosts);
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::UpdateHosts(map<string, HostData*>& hosts)
{
  // update host load to reflect all of its ranks.
  map<string, HostData*>::iterator it = hosts.begin();
  map<string, HostData*>::iterator end = hosts.end();
  while (it != end)
  {
    (*it).second->UpdateMemoryUseWidget();
    ++it;
  }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::EnableStackTraceOnClient(bool enable)
{
  this->EnableStackTrace(enable, vtkPVSession::CLIENT);
  this->StackTraceOnClient = enable;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::EnableStackTraceOnServer(bool enable)
{
  this->EnableStackTrace(enable, vtkPVSession::DATA_SERVER);
  this->StackTraceOnServer = enable;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::EnableStackTraceOnDataServer(bool enable)
{
  this->EnableStackTrace(enable, vtkPVSession::DATA_SERVER);
  this->StackTraceOnDataServer = enable;
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::EnableStackTraceOnRenderServer(bool enable)
{
  this->EnableStackTrace(enable, vtkPVSession::RENDER_SERVER);
  this->StackTraceOnRenderServer = enable;
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
    vtkPVEnableStackTraceSignalHandler* esh = vtkPVEnableStackTraceSignalHandler::New();

    server->session()->GatherInformation(group, esh, 0);
  }
  else
  {
    vtkPVDisableStackTraceSignalHandler* dsh = vtkPVDisableStackTraceSignalHandler::New();

    server->session()->GatherInformation(group, dsh, 0);
  }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ExecuteRemoteCommand()
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::ExecCommand" << endl;
#endif

  QTreeWidgetItem* item = this->Ui->configView->currentItem();
  if (item)
  {
    bool ok;
    int type = item->data(0, ITEM_KEY_PROCESS_TYPE).toInt(&ok);
    if (!ok)
      return;

    // treate the client group like a rank.
    if (type == ITEM_DATA_CLIENT_GROUP)
    {
      item = item->child(0);
      if (item == NULL)
        return;
      type = item->data(0, ITEM_KEY_PROCESS_TYPE).toInt();
    }
    // treate the client host like a rank.
    if (type == ITEM_DATA_CLIENT_HOST)
    {
      type = ITEM_DATA_CLIENT_RANK;
    }

    switch (type)
    {
      case ITEM_DATA_CLIENT_RANK:
      case ITEM_DATA_SERVER_RANK:
      {
        string host((const char*)item->data(0, ITEM_KEY_HOST_NAME).toString().toLocal8Bit());
        string pid((const char*)item->data(0, ITEM_KEY_PID).toString().toLocal8Bit());

        int serverSystemType = item->data(0, ITEM_KEY_SYSTEM_TYPE).toInt();

        bool localServer = (this->ClientHost->GetHostName() == host);
        if (localServer)
        {
          serverSystemType = -1;
        }

        // select and configure a command
        pqRemoteCommandDialog dialog(this, 0, this->ClientSystemType, serverSystemType);

        dialog.SetActiveHost(host);
        dialog.SetActivePid(pid);

        if (dialog.exec() == QDialog::Accepted)
        {
          string command = dialog.GetCommand();

          string tmp;

          QString exe;
          QStringList args;

          istringstream is(command);

          is >> tmp;
          ::unescapeWhitespace(tmp, '^');
          exe = tmp.c_str();

          while (is.good())
          {
            is >> tmp;
            ::unescapeWhitespace(tmp, '^');
            args << tmp.c_str();
          }

          QProcess* proc = new QProcess(this);
          QObject::connect(proc, SIGNAL(error(QProcess::ProcessError)), this,
            SLOT(RemoteCommandFailed(QProcess::ProcessError)));
          proc->setProcessChannelMode(QProcess::ForwardedChannels);
          proc->closeWriteChannel();
          proc->startDetached(exe, args);
        }
      }
      break;

      default:
        QMessageBox ebx(QMessageBox::Information, "Error",
          "No process selected. Select a specific process first by "
          "clicking on its entry in the tree view widget.");
        ebx.exec();
        break;
    }
  }
  else
  {
    QMessageBox ebx(QMessageBox::Information, "Error",
      "No process selected. Select a specific process first by "
      "clicking on its entry in the tree view widget.");
    ebx.exec();
  }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::RemoteCommandFailed(QProcess::ProcessError code)
{
  switch (code)
  {
    case QProcess::FailedToStart:
      qCritical() << "The process failed to start. Either the invoked program is missing, "
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

  QTreeWidgetItem* item;
  QTreeWidgetItemIterator it(this->Ui->configView);
  while ((item = *it) != (QTreeWidgetItem*)0)
  {
    bool ok;
    int type = item->data(0, ITEM_KEY_PROCESS_TYPE).toInt(&ok);
    if (!ok)
      return;
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

  QTreeWidgetItem* item = this->Ui->configView->currentItem();
  if (!item)
  {
    return;
  }

  int type = item->data(0, ITEM_KEY_PROCESS_TYPE).toInt();

  // treate the client group like a host.
  if (type == ITEM_DATA_CLIENT_GROUP)
  {
    item = item->child(0);
    if (item == NULL)
      return;
    type = item->data(0, ITEM_KEY_PROCESS_TYPE).toInt();
  }

  if ((type == ITEM_DATA_CLIENT_HOST) || (type == ITEM_DATA_SERVER_HOST))
  {
    QString host = item->data(0, ITEM_KEY_HOST_NAME).toString();
    QString os = item->data(0, ITEM_KEY_HOST_OS).toString();
    QString cpu = item->data(0, ITEM_KEY_HOST_CPU).toString();
    QString mem = item->data(0, ITEM_KEY_HOST_MEM).toString();

    QString descr;
    descr += "<h2>";
    descr += (type == ITEM_DATA_CLIENT_HOST ? "Client" : "Server");
    descr += " System Properties</h2><hr><table>";
    descr += "<tr><td><b>Host:</b></td><td>";
    descr += host;
    descr += "</td></tr>";
    descr += "<tr><td><b>OS:</b></td><td>";
    descr += os;
    descr += "</td></tr>";
    descr += "<tr><td><b>CPU:</b></td><td>";
    descr += cpu;
    descr += "</td></tr>";
    descr += "<tr><td><b>Memory:</b></td><td>";
    descr += mem;
    descr += "</td></tr></table><hr>";

    QMessageBox props(QMessageBox::Information, "", descr, QMessageBox::Ok, this);
    props.exec();
  }
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::ConfigViewContextMenu(const QPoint& position)
{
#if defined pqMemoryInspectorPanelDEBUG
  cerr << ":::::pqMemoryInspectorPanel::ConfigContextMenu" << endl;
#endif

  QMenu context;

  QTreeWidgetItem* item = this->Ui->configView->itemAt(position);
  if (item)
  {
    bool ok;
    int procType = item->data(0, ITEM_KEY_PROCESS_TYPE).toInt(&ok);
    if (!ok)
    {
      return;
    }
    switch (procType)
    {
      case ITEM_DATA_CLIENT_GROUP:
      {
        QTreeWidgetItem* child = item->child(0);
        if (child == NULL)
          return;
        context.addAction("properties...", this, SLOT(ShowHostPropertiesDialog()));
        int sysType = child->data(0, ITEM_KEY_SYSTEM_TYPE).toInt(&ok);
        if (!ok)
          return;
        if (sysType == ITEM_DATA_UNIX_SYSTEM)
        {
          int serverType = child->data(0, ITEM_KEY_PVSERVER_TYPE).toInt(&ok);
          if (!ok)
            return;

          this->AddEnableStackTraceMenuAction(serverType, context);
          context.addAction("remote command...", this, SLOT(ExecuteRemoteCommand()));
        }
      }
      break;

      case ITEM_DATA_SERVER_GROUP:
      {
        context.addAction("show only nodes", this, SLOT(ShowOnlyNodes()));
        context.addAction("show all ranks", this, SLOT(ShowAllRanks()));

        QTreeWidgetItem* child = item->child(0);
        if (child == NULL)
          return;
        int sysType = child->data(0, ITEM_KEY_SYSTEM_TYPE).toInt(&ok);
        if (!ok)
          return;
        int serverType = child->data(0, ITEM_KEY_PVSERVER_TYPE).toInt(&ok);
        if (!ok)
          return;
        if (sysType == ITEM_DATA_UNIX_SYSTEM)
        {
          this->AddEnableStackTraceMenuAction(serverType, context);
        }
      }
      break;

      case ITEM_DATA_CLIENT_HOST:
      case ITEM_DATA_CLIENT_RANK:
      {
        context.addAction("properties...", this, SLOT(ShowHostPropertiesDialog()));
        int sysType = item->data(0, ITEM_KEY_SYSTEM_TYPE).toInt(&ok);
        if (!ok)
          return;
        if (sysType == ITEM_DATA_UNIX_SYSTEM)
        {
          context.addAction("remote command...", this, SLOT(ExecuteRemoteCommand()));
        }
      }
      break;

      case ITEM_DATA_SERVER_HOST:
      {
        context.addAction("properties...", this, SLOT(ShowHostPropertiesDialog()));
      }
      break;

      case ITEM_DATA_SERVER_RANK:
      {
        context.addAction("show only nodes", this, SLOT(ShowOnlyNodes()));
        context.addAction("show all ranks", this, SLOT(ShowAllRanks()));

        int sysType = item->data(0, ITEM_KEY_SYSTEM_TYPE).toInt(&ok);
        if (!ok)
          return;
        if (sysType == ITEM_DATA_UNIX_SYSTEM)
        {
          context.addAction("remote command...", this, SLOT(ExecuteRemoteCommand()));
        }
      }
      break;
    }
  }

  context.exec(this->Ui->configView->mapToGlobal(position));
}

//-----------------------------------------------------------------------------
void pqMemoryInspectorPanel::AddEnableStackTraceMenuAction(int serverType, QMenu& context)
{
  QAction* action = new QAction(this);
  action->setText("stack trace signal handler");
  action->setCheckable(true);
  switch (serverType)
  {
    case vtkProcessModule::PROCESS_CLIENT:
      action->setChecked(this->StackTraceOnClient);
      connect(action, SIGNAL(toggled(bool)), this, SLOT(EnableStackTraceOnClient(bool)));
      break;

    case vtkProcessModule::PROCESS_SERVER:
      action->setChecked(this->StackTraceOnServer);
      connect(action, SIGNAL(toggled(bool)), this, SLOT(EnableStackTraceOnServer(bool)));
      break;

    case vtkProcessModule::PROCESS_DATA_SERVER:
      action->setChecked(this->StackTraceOnDataServer);
      connect(action, SIGNAL(toggled(bool)), this, SLOT(EnableStackTraceOnDataServer(bool)));
      break;

    case vtkProcessModule::PROCESS_RENDER_SERVER:
      action->setChecked(this->StackTraceOnRenderServer);
      connect(action, SIGNAL(toggled(bool)), this, SLOT(EnableStackTraceOnRenderServer(bool)));
      break;
  }
  context.addAction(action);
}

//-----------------------------------------------------------------------------
QWidget* pqMemoryInspectorPanel::NewGroupWidget(string name, string icon)
{
  QHBoxLayout* hlayout = new QHBoxLayout;

  QLabel* label = new QLabel;
  label->setPixmap(QPixmap(icon.c_str()));
  label->setToolTip(name.c_str());
  hlayout->addWidget(label);

  label = new QLabel;
  label->setText(name.c_str());
  hlayout->addWidget(label);
  hlayout->addStretch(-1);

  QFrame* frame = new QFrame;
  frame->setLayout(hlayout);
  QFont ffont(frame->font());
  ffont.setPointSize(9);
  ffont.setBold(true);
  frame->setFont(ffont);

  return frame;
}
