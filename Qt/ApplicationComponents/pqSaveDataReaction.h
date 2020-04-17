/*=========================================================================

   Program: ParaView
   Module:    pqSaveDataReaction.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef pqSaveDataReaction_h
#define pqSaveDataReaction_h

#include "pqReaction.h"

class pqPipelineSource;
class vtkPVDataInformation;

/**
* @ingroup Reactions
* Reaction to save data files.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqSaveDataReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
  * Constructor. Parent cannot be NULL.
  */
  pqSaveDataReaction(QAction* parent);

  /**
  * Save data files from active port. Users the vtkSMWriterFactory to decide
  * what writes are available. Returns true if the creation is
  * successful, otherwise returns false.
  * Note that this method is static. Applications can simply use this without
  * having to create a reaction instance.
  */
  static bool saveActiveData(const QString& files);
  static bool saveActiveData();

public Q_SLOTS:
  /**
  * Updates the enabled state. Applications need not explicitly call
  * this.
  */
  void updateEnableState() override;
  /**
  * Triggered when a source became valid
  */
  void dataUpdated(pqPipelineSource* source);

protected:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override { pqSaveDataReaction::saveActiveData(); }

private:
  Q_DISABLE_COPY(pqSaveDataReaction)

  static QString defaultExtension(vtkPVDataInformation* info);
  static void setDefaultExtension(vtkPVDataInformation* info, const QString& ext);
};

#endif
