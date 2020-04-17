/*=========================================================================

   Program: ParaView
   Module:    pqTreeWidgetItemObject.cxx

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

#include "pqTreeWidgetItemObject.h"

pqTreeWidgetItemObject::pqTreeWidgetItemObject(const QStringList& t, int item_type)
  : QTreeWidgetItem(t, item_type)
{
}

pqTreeWidgetItemObject::pqTreeWidgetItemObject(QTreeWidget* p, const QStringList& t, int item_type)
  : QTreeWidgetItem(p, t, item_type)
{
}

pqTreeWidgetItemObject::pqTreeWidgetItemObject(
  QTreeWidgetItem* p, const QStringList& t, int item_type)
  : QTreeWidgetItem(p, t, item_type)
{
}

void pqTreeWidgetItemObject::setData(int column, int role, const QVariant& v)
{
  if (Qt::CheckStateRole == role)
  {
    if (v != this->data(column, Qt::CheckStateRole))
    {
      QTreeWidgetItem::setData(column, role, v);
      Q_EMIT this->checkedStateChanged(Qt::Checked == v ? true : false);
    }
  }
  else
  {
    QTreeWidgetItem::setData(column, role, v);
  }
  Q_EMIT this->modified();
}

bool pqTreeWidgetItemObject::isChecked() const
{
  return Qt::Checked == this->checkState(0) ? true : false;
}

void pqTreeWidgetItemObject::setChecked(bool v)
{
  if (v)
  {
    this->setCheckState(0, Qt::Checked);
  }
  else
  {
    this->setCheckState(0, Qt::Unchecked);
  }
}
