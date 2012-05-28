/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqMemoryInspector.h"
#include "ui_pqMemoryInspector.h"

#include <QAbstractTableModel>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QtDebug>
#include <QTextEdit>

#include "pqActiveObjects.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqNonEditableStyledItemDelegate.h"
#include "pqServer.h"
#include "vtkNew.h"
#include "vtkPVSystemInformation.h"
#include "vtkSMSession.h"

#include <map>
#include <algorithm>

namespace
{
  static QString tooltipTemplate()
    {
    static const char* tttemplate = 
    "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
    "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\"> p, li { white-space: pre-wrap; } </style></head><body style=\" font-family:'Helvetica'; font-size:9pt; font-weight:400; font-style:normal;\">"
    "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:12pt; font-weight:600; text-decoration: underline;\">%1</span></p>"
    "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Process Number: </span>%2/%3</p>"
    "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Hostname </span>: %4</p>"
    "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">OS:</span> %5 <span style=\" font-weight:600;\">(%6-bit)</span></p>"
    "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Memory Used:</span> %7 / %8</p>"
    "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Number of Cores:</span> %9</p>"
    "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Number of Threads: </span> %10</p></body></html>";
    return tttemplate;
    }

  static void exportAsCSV(QAbstractTableModel* table, const QString& filename)
    {
    ofstream stream(filename.toAscii().data());
    if (!stream.is_open())
      {
      qCritical() << "Failed to open file : "<< filename;
      }
    for (int cc=0; cc < table->columnCount(); cc++)
      {
      if (cc > 0)
        {
        stream << ", ";
        }
      stream << table->headerData(cc,
        Qt::Horizontal).toString().toAscii().data();
      }
    for (int row=0; row < table->rowCount(); row++)
      {
      stream << "\n";
      for (int col=0; col < table->columnCount(); col++)
        {
        if (col > 0)
          {
          stream << ", ";
          }
        stream << table->data(table->index(row, col),
            Qt::UserRole).toString().toAscii().data();
        }
      }
    }

  static QVariant memorySizeToText(size_t size_in_mb, bool user_friendly=true)
    {
    if (!user_friendly)
      {
      return static_cast<unsigned int>(size_in_mb);
      }
    if (size_in_mb > 1024*1024)
      {
      return QString("%1 TBs").arg(
          QString::number(size_in_mb/(1024*1024.0), 'f', 2));
      }
    else if (size_in_mb > 1024)
      {
      return QString("%1 GBs").arg(
          QString::number(size_in_mb/(1024.0), 'f', 2));
      }
    return QString("%1 MBs").arg(static_cast<unsigned int>(size_in_mb));
    }

  static QString processTypeToText(int type)
    {
    switch (type)
      {
    case vtkProcessModule::PROCESS_CLIENT:
      return "paraview";
    case vtkProcessModule::PROCESS_SERVER:
      return "pvserver";
    case vtkProcessModule::PROCESS_DATA_SERVER:
      return "pvdataserver";
    case vtkProcessModule::PROCESS_RENDER_SERVER:
      return "pvrenderserver";
      }
    return "(unknown)";
    }

  class pqMemoryInspectorModel : public QAbstractTableModel
  {
  typedef QAbstractTableModel Superclass;
  vtkNew<vtkPVSystemInformation> Information;
  bool UsePhysicalMemory;
public:
  enum
    {
    PROCESS_NAME,
    MEMORY_USED,
    MEMORY_FREE,
    HOSTNAME,
    TOTAL_MEMORY
    };
  pqMemoryInspectorModel(QObject* parentObject = 0) :
    QAbstractTableModel(parentObject),
    UsePhysicalMemory(false)
  {
  }
  void setUsePhysicalMemory(bool val)
    {
    this->UsePhysicalMemory = val;
    }

