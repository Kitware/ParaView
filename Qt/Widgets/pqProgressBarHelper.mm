/*=========================================================================

   Program: ParaView
   Module:    pqProgressBarHelper.mm

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

#include "pqProgressBarHelper.h"
#include "pqProgressBar.h"
#include <QHBoxLayout>

#ifndef QT_MAC_USE_COCOA
#include <Carbon/Carbon.h>
#endif

pqProgressBarHelper::pqProgressBarHelper(pqProgressBar* p)
    : QWidget(p, Qt::Tool | Qt::FramelessWindowHint), ParentProgress(p)
{
  this->Progress = new QProgressBar(this);
  QHBoxLayout* l = new QHBoxLayout(this);
  l->setMargin(0);
  l->addWidget(this->Progress);
}
  
void pqProgressBarHelper::setFormat(const QString& fmt)
{
  this->Progress->setFormat(fmt);
}

void pqProgressBarHelper::setProgress(int num)
{
  this->Progress->setValue(num);
   
  // update the progress bar on the Mac
  // Qt only posts a request to update the window
  // we really want it to happen now
#ifndef QT_MAC_USE_COCOA 
  HIViewRef thisView = HIViewRef(this->Progress->winId());
  HIViewSetNeedsDisplay(thisView, true);
  HIWindowFlush(HIViewGetWindow(thisView));
#else
  OSViewRef view = reinterpret_cast<OSViewRef>(this->Progress->winId());
  [view setNeedsDisplay:YES];
  OSWindowRef wnd = [view window];
  [wnd flushWindowIfNeeded];
#endif
}
  
void pqProgressBarHelper::enableProgress(bool e)
{
  if(e)
    {
    QSize sz = this->ParentProgress->size();
    QRect r(this->ParentProgress->mapToGlobal(QPoint(0,0)), sz);
    this->setGeometry(r);
    this->Progress->setAlignment(this->ParentProgress->alignment());
    this->Progress->setMaximum(this->ParentProgress->maximum());
    this->Progress->setMinimum(this->ParentProgress->minimum());
    this->Progress->setOrientation(this->ParentProgress->orientation());
    this->Progress->reset();
    this->show();
    }
  else
    {
    this->hide();
    }
}

bool pqProgressBarHelper::progressEnabled() const
{
  return this->isVisible();
}

