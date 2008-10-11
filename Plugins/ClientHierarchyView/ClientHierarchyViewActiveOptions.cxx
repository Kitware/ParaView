/*=========================================================================

   Program: ParaView
   Module:    ClientHierarchyViewActiveOptions.cxx

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

#include "ClientHierarchyViewActiveOptions.h"
#include "ClientHierarchyViewOptions.h"

#include <pqOptionsDialog.h>

ClientHierarchyViewActiveOptions::ClientHierarchyViewActiveOptions(QObject *parentObject) :
  pqActiveViewOptions(parentObject)
{
}

ClientHierarchyViewActiveOptions::~ClientHierarchyViewActiveOptions()
{
}

void ClientHierarchyViewActiveOptions::showOptions(pqView *view, const QString &page, QWidget *widgetParent)
{
  if(!this->Dialog)
    {
    this->Options = new ClientHierarchyViewOptions;

    this->Dialog = new pqOptionsDialog(widgetParent);
    this->Dialog->setApplyNeeded(true);
    this->Dialog->setObjectName("ActiveClientHierarchyViewOptions");
    this->Dialog->setWindowTitle("Hierarchy View Options");
    this->Dialog->addOptions(this->Options);
    if(page.isEmpty())
      {
      QStringList pages = this->Options->getPageList();
      if(pages.size())
        {
        this->Dialog->setCurrentPage(pages[0]);
        }
      }
    else
      {
      this->Dialog->setCurrentPage(page);
      }
    
    this->connect(this->Dialog, SIGNAL(finished(int)),
        this, SLOT(finishDialog()));
    }

  this->changeView(view);
  this->Dialog->show();
}

void ClientHierarchyViewActiveOptions::changeView(pqView *view)
{
  this->Options->setView(view);
}

void ClientHierarchyViewActiveOptions::closeOptions()
{
  if(this->Dialog)
    {
    this->Dialog->accept();
    }
}

void ClientHierarchyViewActiveOptions::finishDialog()
{
  emit this->optionsClosed(this);
}