  void dataRefreshed()
    {
    this->reset();
    }
  vtkPVSystemInformation* GetInformation() const
    {
    return this->Information.GetPointer();
    }
  virtual int rowCount(const QModelIndex &idx=QModelIndex()) const
    {
    (void)idx;
    return static_cast<int>(this->Information->GetSystemInformations().size());
    }
  virtual int columnCount(const QModelIndex& idx=QModelIndex()) const
    {
    (void)idx;
    // Process #, Memory Used, Memory Free, Hostname, Total Memory
    return 5;
    }
  virtual QVariant data(const QModelIndex& idx, int role=Qt::DisplayRole) const
    {
    if (role != Qt::DisplayRole && role != Qt::ToolTipRole &&
      role != Qt::UserRole && role != Qt::EditRole)
      {
      return QVariant();
      }

    const vtkPVSystemInformation::SystemInformationType& info
      = this->Information->GetSystemInformations()[idx.row()];
    if (role == Qt::ToolTipRole)
      {
      int row = idx.row();
      // show a summary.
      return tooltipTemplate().arg(
        processTypeToText(info.ProcessType)).arg(
        info.ProcessId).arg(info.NumberOfProcesses).arg(
        info.Hostname.c_str()).arg(info.OSName.c_str()).arg(
        info.Is64Bits? "64":"32").arg(
        this->data(this->index(row, MEMORY_USED)).toString()).arg(
        this->data(this->index(row, MEMORY_FREE)).toString()).arg(
        info.NumberOfPhyicalCPUs).arg(info.NumberOfLogicalCPUs);
      }

    switch (idx.column())
      {
    case PROCESS_NAME:
        {
        QString text = processTypeToText(info.ProcessType);
        return (info.NumberOfProcesses > 1)?
          text + QString(" : %1").arg(info.ProcessId) :
          text;
        }
      break;

    case MEMORY_FREE:
      return memorySizeToText(this->UsePhysicalMemory?
        info.AvailablePhysicalMemory: info.AvailableVirtualMemory,
        role != Qt::UserRole);

    case MEMORY_USED:
      return memorySizeToText(
        this->UsePhysicalMemory?
        (info.TotalPhysicalMemory - info.AvailablePhysicalMemory) :
        (info.TotalVirtualMemory - info.AvailableVirtualMemory),
        role != Qt::UserRole);

    case TOTAL_MEMORY:
      return memorySizeToText(this->UsePhysicalMemory?
        info.TotalPhysicalMemory: info.TotalVirtualMemory,
        role != Qt::UserRole); 

    case HOSTNAME:
      return info.Hostname.c_str();
      }

    return QVariant();
    }

  virtual QVariant headerData(int section, Qt::Orientation orientation,
    int role = Qt::DisplayRole) const
    {
    static const char*column_titles[] =
      { "Process", "Memory Used", "Memory Free", "Hostname", "Total Memory" };

    if (orientation == Qt::Horizontal &&
      (role == Qt::DisplayRole || role == Qt::ToolTipRole))
      {
      return column_titles[section];
      }

    return this->Superclass::headerData(section, orientation, role);
    }

  /// Method needed for copy/past cell editor
  virtual Qt::ItemFlags flags ( const QModelIndex & idx) const
 {
    return QAbstractTableModel::flags(idx) | Qt::ItemIsEditable;
  }

  virtual bool setData ( const QModelIndex &, const QVariant &, int role = Qt::EditRole )
  {
  (void) role;
    // Fake edition...
    return true;
  }
  };
};
// Columns:
// Process #, Memory Used, Memory Free, Hostname, Total Memory
//

class pqMemoryInspector::pqIntenals : public Ui::MemoryInspector
{
public:
  pqMemoryInspectorModel Model;
  QSortFilterProxyModel ProxyModel;
  QString SummaryText;
  QString TooltipText;
};

//-----------------------------------------------------------------------------
pqMemoryInspector::pqMemoryInspector(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f),
  Internals(new pqIntenals())
{
  this->Internals->setupUi(this);

  // save the rich-text formatted text for later use.
  this->Internals->SummaryText = this->Internals->summary->document()->toHtml();
  this->Internals->summary->document()->setHtml("");

  this->Internals->ProxyModel.setSourceModel(&this->Internals->Model);
  this->Internals->tableView->setModel(
    &this->Internals->ProxyModel);
  this->Internals->tableView->setItemDelegate(new pqNonEditableStyledItemDelegate(this));
  QObject::connect(this->Internals->buttonBox,
    SIGNAL(accepted()), this, SLOT(refresh()));
  QObject::connect(this->Internals->physicalMemory,
    SIGNAL(toggled(bool)), this, SLOT(physicalMemoryToggled()));

  this->Internals->ProxyModel.setFilterKeyColumn(0);

  QObject::connect(this->Internals->filter,
    SIGNAL(textChanged(const QString&)),
    &this->Internals->ProxyModel,
    SLOT(setFilterWildcard(const QString&)));

  this->Internals->buttonBox->button(QDialogButtonBox::Ok)->setObjectName("Refresh");
  this->Internals->buttonBox->button(QDialogButtonBox::Ok)->setText("Refresh");
  this->Internals->buttonBox->button(QDialogButtonBox::Save)->setObjectName("Export");
  this->Internals->buttonBox->button(QDialogButtonBox::Save)->setText(
    "Export to CSV");
  QObject::connect(
    this->Internals->buttonBox->button(QDialogButtonBox::Save),
    SIGNAL(clicked()),
    this, SLOT(exportToCSV()));

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(serverChanged(pqServer*)),
    this, SLOT(refresh()));
}

