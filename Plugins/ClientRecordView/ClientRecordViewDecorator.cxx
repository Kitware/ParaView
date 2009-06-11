#include "ClientRecordViewDecorator.h"
#include "ClientRecordView.h"
#include "ui_ClientRecordViewDecorator.h"

#include "pqApplicationCore.h"
#include "pqComboBoxDomain.h"
#include "pqDataRepresentation.h"
#include "pqDisplayPolicy.h"
#include "pqMultiViewFrame.h"
#include "pqNamedWidgets.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqPropertyManager.h"
#include "pqSignalAdaptors.h"
#include "pqView.h"

#include "vtkSMProxy.h"
#include "vtkSMProperty.h"

#include <QAction>
#include <QVBoxLayout>
#include <QWidget>
#include <QComboBox>


class ClientRecordViewDecorator::pqInternal : public Ui::ClientRecordViewDecorator
{
public:
  pqPropertyLinks Links;
  pqPropertyManager PropertyManager;
  pqSignalAdaptorComboBox* AttributeAdaptor;
  pqComboBoxDomain* AttributeDomain;
};

//-----------------------------------------------------------------------------
ClientRecordViewDecorator::ClientRecordViewDecorator(ClientRecordView* view):
  Superclass(view)
{
  this->View = view;
  QWidget* container = view->getWidget();

  QWidget* header = new QWidget(container);
  QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(container->layout());
  
  this->Internal = new pqInternal();
  this->Internal->setupUi(header);
  //this->Internal->Source->setAutoUpdateIndex(false);
  //this->Internal->Source->addCustomEntry("None", NULL);
  //this->Internal->Source->fillExistingPorts();
  this->Internal->AttributeAdaptor = 0;
  this->Internal->AttributeDomain = 0;
  this->Internal->Source->hide();
  this->Internal->label->hide();

  QObject::connect(&this->Internal->Links, SIGNAL(smPropertyChanged()),
    this->View, SLOT(render()));

  //QObject::connect(this->Internal->Source, SIGNAL(currentIndexChanged(pqOutputPort*)),
  //  this, SLOT(currentIndexChanged(pqOutputPort*)));
  QObject::connect(this->View, SIGNAL(showing(pqDataRepresentation*)),
    this, SLOT(showing(pqDataRepresentation*)));

  layout->insertWidget(0, header);
  this->showing(0); //TODO: get the actual repr currently shown by the view.
}

//-----------------------------------------------------------------------------
ClientRecordViewDecorator::~ClientRecordViewDecorator()
{
  delete this->Internal;
  this->Internal = 0;
}

//-----------------------------------------------------------------------------
void ClientRecordViewDecorator::showing(pqDataRepresentation* repr)
{
  this->Internal->Links.removeAllPropertyLinks();
  delete this->Internal->AttributeDomain;
  delete this->Internal->AttributeAdaptor;
  this->Internal->AttributeDomain = 0;
  this->Internal->AttributeAdaptor = 0;
  if (repr)
    {
    vtkSMProxy* reprProxy = repr->getProxy();
    //this->Internal->Source->setCurrentPort(repr->getOutputPortFromInput());

    this->Internal->AttributeDomain = new pqComboBoxDomain(
      this->Internal->attribute_mode,
      reprProxy->GetProperty("AttributeType"), "field_list");
    this->Internal->AttributeDomain->setObjectName("FieldModeDomain");
    this->Internal->AttributeAdaptor = 
      new pqSignalAdaptorComboBox(this->Internal->attribute_mode);
    this->Internal->PropertyManager.registerLink(
      this->Internal->AttributeAdaptor, "currentText", 
      SIGNAL(currentTextChanged(const QString&)),
      reprProxy, reprProxy->GetProperty("AttributeType"), 0);  
    this->Internal->Links.addPropertyLink(this->Internal->AttributeAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      reprProxy, reprProxy->GetProperty("AttributeType"),0);
    this->Internal->Links.addPropertyLink(this->Internal->FreezeContents,
      "checked", SIGNAL(toggled(bool)),
      reprProxy, reprProxy->GetProperty("FreezeContents"));

    pqPipelineSource* input = repr->getInput();
    QObject::connect(input, SIGNAL(dataUpdated(pqPipelineSource*)), 
      this, SLOT(dataUpdated()));
    this->dataUpdated();
    }
  else
    {
    //this->Internal->Source->setCurrentPort(NULL);
    }

  //this->Internal->attribute_mode->setEnabled(repr != 0);
}

//-----------------------------------------------------------------------------
void ClientRecordViewDecorator::currentIndexChanged(pqOutputPort* port)
{
  if (port)
    {
    pqDisplayPolicy* dpolicy = 
      pqApplicationCore::instance()->getDisplayPolicy();
    // Will create new display if needed. May also create new view 
    // as defined by the policy.
    if (dpolicy->setRepresentationVisibility(port, this->View, true))
      {
      this->View->render();
      }
    }
  else
    {
    QList<pqRepresentation*> reprs = this->View->getRepresentations();
    foreach (pqRepresentation* repr, reprs)
      {
      if (repr->isVisible())
        {
        repr->setVisible(false);
        this->View->render();
        break; // since only 1 repr can be visible at a time.
        }
      }
    }
}

void ClientRecordViewDecorator::dataUpdated()
{
  QList<pqRepresentation*> reprs = this->View->getRepresentations();
  foreach (pqRepresentation* repr, reprs)
    {
    if (repr->isVisible())
      {
      vtkSMProxy* proxy = repr->getProxy();
      vtkSMProperty* prop = proxy->GetProperty("Input");
      if (prop)
        {
        prop->UpdateDependentDomains();
        }
      break; // since only 1 repr can be visible at a time.
      }
    }

}
