#include "GraphLayoutFilterPanel.h"

#include <pqProxy.h>
#include <pqPropertyHelper.h>

#include <vtkSMProxy.h>
#include <vtkSMProxyDefinitionIterator.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>

#include <QLabel>
#include <QLayout>
#include <QMessageBox>

#include <iostream>
#include <set>

GraphLayoutFilterPanel::GraphLayoutFilterPanel(pqProxy* proxy, QWidget* p) :
  pqObjectPanel(proxy, p)
{
  this->Widgets.setupUi(this);

  vtkSMProxy* const table_to_graph = proxy->getProxy();

  vtkSMProxyDefinitionIterator* defnIter = vtkSMProxyDefinitionIterator::New();
  defnIter->SetModeToOneGroup();
  for (defnIter->Begin("layout_strategies"); !defnIter->IsAtEnd();
    defnIter->Next())
    {
    this->Widgets.layoutStrategy->addItem(defnIter->GetKey());
    }
  defnIter->Delete();

  QObject::connect(this->Widgets.layoutStrategy, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(setModified()));
}

void GraphLayoutFilterPanel::accept()
{
  vtkSMProxy* const proxy = this->referenceProxy()->getProxy();

  vtkSMProxy *layoutProxy = vtkSMProxyManager::GetProxyManager()->NewProxy("layout_strategies", this->Widgets.layoutStrategy->currentText().toAscii().data());

  vtkSMProxyProperty *prop = vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("LayoutStrategy"));
  prop->RemoveAllProxies();
  prop->AddProxy(layoutProxy);

  proxy->UpdateVTKObjects();
 
  Superclass::accept();    
}

void GraphLayoutFilterPanel::reset()
{
  Superclass::reset();
}

