

#include "PrismToolBarActions.h"
#include "pqApplicationCore.h"
#include "pqServerManagerSelectionModel.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqObjectBuilder.h"
#include "vtkSMProxy.h"
#include <QApplication>
#include <QStyle>
#include <QMessageBox>

PrismToolBarActions::PrismToolBarActions(QObject* p)
  : QActionGroup(p)
{

  this->GeometryViewAction = new QAction("Geometry View",this);
  this->GeometryViewAction->setToolTip("Create Geometry View");
  this->GeometryViewAction->setIcon(QIcon(":/Prism/Icons/CreateGeometryView.png"));
  this->addAction(this->GeometryViewAction);

  this->SesameViewAction = new QAction("SESAME View",this);
  this->SesameViewAction->setToolTip("Create SESAME View");
  this->SesameViewAction->setIcon(QIcon(":/Prism/Icons/CreateSESAME.png"));
  this->addAction(this->SesameViewAction);

  QObject::connect(this->GeometryViewAction, SIGNAL(triggered(bool)), this, SLOT(onGeometryFileOpen()));
  QObject::connect(this->SesameViewAction, SIGNAL(triggered(bool)), this, SLOT(onSESAMEFileOpen()));


  pqServerManagerSelectionModel *selection =
      pqApplicationCore::instance()->getSelectionModel();
  this->connect(selection, SIGNAL(currentChanged(pqServerManagerModelItem*)),
      this, SLOT(onSelectionChanged()));
  this->connect(selection,
      SIGNAL(selectionChanged(const pqServerManagerSelection&, const pqServerManagerSelection&)),
      this, SLOT(onSelectionChanged()));

  this->onSelectionChanged();
}

PrismToolBarActions::~PrismToolBarActions()
{
  this->createFilterForActiveSource("PrismSurfaceFilter");

}

void PrismToolBarActions::onGeometryFileOpen()
{
  this->createFilterForActiveSource("PrismGeometryFilter");
}

void PrismToolBarActions::onSESAMEFileOpen()
{
}
//-----------------------------------------------------------------------------
pqPipelineSource* PrismToolBarActions::createFilterForActiveSource(
  const QString& xmlname)
{
  // Get the list of selected sources.
  pqApplicationCore* core = pqApplicationCore::instance();
 pqObjectBuilder* builder = core->getObjectBuilder();
   pqServerManagerSelection sels =
      *core->getSelectionModel()->selectedItems();
  pqPipelineSource* source = 0;
  pqPipelineSource* filter = 0;
  pqServerManagerModelItem* item = 0;
  pqServerManagerSelection::ConstIterator iter = sels.begin();
  if(iter != sels.end())
    {
    item = *iter;
    source = dynamic_cast<pqPipelineSource*>(item);
    filter = builder->createFilter("filters",xmlname, source);
    ++iter;
    }

  for( ; filter && iter != sels.end(); ++iter)
    {
    item = *iter;
    source = dynamic_cast<pqPipelineSource*>(item);
    builder->addConnection(source, filter);
    }

  return filter;
}

void PrismToolBarActions::onSelectionChanged()
{
  pqServerManagerModelItem *item = this->getActiveObject();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
  pqServer* server = dynamic_cast<pqServer*>(item);
  if (source)
    {
    QString name=source->getProxy()->GetXMLName();
    if(name=="GeometryView")
      {
      this->GeometryViewAction->setEnabled(false);
      this->SesameViewAction->setEnabled(true);
      }
    else if(name=="ExodusReader")
      {
      this->GeometryViewAction->setEnabled(true);
      this->SesameViewAction->setEnabled(false);
      }
    else
      {
      this->GeometryViewAction->setEnabled(false);
      this->SesameViewAction->setEnabled(false);
      }
    }
  else if(server)
    {
      this->GeometryViewAction->setEnabled(true);
      this->SesameViewAction->setEnabled(true);
    }
  else
    {
    this->GeometryViewAction->setEnabled(false);
    this->SesameViewAction->setEnabled(true);
    }
}
pqServerManagerModelItem *PrismToolBarActions::getActiveObject() const
{
  pqServerManagerModelItem *item = 0;
  pqServerManagerSelectionModel *selection =
      pqApplicationCore::instance()->getSelectionModel();
  const pqServerManagerSelection *sels = selection->selectedItems();
  if(sels->size() == 1)
    {
    item = sels->first();
    }
  else if(sels->size() > 1)
    {
    item = selection->currentItem();
    if(item && !selection->isSelected(item))
      {
      item = 0;
      }
    }

  return item;
}


