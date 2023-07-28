// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * \file pqPipelineRepresentation.h
 * \date 4/24/2006
 */

#ifndef pqPipelineRepresentation_h
#define pqPipelineRepresentation_h

#include "pqDataRepresentation.h"
#include <QPair>

class pqPipelineSource;
class pqRenderViewModule;
class pqServer;
class vtkPVArrayInformation;
class vtkPVDataSetAttributesInformation;
class vtkPVDataSetAttributesInformation;
class vtkSMRepresentationProxy;

/**
 * This is PQ representation for a single display. A pqRepresentation represents
 * a single vtkSMRepresentationProxy. The display can be added to
 * only one render module or more (ofcouse on the same server, this class
 * doesn't worry about that.
 */
class PQCORE_EXPORT pqPipelineRepresentation : public pqDataRepresentation
{
  Q_OBJECT
  typedef pqDataRepresentation Superclass;

public:
  // Constructor.
  // \c group :- smgroup in which the proxy has been registered.
  // \c name  :- smname as which the proxy has been registered.
  // \c repr  :- the representation proxy.
  // \c server:- server on which the proxy is created.
  // \c parent:- QObject parent.
  pqPipelineRepresentation(const QString& group, const QString& name, vtkSMProxy* repr,
    pqServer* server, QObject* parent = nullptr);
  ~pqPipelineRepresentation() override;

  // Get the internal display proxy.
  vtkSMRepresentationProxy* getRepresentationProxy() const;

protected:
  // Overridden to set up some additional Qt connections
  void setView(pqView* view) override;

public Q_SLOTS:
  // If lookuptable is set up and is used for coloring,
  // then calling this method resets the table ranges to match the current
  // range of the selected array.
  void resetLookupTableScalarRange();

  // If lookuptable is set up and is used for coloring,
  // then calling this method resets the table ranges to match the
  // range of the selected array over time. This can potentially be a slow
  // processes hence use with caution!!!
  void resetLookupTableScalarRangeOverTime();
};

#endif
