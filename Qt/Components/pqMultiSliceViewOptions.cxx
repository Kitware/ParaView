/*=========================================================================

   Program: ParaView
   Module:  pqMultiSliceViewOptions.cxx

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

#include "pqMultiSliceViewOptions.h"
#include "ui_pqMultiSliceViewOptions.h"

#include "vtkSMRenderViewProxy.h"
#include "vtkSMPropertyHelper.h"

#include <QHBoxLayout>

#include "pqMultiSliceView.h"

#include "pqActiveView.h"

class pqMultiSliceViewOptions::pqInternal
{
public:
  Ui::pqMultiSliceViewOptions ui;
};

//----------------------------------------------------------------------------
pqMultiSliceViewOptions::pqMultiSliceViewOptions(QWidget *widgetParent)
  : pqOptionsContainer(widgetParent)
{
  this->Internal = new pqInternal();
  this->Internal->ui.setupUi(this);

  // Show outline/cubeaxes/orientation axes
  QObject::connect( this->Internal->ui.ShowOutline,
                    SIGNAL(stateChanged(int)),
                    this, SIGNAL(changesAvailable()));
}

//----------------------------------------------------------------------------
pqMultiSliceViewOptions::~pqMultiSliceViewOptions()
{
}

//----------------------------------------------------------------------------
void pqMultiSliceViewOptions::setPage(const QString&)
{
}

//----------------------------------------------------------------------------
QStringList pqMultiSliceViewOptions::getPageList()
{
  QStringList ret;
  ret << "MultiSlice View";
  return ret;
}

//----------------------------------------------------------------------------
void pqMultiSliceViewOptions::applyChanges()
{
  if(!this->View)
    {
    return;
    }

  // Update View options...
  this->View->setOutlineVisibility(this->Internal->ui.ShowOutline->isChecked());

  this->View->render();
}

//----------------------------------------------------------------------------
void pqMultiSliceViewOptions::resetChanges()
{
  if(!this->View)
    {
    return;
    }

  this->setView(this->View);
}

//----------------------------------------------------------------------------
void pqMultiSliceViewOptions::setView(pqView* view)
{
  this->View = qobject_cast<pqMultiSliceView*>(view);

  if(!this->View)
    {
    return;
    }

  // Update UI with the current values
  this->Internal->ui.ShowOutline->setChecked(this->View->getOutlineVisibility());
}
