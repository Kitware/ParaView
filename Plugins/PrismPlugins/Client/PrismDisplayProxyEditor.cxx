/*=========================================================================

Program: ParaView
Module:    PrismDisplayProxyEditor.cxx

=========================================================================*/

// this include
#include "PrismDisplayProxyEditor.h"
#include "ui_pqDisplayProxyEditor.h"

// Qt includes
#include <QFileInfo>
#include <QIcon>
#include <QIntValidator>
#include <QKeyEvent>
#include <QMetaType>
#include <QPointer>
#include <QtDebug>
#include <QTimer>

// ParaView Server Manager includes
#include "vtkEventQtSlotConnect.h"
#include "vtkLabeledDataMapper.h"
#include "vtkMaterialLibrary.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMLookupTableProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMCompositeTreeDomain.h"
#include "vtkClientServerStream.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMProxyManager.h"

// ParaView widget includes
#include "pqSignalAdaptors.h"

// ParaView client includes
#include "pqDisplayProxyEditor.h"
#include "pqApplicationCore.h"
#include "pqColorScaleEditor.h"
#include "pqCubeAxesEditorDialog.h"
#include "pqFileDialog.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqScalarsToColors.h"
#include "pqSMAdaptor.h"
#include "pqWidgetRangeDomain.h"
#include "pqOutputPort.h"
#include "pqSignalAdaptorCompositeTreeWidget.h"
#include "pqObjectBuilder.h"
#include "pqServerManagerSelectionModel.h"
#include "pqServer.h"
#include "pqView.h"


//-----------------------------------------------------------------------------
/// constructor
PrismDisplayProxyEditor::PrismDisplayProxyEditor(pqPipelineRepresentation* repr, QWidget* p)
: pqDisplayProxyEditor(repr, p)
    {
    this->CubeAxesActor = NULL;
    this->Representation= repr;

    pqApplicationCore* core = pqApplicationCore::instance();
    pqObjectBuilder* builder = core->getObjectBuilder();
    pqServer* server = this->getActiveServer();
    if(!server)
        {
        qCritical() << "Cannot create reader without an active server.";
        return ;
        }

    this->CubeAxesActor=vtkSMPrismCubeAxesRepresentationProxy::SafeDownCast(builder->createProxy("props", 
        "PrismCubeAxesRepresentation", server, 
        "props"));


    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
        this->CubeAxesActor->GetProperty("Input"));
    vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);
    if (!pp)
        {
        vtkErrorWithObjectMacro(this->CubeAxesActor,"Failed to locate property " << "Input"
            << " on the consumer " <<  (this->CubeAxesActor->GetXMLName()));
        return;
        }

    if (ip)
        {
        ip->RemoveAllProxies();
        ip->AddInputConnection(repr->getInput()->getProxy(),repr->getOutputPortFromInput()->getPortNumber() );
        }
    else
        {
        pp->RemoveAllProxies();
        pp->AddProxy(repr->getInput()->getProxy());
        }
    this->CubeAxesActor->UpdateProperty("Input");


    pqRenderView * view= qobject_cast<pqRenderView*>(this->Representation->getView());
    if(view)
        {
        vtkSMViewProxy* renv= view->getViewProxy();
        renv->AddRepresentation(this->CubeAxesActor);
        }
    }

//-----------------------------------------------------------------------------
/// destructor
PrismDisplayProxyEditor::~PrismDisplayProxyEditor()
    {

    if (this->CubeAxesActor)
        {

        pqRenderView * view= qobject_cast<pqRenderView*>(this->Representation->getView());
        if(view)
            {
            vtkSMViewProxy* renv= view->getViewProxy();
            renv->RemoveRepresentation(this->CubeAxesActor);
            view->getProxy()->UpdateVTKObjects();
            }



        vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
        pxm->UnRegisterProxy(this->CubeAxesActor->GetXMLGroup(),this->CubeAxesActor->GetClassName(),this->CubeAxesActor);
        }




    }

void PrismDisplayProxyEditor::cubeAxesVisibilityChanged()
{
   this->CubeAxesActor->SetCubeAxesVisibility(this->isCubeAxesVisible());
    this->Representation->renderViewEventually();

}
//-----------------------------------------------------------------------------
void PrismDisplayProxyEditor::editCubeAxes()
{
  pqCubeAxesEditorDialog dialog(this);
  dialog.setRepresentationProxy(this->CubeAxesActor);
  dialog.exec();
}

pqServer* PrismDisplayProxyEditor::getActiveServer() const
    {
    pqApplicationCore* core = pqApplicationCore::instance();
    pqServer* server=core->getActiveServer();

    /* pqServerManagerSelection sels = *core->getSelectionModel()->selectedItems();
    pqPipelineSource* source = 0;
    pqServer* server=0;
    pqOutputPort* outputPort=0;
    pqServerManagerModelItem* item = 0;
    pqServerManagerSelection::ConstIterator iter = sels.begin();

    item = *iter;
    source = dynamic_cast<pqPipelineSource*>(item);   

    if(source)
    {
    server = source->getServer();
    }
    else
    {
    outputPort=dynamic_cast<pqOutputPort*>(item); 
    if(outputPort)
    {
    server= outputPort->getServer();
    }
    else
    {
    server = dynamic_cast<pqServer*>(item);  
    }
    }
    */
    return server;
    }


