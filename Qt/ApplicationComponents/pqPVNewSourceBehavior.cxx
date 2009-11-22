/*=========================================================================

   Program: ParaView
   Module:    pqPVNewSourceBehavior.cxx

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
#include "pqPVNewSourceBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqIgnoreSourceTimeReaction.h"
#include "pqObjectBuilder.h"
#include "pqPipelineFilter.h"
#include "vtkSMProxy.h"
#include "pqOutputPort.h"

//-----------------------------------------------------------------------------
pqPVNewSourceBehavior::pqPVNewSourceBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getObjectBuilder(),
    SIGNAL(proxyCreated(pqProxy*)),
    this, SLOT(activate(pqProxy*)));
}

// Go upstream till we find an input that has timesteps and hide its time.
static void pqPVNewSourceBehaviorHideInputTimes(pqPipelineFilter* filter,
  bool hide)
{
  if (!filter)
    {
    return;
    }
  QList<pqOutputPort*> inputs = filter->getAllInputs();
  foreach (pqOutputPort* input, inputs)
    {
    pqPipelineSource* source = input->getSource();
    if (   source->getProxy()->GetProperty("TimestepValues")
        || source->getProxy()->GetProperty("TimeRange") )
      {
      pqIgnoreSourceTimeReaction::ignoreSourceTime(source, hide);
      }
    else
      {
      pqPVNewSourceBehaviorHideInputTimes(
        qobject_cast<pqPipelineFilter*>(source), hide);
      }
    }
}

//-----------------------------------------------------------------------------
void pqPVNewSourceBehavior::activate(pqProxy* proxy)
{
  pqPipelineSource* src = qobject_cast<pqPipelineSource*>(proxy);
  if (src)
    {
    pqActiveObjects::instance().setActiveSource(src);
    }

  // If the newly created source is a filter has TimestepValues or TimeRange
  // then we assume that this is a "temporal" filter which may distort the
  // time. So we hide the timesteps from all the inputs.
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(proxy);
  if (filter && 
    (filter->getProxy()->GetProperty("TimestepValues")
     || filter->getProxy()->GetProperty("TimeRange")))
    {
    pqPVNewSourceBehaviorHideInputTimes(filter, true);
    }
}


