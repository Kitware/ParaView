/*=========================================================================

   Program: ParaView
   Module:    pqTableView.cxx

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
#include "pqTableView.h"

#include "pqHistogramTableModel.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqRepresentation.h"
#include "pqServer.h"

#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkRectilinearGrid.h>
#include <vtkSMProxy.h>

#include <QtDebug>
#include <QPointer>
#include <QStandardItemModel>
#include <QTableView>

//-----------------------------------------------------------------------------
class pqTableView::pqImplementation
{
public:
  pqImplementation() :
    Table(new QTableView())
  {
  }

  QPointer<QTableView> Table;
  QPointer<QWidget> WindowParent;
};

//-----------------------------------------------------------------------------
pqTableView::pqTableView(
    const QString& group,
    const QString& name, 
    vtkSMViewProxy* renModule,
    pqServer* server,
    QObject* _parent) :
  pqView(
    tableType(), group, name, renModule, server, _parent),
  Implementation(new pqImplementation())
{
}

//-----------------------------------------------------------------------------
pqTableView::~pqTableView()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
QWidget* pqTableView::getWidget()
{
  return this->Implementation->Table;
}

//-----------------------------------------------------------------------------
void pqTableView::visibilityChanged(pqRepresentation* /*disp*/)
{
}

//-----------------------------------------------------------------------------
void pqTableView::forceRender()
{
  this->Superclass::forceRender();

  const QList<pqRepresentation*> pqdisplays = this->getRepresentations();
  foreach(pqRepresentation* pqRepresentation, pqdisplays)
    {
    if(!pqRepresentation->isVisible())
      continue;
#ifdef FIXME
    vtkSMClientDeliveryRepresentationProxy* const display = 
      vtkSMClientDeliveryRepresentationProxy::SafeDownCast(pqRepresentation->getProxy());
      
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
#endif

    return;
    }
    
  delete this->Implementation->Table->model();
  this->Implementation->Table->setModel(new QStandardItemModel());
}
  

bool pqTableView::canDisplay(pqOutputPort* opPort) const
{
  pqPipelineSource* source = opPort? opPort->getSource() : 0;
  if(!source ||
     this->getServer()->GetConnectionID() !=
     source->getServer()->GetConnectionID())
    {
    return false;
    }
  return true;
}


