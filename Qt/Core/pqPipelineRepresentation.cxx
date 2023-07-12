// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPipelineRepresentation.h"
#include "pqView.h"

#include "vtkSMColorMapEditorHelper.h"
#include "vtkSMRepresentationProxy.h"

//-----------------------------------------------------------------------------
pqPipelineRepresentation::pqPipelineRepresentation(const QString& group, const QString& name,
  vtkSMProxy* display, pqServer* server, QObject* p /*=null*/)
  : Superclass(group, name, display, server, p)
{
}

//-----------------------------------------------------------------------------
pqPipelineRepresentation::~pqPipelineRepresentation() = default;

//-----------------------------------------------------------------------------
vtkSMRepresentationProxy* pqPipelineRepresentation::getRepresentationProxy() const
{
  return vtkSMRepresentationProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::setView(pqView* view)
{
  pqView* oldView = this->getView();
  this->Superclass::setView(view);

  if (view)
  {
    this->connect(view, SIGNAL(updateDataEvent()), this, SLOT(updateLookupTable()));
  }

  if (oldView && oldView != view)
  {
    this->disconnect(oldView, SIGNAL(updateDataEvent()), this, SLOT(updateLookupTable()));
  }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::resetLookupTableScalarRange()
{
  vtkSMProxy* proxy = this->getProxy();
  if (vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy))
  {
    vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRange(proxy);
  }
}

//-----------------------------------------------------------------------------
void pqPipelineRepresentation::resetLookupTableScalarRangeOverTime()
{
  vtkSMProxy* proxy = this->getProxy();
  if (vtkSMColorMapEditorHelper::GetUsingScalarColoring(proxy))
  {
    vtkSMColorMapEditorHelper::RescaleTransferFunctionToDataRangeOverTime(proxy);
  }
}
