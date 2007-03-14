/*=========================================================================

   Program: ParaView
   Module:    pqLookmarkDefinitionWizard.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#ifndef _pqLookmarkDefinitionWizard_h
#define _pqLookmarkDefinitionWizard_h


#include "pqComponentsExport.h"
#include <QDialog>

class pqLookmarkDefinitionWizardForm;
class QModelIndex;
class pqGenericViewModule;
class vtkCollection;
class pqConsumerDisplay;
class pqFlatTreeView;
class pqPipelineModel;
class vtkPVXMLElement;
class pqLookmarkManagerModel;

/*! \class pqLookmarkDefinitionWizard
 *  \brief
 *    The pqLookmarkDefinitionWizard class is used to create a lookmark model.
 *
 *  Currently you cannot create a lookmark of multiple views. 
 * 
 *  A lookmark needs three things to be created: a unique name, an icon (of the current view), and an XML representation of a subset of the server manager state.
 *  The user may also add a description of the lookmark. 
 *
 *  A subset of the server manager state is saved on creation:
 *   - all pqConsumerDisplays that are visible in the given pqRenderViewModule
 *   - all pqConsumerDisplays that are invisible in the given pqRenderViewModule but "upstream" from a visible one in the pipeline 
 *   - all pqPipelineSources associated with the displays being saved
 *   - all referred proxies of all saved display and source proxies. A referred proxy is one that is part of a proxy property of a saved proxy.
 *   - the vtkSMRenderModuleProxy state (but not its referred proxies)
 *
 */
class PQCOMPONENTS_EXPORT pqLookmarkDefinitionWizard : public QDialog
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a lookmark definition wizard.
  /// \param model The view module to create a lookmark from (currently only supports render views)
  /// \param parent The parent widget for the wizard.
  pqLookmarkDefinitionWizard(pqLookmarkManagerModel *model, pqGenericViewModule *view, QWidget *parent=0);
  virtual ~pqLookmarkDefinitionWizard();

public slots:
  /// \brief
  ///   Creates a lookmark definition.
  ///
  /// A lookmark is created with the name provided by the user, an icon image of the current view, 
  /// and the state of the displays and sources that make up the render module.
  void createLookmark();

private:

  /// \brief
  ///   Validates the lookmark name field.
  ///
  /// This method will pop up message boxes for the user if there is
  /// something wrong with the name entered.
  ///
  /// \return
  ///   True if the lookmark name is valid.
  bool validateLookmarkName();

  /// \brief
  ///   Generates a "trimmed" view of the current pipeline that reflects the pipeline that will be restored when this lookmark is invoked.
  /// \return
  ///   The view of the pipeline.
  void createPipelinePreview();

  /// \brief
  ///   A recursive helper function for adding to the proxy collection the given display, its input source, 
  ///   and all displays and sources "upstream" in the pipeline from it.
  ///
  /// The collection of proxies is given to the server manager and their XML representation is generated
  void addToProxyCollection(pqConsumerDisplay *src, vtkCollection *proxies);

private slots:

  /// Called when the user clicks the finish button.
  void finishWizard();

  /// \brief
  ///   Clears the lookmark overwite flag.
  /// \param text The changed name text.
  void clearNameOverwrite(const QString &text);
  //@}

  void addChildItems(const QModelIndex &index, vtkPVXMLElement *elem);

private:
  bool OverwriteOK;                         ///< Used with name validation.
  pqGenericViewModule *ViewModule;
  pqLookmarkDefinitionWizardForm *Form;
  pqFlatTreeView *PipelineView;
  pqPipelineModel *PipelineModel;
  vtkPVXMLElement *PipelineHierarchy;
  pqLookmarkManagerModel *Model;
};

#endif
