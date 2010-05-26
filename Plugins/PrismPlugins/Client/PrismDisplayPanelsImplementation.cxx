/*=========================================================================

Program: ParaView
Module:    PrismDisplayPanelsImplementation.cxx

=========================================================================*/

// this include
#include "PrismDisplayPanelsImplementation.h"
#include "pqDataRepresentation.h"
#include "PrismDisplayProxyEditor.h"
#include "pqPipelineSource.h"
#include "PrismSurfacePanel.h"


PrismDisplayPanelsImplementation::PrismDisplayPanelsImplementation(QObject* p):
QObject(p)
    {
    }

bool PrismDisplayPanelsImplementation::canCreatePanel(pqRepresentation* repr) const
{

 
    if(!repr || !repr->getProxy())
    {
        return false;
    }

    pqDataRepresentation* dataRepr = qobject_cast<pqDataRepresentation*>(repr);
    if(dataRepr)
    {
        pqPipelineSource* input = dataRepr->getInput(); 
        QString name=input->getProxy()->GetXMLName();
        if(name=="PrismFilter"|| name=="PrismSurfaceReader")
        {
            return true;//this needs to be changed back to true when the cube axis filter is fixed.
        }

    }

    return false;
    }
/// Creates a panel for the given proxy
pqDisplayPanel* PrismDisplayPanelsImplementation::createPanel(pqRepresentation* repr, QWidget* p)
    {
    if(!repr || !repr->getProxy())
        {
        return NULL;
        }


    pqDataRepresentation* dataRepr = qobject_cast<pqDataRepresentation*>(repr);
    if(dataRepr)
        {
        pqPipelineSource* input = dataRepr->getInput(); 
        QString name=input->getProxy()->GetXMLName();
        if(name=="PrismFilter" || name=="PrismSurfaceReader")
            {

              pqPipelineRepresentation* pd = qobject_cast<pqPipelineRepresentation*>(repr);

              if(pd)
              {
                return new PrismDisplayProxyEditor(pd,p);
              }

        }

        }
    return NULL;
    }


