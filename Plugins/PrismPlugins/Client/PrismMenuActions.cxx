

#include "PrismMenuActions.h"
#include "PrismCore.h"
#include "pqCoreUtilities.h"

PrismMenuActions::PrismMenuActions(QObject* p)
:QActionGroup(p)
    {

     this->setParent(pqCoreUtilities::mainWidget());
    PrismCore* core=PrismCore::instance();

    core->createActions(this);
/*
    QList<QAction*> actionsList;
    core->actions(actionsList);
    foreach(QAction *a,actionsList)
        {
        this->addAction(a);
        }
*/

    }

PrismMenuActions::~PrismMenuActions()
    {

    }

