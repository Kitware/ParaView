

#include "PrismMenuActions.h"
#include "PrismCore.h"
#include "pqCoreUtilities.h"

PrismMenuActions::PrismMenuActions(QObject* p)
:QActionGroup(p)
    {
    this->setParent(pqCoreUtilities::mainWidget());
    PrismCore* core=PrismCore::instance();

    QAction *prismView = new QAction(this);
    QAction *sesameFilter = new QAction(this);
    QAction *scaleView = new QAction(this);
    core->registerActions(prismView,sesameFilter,scaleView);
    }

PrismMenuActions::~PrismMenuActions()
    {

    }

