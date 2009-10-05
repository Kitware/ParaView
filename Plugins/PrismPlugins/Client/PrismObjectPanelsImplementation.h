/*=========================================================================

   Program: ParaView
   Module:    PrismObjectPanelsImplementation.h

=========================================================================*/
#ifndef _PrismObjectPanelsImplementation_h
#define _PrismObjectPanelsImplementation_h

#include <QWidget>
#include <QVariant>
#include "pqObjectPanelInterface.h"
#include "vtkSMProxy.h"
#include <QObject>
class pqProxy;

/// standard display panels
class PrismObjectPanelsImplementation :public QObject,
                                        public pqObjectPanelInterface
{
  Q_OBJECT
  Q_INTERFACES(pqObjectPanelInterface)
  public:
    /// constructor
    PrismObjectPanelsImplementation(){}
    PrismObjectPanelsImplementation(QObject* p);
    /// destructor
    virtual ~PrismObjectPanelsImplementation(){}

    /// Returns true if this panel can be created for the given the proxy.
    virtual bool canCreatePanel(pqProxy* proxy) const;
    /// Creates a panel for the given proxy
    virtual pqObjectPanel* createPanel(pqProxy* proxy, QWidget* p);
};
#endif

