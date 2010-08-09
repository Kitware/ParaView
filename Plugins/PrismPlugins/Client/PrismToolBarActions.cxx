

#include "PrismToolBarActions.h"
#include "PrismCore.h"
#include "pqCoreUtilities.h"

PrismToolBarActions::PrismToolBarActions(QObject* p)
: QActionGroup(p)
    {
     this->setParent(pqCoreUtilities::mainWidget());
    PrismCore* core=PrismCore::instance();

    core->createActions(this);


  /*     PrismCore* core=PrismCore::instance();
 QAction* PrismViewAction = new QAction("Prism View",this);
    PrismViewAction->setToolTip("Create Prism View");
    PrismViewAction->setIcon(QIcon(":/Prism/Icons/PrismSmall.png"));

    QObject::connect(PrismViewAction, SIGNAL(triggered(bool)), core, SLOT(onCreatePrismView()));

    QAction* SesameViewAction = new QAction("SESAME Surface",this);
    SesameViewAction->setToolTip("Open SESAME Surface");
    SesameViewAction->setIcon(QIcon(":/Prism/Icons/CreateSESAME.png"));

    QObject::connect(SesameViewAction, SIGNAL(triggered(bool)), core, SLOT(onSESAMEFileOpen()));*/



    }

PrismToolBarActions::~PrismToolBarActions()
    {

    }

