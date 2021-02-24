/*=========================================================================

   Program: ParaView
   Module:    pqSectionVisibilityContextMenu.cxx

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
#include "pqSectionVisibilityContextMenu.h"

#include <QHeaderView>

#include "pqSetName.h"

//-----------------------------------------------------------------------------
pqSectionVisibilityContextMenu::pqSectionVisibilityContextMenu(QWidget* _p)
  : QMenu(_p)
{
  this->HeaderView = nullptr;
  QObject::connect(
    this, SIGNAL(triggered(QAction*)), this, SLOT(toggleSectionVisibility(QAction*)));
}

//-----------------------------------------------------------------------------
pqSectionVisibilityContextMenu::~pqSectionVisibilityContextMenu() = default;

//-----------------------------------------------------------------------------
void pqSectionVisibilityContextMenu::setHeaderView(QHeaderView* header)
{
  this->clear();
  this->HeaderView = header;
  if (header)
  {
    QAbstractItemModel* model = header->model();
    for (int cc = 0; cc < header->count(); cc++)
    {
      QString headertext = model->headerData(cc, header->orientation()).toString();
      QAction* action = this->addAction(headertext) << pqSetName(headertext);
      action->setCheckable(true);
      action->setChecked(!header->isSectionHidden(cc));
    }
  }
}

//-----------------------------------------------------------------------------
void pqSectionVisibilityContextMenu::toggleSectionVisibility(QAction* action)
{
  QHeaderView* header = this->HeaderView;
  if (!header)
  {
    return;
  }
  QString headertext = action->text();
  QAbstractItemModel* model = header->model();
  for (int cc = 0; cc < header->count(); cc++)
  {
    if (headertext == model->headerData(cc, Qt::Horizontal).toString())
    {
      if (action->isChecked())
      {
        header->showSection(cc);
      }
      else
      {
        header->hideSection(cc);
      }
      break;
    }
  }
}
