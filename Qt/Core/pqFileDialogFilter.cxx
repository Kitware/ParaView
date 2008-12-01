/*=========================================================================

   Program: ParaView
   Module:    pqFileDialogFilter.cxx

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
  

#include "pqFileDialogFilter.h"

#include <QIcon>
#include <QFileIconProvider>

#include "pqFileDialogModel.h"

pqFileDialogFilter::pqFileDialogFilter(pqFileDialogModel* model, QObject* Parent)
  : QSortFilterProxyModel(Parent), Model(model)
{
  this->setSourceModel(model);
}

pqFileDialogFilter::~pqFileDialogFilter()
{
}

void pqFileDialogFilter::setFilter(const QStringList& wildcards)
{
  Wildcards.clear();
  foreach(QString p, wildcards)
    {
    Wildcards.append(QRegExp(p, Qt::CaseInsensitive, QRegExp::Wildcard));
    }
}

bool pqFileDialogFilter::filterAcceptsRow(int row_source, const QModelIndex& source_parent) const
{
  QModelIndex idx = this->Model->index(row_source, 0, source_parent);
  if(this->Model->isDir(idx))
    {
    return true;
    }

  QString str = this->sourceModel()->data(idx).toString();
  
  // file grouping removes extension from the above str,
  // so let's let the parent item of file groups through.
  // NOTE: file extension removal is now DISABLED in order
  // to fix bug #0008159.
  bool pass = false;

  int i=0, end=this->Wildcards.size();
  for(; i<end && pass == false; i++)
    {
    pass = this->Wildcards[i].exactMatch(str);
    }
  return pass;
}


