/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCommandDispatcherManager.h"
#include "pqExplicitCommandDispatcher.h"
#include "pqImmediateCommandDispatcher.h"
#include "pqRefreshToolbar.h"
#include "pqTimeoutCommandDispatcher.h"

#include <QComboBox>
#include <QPushButton>

pqRefreshToolbar::pqRefreshToolbar(QWidget* Parent) :
  QToolBar("Refresh", Parent),
  RefreshButton(0)
{
  QComboBox* const combo = new QComboBox(this);
  combo->addItem(tr("Immediate"));
  combo->addItem(tr("Timeout"));
  combo->addItem(tr("Explicit"));
  this->addWidget(combo);

  this->RefreshButton = new QPushButton(tr("Refresh"), this);
  this->addWidget(this->RefreshButton);

  onRefreshType(0);
  QObject::connect(combo, SIGNAL(activated(int)), this, SLOT(onRefreshType(int)));
}

void pqRefreshToolbar::onRefreshType(int Index)
{
  switch(Index)
    {
    case 0:
      pqCommandDispatcherManager::Instance().SetDispatcher(new pqImmediateCommandDispatcher());
      this->RefreshButton->setDisabled(true);
      break;
      
    case 1:
      pqCommandDispatcherManager::Instance().SetDispatcher(new pqTimeoutCommandDispatcher(1000));
      this->RefreshButton->setDisabled(true);
      break;
      
    case 2:
      {
      pqExplicitCommandDispatcher* const dispatcher = new pqExplicitCommandDispatcher();
      pqCommandDispatcherManager::Instance().SetDispatcher(dispatcher);
      QObject::connect(dispatcher, SIGNAL(commandsPending(bool)), this->RefreshButton, SLOT(setEnabled(bool)));
      QObject::connect(this->RefreshButton, SIGNAL(clicked()), dispatcher, SLOT(onExecute()));
      this->RefreshButton->setDisabled(true);
      }
      break;
    }
}

