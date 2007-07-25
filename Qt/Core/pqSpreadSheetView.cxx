/*=========================================================================

   Program: ParaView
   Module:    pqSpreadSheetView.cxx

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
#include "pqSpreadSheetView.h"

// Server Manager Includes.
#include "vtkSMViewProxy.h"
#include "vtkSMBlockDeliveryRepresentationProxy.h"

// Qt Includes.
#include <QHeaderView>
#include <QItemDelegate>
#include <QPointer>
#include <QTableView>

// ParaView Includes.
#include "pqOutputPort.h"
#include "pqServer.h"
#include "pqRepresentation.h"
#include "pqSpreadSheetViewModel.h"

//-----------------------------------------------------------------------------
class pqSpreadSheetView::pqDelegate : public QItemDelegate
{
  typedef QItemDelegate Superclass;
public:
  pqDelegate(QObject* _parent=0):Superclass(_parent)
  {
  }
  void beginPaint()
    {
    this->Top = QModelIndex();
    this->Bottom = QModelIndex();
    }
  void endPaint()
    {
    }

  virtual void paint (QPainter* painter, const QStyleOptionViewItem& option, 
    const QModelIndex& index) const 
    {
    const_cast<pqDelegate*>(this)->Top = (this->Top.isValid() && this->Top < index)?
      this->Top : index;
    const_cast<pqDelegate*>(this)->Bottom = (this->Bottom.isValid() && index < this->Bottom)?
      this->Bottom : index;

    this->Superclass::paint(painter, option, index);
    }

  QModelIndex Top;
  QModelIndex Bottom;
};

//-----------------------------------------------------------------------------
/// As one scrolls through the table view, the view requests the data for all
/// elements scrolled through, not only the ones eventually visible. We do this
/// trick with pqTableView and pqDelegate to make the model request the data
/// only for the region eventually visible to the user.
class pqSpreadSheetView::pqTableView : public QTableView
{
  typedef QTableView Superclass;
protected:
  virtual void paintEvent (QPaintEvent* event)
    {
    pqSpreadSheetView::pqDelegate* del =
      dynamic_cast<pqDelegate*>(this->itemDelegate());
    pqSpreadSheetViewModel* smodel = 
      qobject_cast<pqSpreadSheetViewModel*>(this->model());
    if (del)
      {
      del->beginPaint();
      }
    this->Superclass::paintEvent(event);
    if (del)
      {
      del->endPaint();
      smodel->setActiveBlock(del->Top, del->Bottom);
      }
    }
};

//-----------------------------------------------------------------------------
class pqSpreadSheetView::pqInternal
{
public:
  pqInternal()
  {
  pqSpreadSheetView::pqTableView* table = new pqSpreadSheetView::pqTableView();
  pqSpreadSheetView::pqDelegate* delegate = new pqSpreadSheetView::pqDelegate(table);

  table->setItemDelegate(delegate);
  this->Table= table;
  this->Table->setModel(&this->Model);
  this->Table->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Table->horizontalHeader()->setMovable(true);
  }

  ~pqInternal()
    {
    delete this->Table;
    }

  QPointer<QTableView> Table;
  pqSpreadSheetViewModel Model;
};


//-----------------------------------------------------------------------------
pqSpreadSheetView::pqSpreadSheetView(
 const QString& group, const QString& name, 
    vtkSMViewProxy* viewModule, pqServer* server, 
    QObject* _parent/*=NULL*/):
   pqView(spreadsheetViewType(), group, name, viewModule, server, _parent)
{
  this->Internal = new pqInternal();
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    this, SLOT(onAddRepresentation(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
    this, SLOT(onRemoveRepresentation(pqRepresentation*)));
  QObject::connect(
    this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
    this, SLOT(updateRepresentationVisibility(pqRepresentation*, bool)));
  QObject::connect(this, SIGNAL(endRender()), this, SLOT(onEndRender()));
}

//-----------------------------------------------------------------------------
pqSpreadSheetView::~pqSpreadSheetView()
{
  delete this->Internal;
}


//-----------------------------------------------------------------------------
QWidget* pqSpreadSheetView::getWidget()
{
  return this->Internal->Table;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onAddRepresentation(pqRepresentation* repr)
{
  this->updateRepresentationVisibility(repr, repr->isVisible());
}


//-----------------------------------------------------------------------------
void pqSpreadSheetView::onRemoveRepresentation(pqRepresentation* repr)
{
  if (repr && repr->getProxy() == this->Internal->Model.getRepresentationProxy())
    {
    this->Internal->Model.setRepresentationProxy(0);
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::updateRepresentationVisibility(
  pqRepresentation* repr, bool visible)
{
  if (!visible || !repr)
    {
    return;
    }

  // If visible, turn-off visibility of all other representations.
  QList<pqRepresentation*> reprs = this->getRepresentations();
  foreach (pqRepresentation* cur_repr, reprs)
    {
    if (cur_repr != repr)
      {
      cur_repr->setVisible(false);
      }
    }

  this->Internal->Model.setRepresentationProxy(
    vtkSMBlockDeliveryRepresentationProxy::SafeDownCast(
      repr->getProxy()));
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onEndRender()
{
  cout << "Render" << endl;
  this->Internal->Model.forceUpdate();
}

//-----------------------------------------------------------------------------
bool pqSpreadSheetView::canDisplay(pqOutputPort* opPort) const
{
  return (opPort && opPort->getServer()->GetConnectionID() == 
    this->getServer()->GetConnectionID());
}
