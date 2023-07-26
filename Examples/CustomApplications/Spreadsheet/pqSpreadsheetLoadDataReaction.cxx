// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqSpreadsheetLoadDataReaction.h"

#include <QSet>

//-----------------------------------------------------------------------------
pqSpreadsheetLoadDataReaction::pqSpreadsheetLoadDataReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
void pqSpreadsheetLoadDataReaction::onTriggered()
{
  ReaderSet readerSet{ { "sources", "XMLPolyDataReader" }, { "sources", "LegacyVTKFileReader" },
    { "sources", "PVDReader" } };
  QList<pqPipelineSource*> sources = pqLoadDataReaction::loadData(readerSet, false);

  Q_FOREACH (pqPipelineSource* source, sources)
  {
    Q_EMIT this->loadedData(source);
  }
}
