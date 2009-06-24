

#include "PrismMenuActions.h"
#include "PrismCore.h"

PrismMenuActions::PrismMenuActions(QObject* p)
: QActionGroup(p)
    {
    PrismCore* core=PrismCore::instance();
    if(!core)
        {
        core=new PrismCore(this);
        }

    QList<QAction*> actionsList= core->actions();
    foreach(QAction *a,actionsList)
        {
        this->addAction(a);
        }

    }

PrismMenuActions::~PrismMenuActions()
    {

    }

