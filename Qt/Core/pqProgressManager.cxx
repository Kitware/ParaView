/*=========================================================================

   Program: ParaView
   Module:    pqProgressManager.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqProgressManager.h"

#include <QtDebug>
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>

//-----------------------------------------------------------------------------
pqProgressManager::pqProgressManager(QObject* _parent)
  : QObject(_parent)
{
  this->ProgressCount = 0;
  QApplication::instance()->installEventFilter(this); 
}

//-----------------------------------------------------------------------------
pqProgressManager::~pqProgressManager()
{
}


//-----------------------------------------------------------------------------
bool pqProgressManager::eventFilter(QObject* obj, QEvent* evt)
{
  if (this->ProgressCount != 0)
    {
    if (dynamic_cast<QKeyEvent*>(evt) || dynamic_cast<QMouseEvent*>(evt))
      {
      if (!this->NonBlockableObjects.contains(obj))
        {
        return true;
        }
      }
    }

  return QObject::eventFilter(obj, evt);
}

//-----------------------------------------------------------------------------
void pqProgressManager::lockProgress(QObject* object)
{
  if (!object)
    {
    return;
    }

  if (this->Lock)
    {
    qDebug() << "Progress is already locked.";
    return;
    }
  this->Lock = object;
}

//-----------------------------------------------------------------------------
void pqProgressManager::unlockProgress(QObject* object)
{
  if (!object)
    {
    return;
    }

  if (this->Lock == object)
    {
    this->Lock = 0;
    }
}

//-----------------------------------------------------------------------------
bool pqProgressManager::isLocked() const
{
  return (this->Lock != 0);
}

//-----------------------------------------------------------------------------
void pqProgressManager::setProgress(const QString& message, int progress_val)
{
  if (this->Lock && this->Lock != this->sender())
    {
    // When locked, ignore all other senders.
    return;
    }
  emit this->progress(message, progress_val);
}

//-----------------------------------------------------------------------------
void pqProgressManager::setEnableAbort(bool enable)
{
  if (this->Lock && this->Lock != this->sender())
    {
    // When locked, ignore all other senders.
    return;
    }
  emit this->enableAbort(enable);
}

//-----------------------------------------------------------------------------
void pqProgressManager::setEnableProgress(bool enable)
{
  if (this->Lock && this->Lock != this->sender())
    {
    // When locked, ignore all other senders.
    return;
    }
  this->ProgressCount += (enable? 1 : -1);
  if (this->ProgressCount < 0)
    {
    this->ProgressCount = 0;
    }

  if (this->ProgressCount <= 1)
    {
    emit this->enableProgress(enable);
    }
}

//-----------------------------------------------------------------------------
void pqProgressManager::triggerAbort()
{
  emit this->abort();
}
