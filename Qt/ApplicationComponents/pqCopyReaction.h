// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCopyReaction_h
#define pqCopyReaction_h

#include "pqReaction.h"

#include <QMap>
#include <QPointer>
#include <QSet>

#include <iostream>

class pqProxy;
class vtkSMProxy;
/**
 * @ingroup Reactions
 * Reaction for copying sources/filters.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCopyReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  pqCopyReaction(QAction* parent, bool paste_mode = false, bool pipeline_mode = false);
  ~pqCopyReaction() override;

  /**
   * Copy all properties from source to dest. Uses the property names as the
   * key for matching properties.
   */
  static void copy(vtkSMProxy* dest, vtkSMProxy* source);

  static void copy();
  static void paste();
  static void copyPipeline();
  static void pastePipeline();
  static pqProxy* getSelectedPipelineRoot();
  static bool canPastePipeline();

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates the enabled state. Applications need not explicitly call this.
   */
  void updateEnableState() override;

Q_SIGNALS:
  void pipelineCopied();

protected:
  /**
   * Called when the action is triggered.
   */
  void onTriggered() override
  {
    if (this->Paste && !this->CreatePipeline)
    {
      pqCopyReaction::paste();
    }
    else if (this->Paste && this->CreatePipeline)
    {
      this->pastePipeline();
    }
    else if (!this->Paste && !this->CreatePipeline)
    {
      pqCopyReaction::copy();
    }
    else
    {
      this->copyPipeline();
      Q_EMIT this->pipelineCopied();
    }
  }
  bool Paste;
  bool CreatePipeline;

  ///@{
  /**
   * Used for copy/paste pipeline. pqApplicationCore::registerManager() requires
   * a QObject, but we need to track potentially multiple pipeline sources.
   * Also using QPointer, so that way if any pqPipelineSource in this selection
   * is deleted between Copy Pipeline and Paste Pipeline, we will be able to easily
   * determine this.
   */
  static QSet<QPointer<pqProxy>> FilterSelection;
  static pqProxy* SelectionRoot;
  ///@}

  ///@{
  /**
   * Containers storing all instances of pipeline paster / copier.
   * They enable to trigger `updateEnableState` of pasters once a pipeline
   * has been copied in the clipboard.
   */
  static QSet<pqCopyReaction*> PastePipelineContainer;
  static QSet<pqCopyReaction*> CopyPipelineContainer;
  static QMap<pqProxy*, QMetaObject::Connection> SelectedProxyConnections;
  ///@}

private:
  Q_DISABLE_COPY(pqCopyReaction)
};

#endif
