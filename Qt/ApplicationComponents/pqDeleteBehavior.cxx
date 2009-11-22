/*=========================================================================

   Program: ParaView
   Module:    pqDeleteBehavior.cxx

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
#include "pqDeleteBehavior.h"

#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqServer.h"
#include "pqServerManagerSelectionModel.h"
#include "pqView.h"

//-----------------------------------------------------------------------------
pqDeleteBehavior::pqDeleteBehavior(QObject* parentObject):
  Superclass(parentObject)
{
  QObject::connect(
    pqApplicationCore::instance()->getObjectBuilder(),
    SIGNAL(destroying(pqPipelineSource*)),
    this, SLOT(removeSource(pqPipelineSource*)));
}

//-----------------------------------------------------------------------------
void pqDeleteBehavior::removeSource(pqPipelineSource* source)
{
  // FIXME: updating of selection must happen even is the source is removed
  // from python script or undo redo.
  // If the source is selected, remove it from the selection.
  pqApplicationCore *core = pqApplicationCore::instance();
  pqServerManagerSelectionModel *selectionModel = core->getSelectionModel();
  pqPipelineFilter *filter = qobject_cast<pqPipelineFilter *>(source);
  if (selectionModel->isSelected(source) && selectionModel->selectedItems()->size() == 1)
    {
    // If the item is a filter and has only one input, set the
    // input as the current item. Otherwise, select the server.
    if (filter && filter->getInputCount() == 1)
      {
      selectionModel->setCurrentItem(filter->getInput(0),
        pqServerManagerSelectionModel::ClearAndSelect);
      }
    else
      {
      selectionModel->setCurrentItem(source->getServer(),
        pqServerManagerSelectionModel::ClearAndSelect);
      }
    }

  QList<pqView*> views = source->getViews();
  if (filter)
    {
    // Make all inputs visible in views that the removed source
    // is currently visible in.
    QList<pqOutputPort*> inputs = filter->getInputs();
    foreach(pqView* view, views)
      {
      pqDataRepresentation* src_disp = source->getRepresentation(view);
      if (!src_disp || !src_disp->isVisible())
        {
        continue;
        }
      // For each input, if it is not visible in any of the views
      // that the delete filter is visible, we make the input visible.
      for(int cc=0; cc < inputs.size(); ++cc)
        {
        pqPipelineSource* input = inputs[cc]->getSource();
        pqDataRepresentation* input_disp = input->getRepresentation(view);
        if (input_disp && !input_disp->isVisible())
          {
          input_disp->setVisible(true);
          }
        }
      }

// FIXME  --- TODO
//    if (filter->getProxy()->GetProperty("TimestepValues")
//        || filter->getProxy()->GetProperty("TimeRange") )
//      {
//      pqMainWindowCoreHideInputTimes(filter, false);
//      }
    }

  foreach (pqView* view, views)
    {
    // this triggers an eventually render call.
    view->render();
    }
}


