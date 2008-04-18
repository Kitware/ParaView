/*=========================================================================

   Program: ParaView
   Module:    pqActiveTwoDRenderViewOptions.cxx

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

/// \file pqActiveTwoDRenderViewOptions.cxx
/// \date 7/31/2007

#include "pqActiveTwoDRenderViewOptions.h"

#include <QPointer>

#include "pqTwoDRenderViewOptions.h"
#include "pqOptionsDialog.h"

class pqActiveTwoDRenderViewOptions::pqInternal
{
public:
  QPointer<pqOptionsDialog> Dialog;
  pqTwoDRenderViewOptions* Options;
};

//-----------------------------------------------------------------------------
pqActiveTwoDRenderViewOptions::pqActiveTwoDRenderViewOptions(QObject *parentObject)
  : pqActiveViewOptions(parentObject)
{
  this->Internal = new pqInternal;
}

//-----------------------------------------------------------------------------
pqActiveTwoDRenderViewOptions::~pqActiveTwoDRenderViewOptions()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqActiveTwoDRenderViewOptions::showOptions(pqView *view, const QString &page,
    QWidget *widgetParent)
{
  if(!this->Internal->Dialog)
    {
    this->Internal->Dialog = new pqOptionsDialog(widgetParent);
    this->Internal->Dialog->setApplyNeeded(true);
    this->Internal->Dialog->setObjectName("Active2DViewOptions");
    this->Internal->Dialog->setWindowTitle("2D View Options");
    this->Internal->Options = new pqTwoDRenderViewOptions;
    this->Internal->Dialog->addOptions(this->Internal->Options);
    if(page.isEmpty())
      {
      QStringList pages = this->Internal->Options->getPageList();
      if(pages.size())
        {
        this->Internal->Dialog->setCurrentPage(pages[0]);
        }
      }
    else
      {
      this->Internal->Dialog->setCurrentPage(page);
      }
    
    this->connect(this->Internal->Dialog, SIGNAL(finished(int)),
        this, SLOT(finishDialog()));
    }

  this->changeView(view);
  this->Internal->Dialog->show();
}

//-----------------------------------------------------------------------------
void pqActiveTwoDRenderViewOptions::changeView(pqView *view)
{
  this->Internal->Options->setView(view);
}

//-----------------------------------------------------------------------------
void pqActiveTwoDRenderViewOptions::closeOptions()
{
  if(this->Internal->Dialog)
    {
    this->Internal->Dialog->accept();
    }
}

//-----------------------------------------------------------------------------
void pqActiveTwoDRenderViewOptions::finishDialog()
{
  emit this->optionsClosed(this);
}


