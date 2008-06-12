/*=========================================================================

   Program: ParaView
   Module:    PrismDisplayPanelsImplementation.h

=========================================================================*/
#ifndef _PrismDisplayPanelsImplementation_h
#define _PrismDisplayPanelsImplementation_h

#include <QWidget>
#include <QVariant>
#include "pqDisplayPanelInterface.h"
#include "pqRepresentation.h"
#include "vtkSMProxy.h"
#include "pqDisplayProxyEditor.h"
#include <QObject>

/// standard display panels
class PrismDisplayPanelsImplementation :public QObject,
                                        public pqDisplayPanelInterface
{
  Q_OBJECT
  Q_INTERFACES(pqDisplayPanelInterface)
  public:
    /// constructor
    PrismDisplayPanelsImplementation(){}
    PrismDisplayPanelsImplementation(QObject* p);
    /// destructor
    virtual ~PrismDisplayPanelsImplementation(){}

    /// Returns true if this panel can be created for the given the proxy.
    virtual bool canCreatePanel(pqRepresentation* proxy) const;
    /// Creates a panel for the given proxy
    virtual pqDisplayPanel* createPanel(pqRepresentation* proxy, QWidget* p);
};
#endif

