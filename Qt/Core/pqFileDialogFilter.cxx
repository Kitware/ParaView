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
#include <QStringBuilder>

#include "pqFileDialogModel.h"

pqFileDialogFilter::pqFileDialogFilter(pqFileDialogModel* model, QObject* Parent)
  : QSortFilterProxyModel(Parent), Model(model), showHidden(false)
{
  this->setSourceModel(model);
  this->Wildcards.setPatternSyntax(QRegExp::RegExp2);
  this->Wildcards.setCaseSensitivity(Qt::CaseSensitive);
}

pqFileDialogFilter::~pqFileDialogFilter()
{
}

#include <stdio.h>

void pqFileDialogFilter::setFilter(const QString& filter)
{
  QString f(filter);
  // if we have (...) in our filter, strip everything out but the contents of ()
  int start, end;
  end = filter.lastIndexOf(')');
  //we need to from the backwards incase the name of the filter is:
  // File Type (polydata) (*.ft)
  start = filter.lastIndexOf('(',end);
  if(start != -1 && end != -1)
    {
    f = f.mid(start+1, end-start-1);
    }
  // Now 'f' is going to be string like "*.ext1 *.ext2" or "spct* *pattern*"
  // etc. The following code converts it to a regular expression.
  QString pattern = ".*";
  if ( f != "*" )
    {
    f = f.trimmed();

    //convert all spaces into |
    f.replace(QRegExp("[\\s+;]+"),"|");

    QStringList strings = f.split("|");
    QStringList extensions_list, filepatterns_list;
    foreach (QString string, strings)
      {
      if (string.startsWith("*."))
        {
        extensions_list.push_back(string.remove(0, 2));
        }
      else
        {
        filepatterns_list.push_back(string);
        }
      }

    QString extensions = extensions_list.join("|");
    QString filepatterns = filepatterns_list.join("|");

    extensions.replace(".","\\.");
    extensions.replace("*", ".*");

    filepatterns.replace(".","\\.");
    filepatterns.replace("*", ".*");

    //use non capturing(?:) for speed
    //name.ext or ext.001 or name.ext001 (for bug #10101)
    QString postExtFileSeries("(\\.?\\d+)?$"); // match the .0001 component
    QString extGroup = ".*\\.(?:" % extensions % ")" % postExtFileSeries;
    QString fileGroup = "(?:" % filepatterns % ")" % postExtFileSeries;
    if (extensions_list.size() > 0 && filepatterns_list.size() > 0)
      {
      pattern = "(?:" % fileGroup % "|" % extGroup % ")";
      }
    else if (extensions_list.size() > 0)
      {
      pattern = extGroup;
      }
    else
      {
      pattern = fileGroup;
      }
    }

  this->Wildcards.setPattern(pattern);
  this->invalidateFilter();
}

void pqFileDialogFilter::setShowHidden( const bool &hidden)
{
  if ( this->showHidden != hidden )
    {
    this->showHidden = hidden;
    this->invalidateFilter();
    }

}

bool pqFileDialogFilter::filterAcceptsRow(int row_source, const QModelIndex& source_parent) const
{
  QModelIndex idx = this->Model->index(row_source, 0, source_parent);

  //hidden flag supersedes anything else
  if ( this->Model->isHidden(idx) && !this->showHidden)
    {
    return false;
    }

  if(this->Model->isDir(idx))
    {
    return true;
    }

  QString str = this->sourceModel()->data(idx).toString();
  bool pass = this->Wildcards.exactMatch(str);
  return pass;
}


