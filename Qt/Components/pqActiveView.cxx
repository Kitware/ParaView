/*=========================================================================

   Program: ParaView
   Module:    pqActiveView.cxx

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
#include "pqActiveView.h"

#include "vtkSMProxyManager.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkEventQtSlotConnect.h"

#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

//-----------------------------------------------------------------------------
pqActiveView& pqActiveView::instance()
{
  static pqActiveView the_instance;
  return the_instance;
}

//-----------------------------------------------------------------------------
pqActiveView::pqActiveView() : ActiveView(0)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  this->SMActiveView = pxm->GetSelectionModel("ActiveView");
  if (!this->SMActiveView)
    {
    this->SMActiveView = vtkSMProxySelectionModel::New();
    pxm->RegisterSelectionModel("ActiveView", this->SMActiveView);
    this->SMActiveView->Delete();
    }

  this->VTKConnect = vtkEventQtSlotConnect::New();
  this->VTKConnect->Connect(this->SMActiveView, vtkCommand::CurrentChangedEvent,
    this, SLOT(smCurrentChanged()));

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(viewRemoved(pqView*)),
    this, SLOT(onViewRemoved(pqView*)));
}

//-----------------------------------------------------------------------------
pqActiveView::~pqActiveView()
{
  this->VTKConnect->Delete();
}

//-----------------------------------------------------------------------------
pqView* pqActiveView::current()
{
  return this->ActiveView;
}

//-----------------------------------------------------------------------------
void pqActiveView::smCurrentChanged()
{
  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  pqView* view = smmodel->findItem<pqView*>(this->SMActiveView->GetCurrentProxy());
  if (this->ActiveView != view)
    {
    this->ActiveView = view;
    emit this->changed(view);
    }
}

//-----------------------------------------------------------------------------
void pqActiveView::setCurrent(pqView* view)
{
  vtkSMProxy* viewProxy = view? view->getProxy() : 0;
  this->SMActiveView->SetCurrentProxy(viewProxy,
    vtkSMProxySelectionModel::NO_UPDATE);
}

//-----------------------------------------------------------------------------
void pqActiveView::onViewRemoved(pqView* view)
{
  if (this->ActiveView == view)
    {
    this->setCurrent(0);
    }
}
