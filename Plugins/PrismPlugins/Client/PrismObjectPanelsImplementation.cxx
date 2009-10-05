/*=========================================================================

   Program: ParaView
   Module:    PrismObjectPanelsImplementation.cxx

=========================================================================*/

// this include
#include "PrismObjectPanelsImplementation.h"
#include "pqDataRepresentation.h"
#include "pqPipelineSource.h"
#include "PrismSurfacePanel.h"
#include "PrismPanel.h"


PrismObjectPanelsImplementation::PrismObjectPanelsImplementation(QObject* p):
 QObject(p)
{
}

bool PrismObjectPanelsImplementation::canCreatePanel(pqProxy* proxy ) const
    {
    if(!proxy)
      {
      return false;
      }

    QString name=proxy->getProxy()->GetXMLName();
     if(name=="PrismSurfaceReader")
     {
       return true;
     }
     if(name=="PrismFilter")
     {
       return true;
     }
    return false;
   }
  /// Creates a panel for the given proxy
pqObjectPanel* PrismObjectPanelsImplementation::createPanel(pqProxy* proxy, QWidget* p)
    {
    if(!proxy)
      {
      return false;
      }


     QString name=proxy->getProxy()->GetXMLName();
     if(name=="PrismSurfaceReader")
     {

       return new PrismSurfacePanel(proxy,p);

     }
     if(name=="PrismFilter")
     {

       return new PrismPanel(proxy,p);

     }

    return NULL;
    }


