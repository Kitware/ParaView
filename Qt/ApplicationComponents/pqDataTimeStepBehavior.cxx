/*=========================================================================

   Program: ParaView
   Module:    pqDataTimeStepBehavior.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqDataTimeStepBehavior.h"

#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqTimeKeeper.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

//-----------------------------------------------------------------------------
pqDataTimeStepBehavior::pqDataTimeStepBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getObjectBuilder(),
    SIGNAL(readerCreated(pqPipelineSource*, const QStringList&)),
    this, SLOT(onReaderCreated(pqPipelineSource*)),
    Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
void pqDataTimeStepBehavior::onReaderCreated(pqPipelineSource* reader)
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  // FIXME: Need some better organization for keep track for such global
  // settings.
  if (settings->value("DefaultTimeStepMode",0) == 0)
    {
    return;
    }

  pqTimeKeeper* timeKeeper = reader->getServer()->getTimeKeeper();
  pqAnimationScene* scene =
    pqApplicationCore::instance()->getServerManagerModel()->findItems<pqAnimationScene*>(
      reader->getServer())[0];
  vtkSMProxy* readerProxy = reader->getProxy();
  if (readerProxy->GetProperty("TimestepValues"))
    {
    vtkSMPropertyHelper helper(readerProxy, "TimestepValues");
    unsigned int num_timesteps = helper.GetNumberOfElements();
    const double *timesteps = helper.GetAsDoublePtr();
    if (num_timesteps > 1)
      {
      if (timeKeeper->getTime() < timesteps[num_timesteps-1])
        {
        scene->setAnimationTime(timesteps[num_timesteps-1]);
        }
      }
    }
  else if (readerProxy->GetProperty("TimeRange"))
    {
    double max_time = vtkSMPropertyHelper(readerProxy, "TimeRange").GetAsDouble(1);
    if (timeKeeper->getTime() < max_time)
      {
      scene->setAnimationTime(max_time);
      }
    }
}


