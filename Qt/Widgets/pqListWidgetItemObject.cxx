/*=========================================================================

   Program: ParaView
   Module:    pqListWidgetItemObject.cxx

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

#include "pqListWidgetItemObject.h"

pqListWidgetItemObject::pqListWidgetItemObject(const QString& t, QListWidget* p)
  : QListWidgetItem(t, p)
{
}

void pqListWidgetItemObject::setData(int role, const QVariant& v)
{
  if (Qt::CheckStateRole == role)
  {
    if (v != this->data(Qt::CheckStateRole))
    {
      QListWidgetItem::setData(role, v);
      Q_EMIT this->checkedStateChanged(Qt::Checked == v ? true : false);
    }
  }
  else
  {
    QListWidgetItem::setData(role, v);
  }
}

bool pqListWidgetItemObject::isChecked() const
{
  return Qt::Checked == this->checkState() ? true : false;
}

void pqListWidgetItemObject::setChecked(bool v)
{
  if (v)
  {
    this->setCheckState(Qt::Checked);
  }
  else
  {
    this->setCheckState(Qt::Unchecked);
  }
}
