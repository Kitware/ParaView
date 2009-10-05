

#include "PrismMenuActions.h"
#include "PrismCore.h"

PrismMenuActions::PrismMenuActions(QObject* p)
: QActionGroup(p)
    {
    PrismCore* core=PrismCore::instance();


    QList<QAction*> actionsList;
    core->actions(actionsList);
    foreach(QAction *a,actionsList)
        {
        this->addAction(a);
        }

    }

PrismMenuActions::~PrismMenuActions()
    {

    }