//-----------------------------------------------------------------------------
pqMemoryInspector::~pqMemoryInspector()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqMemoryInspector::refresh()
{
  this->Internals->filter->setText("");
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (server)
    {
    vtkSMSession* session = server->session();
    session->GatherInformation(
      vtkSMSession::CLIENT_AND_SERVERS,
      this->Internals->Model.GetInformation(), 0);

    this->Internals->Model.setUsePhysicalMemory(
      this->Internals->physicalMemory->isChecked());
    this->updateSummary();
    this->Internals->Model.dataRefreshed();
    }
}

//-----------------------------------------------------------------------------
void pqMemoryInspector::physicalMemoryToggled()
{
  this->Internals->Model.setUsePhysicalMemory(
    this->Internals->physicalMemory->isChecked());
  this->Internals->Model.dataRefreshed();
}

//-----------------------------------------------------------------------------
namespace
{
  struct SummaryInfo
    {
    size_t PhysicalMin;
    size_t PhysicalMax;
    size_t PhysicalSum;
    size_t VirtualMin;
    size_t VirtualMax;
    size_t VirtualSum;
    int Count;
    SummaryInfo() :
      PhysicalMin(VTK_INT_MAX), PhysicalMax(0), PhysicalSum(0),
      VirtualMin(VTK_INT_MAX), VirtualMax(0), VirtualSum(0),
      Count(0) { }
    };

  class GatherSummary
    {
    std::map<int, SummaryInfo> &Map;
  public:
    GatherSummary(std::map<int, SummaryInfo>& map):
      Map(map) { }
    void operator() (
      const vtkPVSystemInformation::SystemInformationType& data)
      {
      SummaryInfo& info = this->Map[data.ProcessType];
      info.PhysicalMin = std::min(info.PhysicalMin,
        data.TotalPhysicalMemory - data.AvailablePhysicalMemory);
      info.PhysicalMax = std::max(info.PhysicalMax,
        data.TotalPhysicalMemory - data.AvailablePhysicalMemory);
      info.PhysicalSum += 
        (data.TotalPhysicalMemory - data.AvailablePhysicalMemory);

      info.VirtualMin = std::min(info.VirtualMin,
        data.TotalVirtualMemory - data.AvailableVirtualMemory);
      info.VirtualMax = std::max(info.VirtualMax,
        data.TotalVirtualMemory - data.AvailableVirtualMemory);
      info.VirtualSum += 
        (data.TotalVirtualMemory - data.AvailableVirtualMemory);

      info.Count++;
      }
    };
}

//-----------------------------------------------------------------------------
void pqMemoryInspector::updateSummary()
{
  std::map<int, SummaryInfo> collector;
  std::for_each(
    this->Internals->Model.GetInformation()->GetSystemInformations().begin(),
    this->Internals->Model.GetInformation()->GetSystemInformations().end(),
    GatherSummary(collector));
  QString text;

  for (std::map<int, SummaryInfo>::iterator iter = collector.begin();
  iter != collector.end(); ++iter)
    {
    text += this->Internals->SummaryText.arg(
      processTypeToText(iter->first)).arg(
      memorySizeToText(iter->second.PhysicalMin).toString()).arg(
      memorySizeToText(iter->second.PhysicalMax).toString()).arg(
      memorySizeToText(iter->second.PhysicalSum/iter->second.Count).toString()).arg(
      memorySizeToText(iter->second.VirtualMin).toString()).arg(
      memorySizeToText(iter->second.VirtualMax).toString()).arg(
      memorySizeToText(iter->second.VirtualSum/iter->second.Count).toString());
    }
  this->Internals->summary->document()->setHtml(text);

  // Try reduce height of the process summary
  int height1 = this->Internals->summary->heightForWidth(this->Internals->summary->width());
  if(height1 < 0)
    {
    height1 = this->Internals->summary->minimumSizeHint().height();
    }
  this->Internals->summary->setMaximumHeight(height1 + 10);
}

//-----------------------------------------------------------------------------
void pqMemoryInspector::exportToCSV()
{
  pqFileDialog dialog(NULL, pqCoreUtilities::mainWidget(),
    "Export as Comma-Separated Values");
  dialog.setFileMode(pqFileDialog::AnyFile);
  if (dialog.exec() == QDialog::Accepted &&
    dialog.getSelectedFiles(0).size() == 1)
    {
    exportAsCSV(
      &this->Internals->Model, dialog.getSelectedFiles(0)[0]);
    }
}
