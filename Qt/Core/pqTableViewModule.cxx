/*=========================================================================

   Program: ParaView
   Module:    pqTableViewModule.cxx

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

=========================================================================*/
#include "pqTableViewModule.h"

#include "pqDisplay.h"
#include "pqHistogramTableModel.h"
#include "pqPipelineSource.h"

#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkRectilinearGrid.h>
#include <vtkSMGenericViewDisplayProxy.h>
#include <vtkSMProxy.h>

#include <QtDebug>
#include <QPointer>
#include <QStandardItemModel>
#include <QTableView>

//-----------------------------------------------------------------------------
class pqTableViewModule::pqImplementation
{
public:
  pqImplementation() :
    Table(new QTableView())
  {
  }

  QPointer<QTableView> Table;
};

//-----------------------------------------------------------------------------
pqTableViewModule::pqTableViewModule(
    const QString& group,
    const QString& name, 
    vtkSMAbstractViewModuleProxy* renModule,
    pqServer* server,
    QObject* _parent) :
  pqGenericViewModule(group, name, renModule, server, _parent),
  Implementation(new pqImplementation())
{
}

//-----------------------------------------------------------------------------
pqTableViewModule::~pqTableViewModule()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
QWidget* pqTableViewModule::getWidget()
{
  return this->Implementation->Table;
}

//-----------------------------------------------------------------------------
void pqTableViewModule::setWindowParent(QWidget* /*p*/)
{
}
//-----------------------------------------------------------------------------
QWidget* pqTableViewModule::getWindowParent() const
{
  return 0;
}

//-----------------------------------------------------------------------------
void pqTableViewModule::visibilityChanged(pqDisplay* /*disp*/)
{
}

//-----------------------------------------------------------------------------
void pqTableViewModule::forceRender()
{
  this->Superclass::forceRender();

  const QList<pqDisplay*> pqdisplays = this->getDisplays();
  foreach(pqDisplay* pqdisplay, pqdisplays)
    {
    if(!pqdisplay->isVisible())
      continue;
      
    vtkSMGenericViewDisplayProxy* const display = 
      vtkSMGenericViewDisplayProxy::SafeDownCast(pqdisplay->getProxy());
      
    vtkDataObject* const data = display->GetOutput();

    if(vtkRectilinearGrid* const grid = vtkRectilinearGrid::SafeDownCast(data))
      {
      if(vtkDoubleArray* const bin_extents = vtkDoubleArray::SafeDownCast(grid->GetXCoordinates()))
        {
        if(vtkIntArray* const bin_values = vtkIntArray::SafeDownCast(
          grid->GetCellData()->GetArray("bin_values")))
          {
          if(bin_extents->GetNumberOfTuples() == bin_values->GetNumberOfTuples() + 1)
            {
            delete this->Implementation->Table->model();
            this->Implementation->Table->setModel(new pqHistogramTableModel(bin_extents, bin_values, this->Implementation->Table));
            }
          }
        }
      }

    return;
    }
    
  delete this->Implementation->Table->model();
  this->Implementation->Table->setModel(new QStandardItemModel());
}
