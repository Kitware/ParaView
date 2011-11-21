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

namespace
{
  class pqMemoryInspectorModel : public QAbstractTableModel
  {
  typedef QAbstractTableModel Superclass;
public:
  pqMemoryInspectorModel(QObject* parentObject) :
    QAbstractTableModel(parentObject)
  {
  }

  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const
    {
    (void)parent;
    return 1;// FIXME
    }
  virtual int columnCount(const QModelIndex& parent=QModelIndex()) const
    {
    (void)parent;
    // Process #, Memory Used, Memory Free, Hostname, Total Memory
    return 5;
    }
  virtual QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const
    {
    (void)index;
    (void)role;
    return index.row();
    }

  virtual QVariant headerData(int section, Qt::Orientation orientation,
    int role = Qt::DisplayRole) const
    {
    static const char*column_titles[] =
      { "Process #", "Memory Used", "Memory Free", "Hostname", "Total Memory" };

    if (orientation == Qt::Horizontal &&
      (role == Qt::DisplayRole || role == Qt::ToolTipRole))
      {
      return column_titles[section];
      }
    return this->Superclass::headerData(section, orientation, role);
    }
  };
};
// Columns:
// Process #, Memory Used, Memory Free, Hostname, Total Memory
//

class pqMemoryInspector::pqIntenals : public Ui::MemoryInspector
{
};

//-----------------------------------------------------------------------------
pqMemoryInspector::pqMemoryInspector(QWidget* parentObject, Qt::WindowFlags f)
  : Superclass(parentObject, f),
  Internals(new pqIntenals())
{
  this->Internals->setupUi(this);
  this->Internals->tableView->setModel(new pqMemoryInspectorModel(this));
}

//-----------------------------------------------------------------------------
pqMemoryInspector::~pqMemoryInspector()
{
  delete this->Internals;
}
