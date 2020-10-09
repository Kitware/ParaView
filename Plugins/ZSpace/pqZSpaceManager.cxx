/*=========================================================================

   Program: ParaView
   Module:    pqZSpaceManager.cxx

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
#include "pqZSpaceManager.h"

#include "pqApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "vtkSMViewProxy.h"

//-----------------------------------------------------------------------------
pqZSpaceManager::pqZSpaceManager(QObject* p)
  : QObject(p)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(
    smmodel, &pqServerManagerModel::preViewAdded, this, &pqZSpaceManager::onViewAdded);
  QObject::connect(
    smmodel, &pqServerManagerModel::preViewRemoved, this, &pqZSpaceManager::onViewRemoved);

  // Add currently existing ZSpace views
  for (pqView* view : smmodel->findItems<pqView*>())
  {
    this->onViewAdded(view);
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onRenderEnded()
{
  pqView* view = dynamic_cast<pqView*>(sender());
  if (view != nullptr)
  {
    view->render();
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onViewAdded(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    const std::string viewName = view->getViewProxy()->GetVTKClassName();
    if (viewName == "vtkPVZSpaceView")
    {
      this->ZSpaceViews.insert(view);
      QObject::connect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));
    }
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onViewRemoved(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    const std::string viewName = view->getViewProxy()->GetVTKClassName();
    if (viewName == "vtkPVZSpaceView")
    {
      QObject::disconnect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));
      this->ZSpaceViews.erase(view);
    }
  }
}
