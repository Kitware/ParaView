/*=========================================================================

   Program: ParaView
   Module:    pqProgressManager.cxx

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

========================================================================*/
#include "pqProgressManager.h"

#include <QtDebug>
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkTimerLog.h"

//-----------------------------------------------------------------------------
pqProgressManager::pqProgressManager(QObject* _parent)
  : QObject(_parent)
{
  this->ProgressCount = 0;
  this->InUpdate = false;
  QApplication::instance()->installEventFilter(this); 

  this->EnableProgress = false;
  this->ReadyEnableProgress = false;
  this->LastProgressTime = 0;

  this->VTKConnect = vtkEventQtSlotConnect::New();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  this->VTKConnect->Connect(pm, vtkCommand::StartEvent,
    this, SLOT(onStartProgress()));
  this->VTKConnect->Connect(pm, vtkCommand::EndEvent,
    this, SLOT(onEndProgress()));
  this->VTKConnect->Connect(pm, vtkCommand::ProgressEvent,
    this, SLOT(onProgress()));
}

//-----------------------------------------------------------------------------
pqProgressManager::~pqProgressManager()
{
  this->VTKConnect->Delete();
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
  if (this->InUpdate)
    {
    return;
    }
  this->InUpdate = true;
  emit this->progress(message, progress_val);
  this->InUpdate = false;
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

  if (this->InUpdate)
    {
    return;
    }  this->InUpdate = true;
  if (this->ProgressCount <= 1)
    {
    emit this->enableProgress(enable);
    }
  this->InUpdate = false;
}

//-----------------------------------------------------------------------------
void pqProgressManager::triggerAbort()
{
  emit this->abort();
}

//-----------------------------------------------------------------------------
void pqProgressManager::onStartProgress()
{
  this->ReadyEnableProgress = true;
}

//-----------------------------------------------------------------------------
void pqProgressManager::onEndProgress()
{
  this->ReadyEnableProgress = false;
  if (this->EnableProgress)
    {
    this->setEnableProgress(false);
    }
  this->EnableProgress = false;
}

//-----------------------------------------------------------------------------
void pqProgressManager::onProgress()
{
  int oldProgress = vtkProcessModule::GetProcessModule()->GetLastProgress();
  QString text = vtkProcessModule::GetProcessModule()->GetLastProgressName();

  // forgive those who don't call SendPrepareProgress beforehand
  if (this->EnableProgress == false &&
    this->ReadyEnableProgress == false && oldProgress == 0)
    {
    this->onStartProgress();
    return;
    }

  // forgive those who don't cleanup or want to go the extra mile
  if (oldProgress >= 100)
    {
    this->onEndProgress();
    return;
    }

  // only forward progress events to the GUI if we get at least .05 seconds
  // since the last time we forwarded the progress event
  double lastprog = vtkTimerLog::GetUniversalTime();
  if (lastprog - this->LastProgressTime < .05)
    {
    return;
    }

  // We will show progress. Reset timer.
  this->LastProgressTime = vtkTimerLog::GetUniversalTime();

  // delayed progress starting so the progress bar doesn't flicker
  // so much for the quick operations
  if (this->EnableProgress == false)
    {
    this->EnableProgress = true;
    this->setEnableProgress(true);
    }

  this->LastProgressTime = lastprog;

  // chop of "vtk" prefix
  if (text.startsWith("vtk"))
    {
    text = text.mid(3);
    }
  this->setProgress(text, oldProgress);
}
