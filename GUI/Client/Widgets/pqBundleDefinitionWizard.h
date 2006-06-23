/*=========================================================================

   Program: ParaView
   Module:    pqBundleDefinitionWizard.h

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

/// \file pqBundleDefinitionWizard.h
/// \date 6/19/2006

#ifndef _pqBundleDefinitionWizard_h
#define _pqBundleDefinitionWizard_h


#include "pqWidgetsExport.h"
#include <QDialog>

class pqBundleDefinitionModel;
class pqBundleDefinitionWizardForm;
class QModelIndex;
class vtkSMCompoundProxy;


/*! \class pqBundleDefinitionWizard
 *  \brief
 *    The pqBundleDefinitionWizard class is used to create a compound
 *    proxy definition one step at a time.
 * 
 *  The wizard should be created with a pqBundleDefinitionModel. The
 *  model stores the sources that will be placed in the bundle. The
 *  model is also used when selecting the exposed properties.
 * 
 *  The following is an example of how to use the wizard:
 * 
 *  \code
 *  pqBundleDefinitionModel bundle(this);
 *  bundle.setContents(
 *    pqApplicationCore::instance()->getSelectionModel()->selectedItems());
 *  pqBundleDefinitionWizard wizard(&bundle, this);
 *  if(wizard.exec() == QDialog::Accepted)
 *    {
 *    wizard.createPipelineBundle();
 *    }
 *  \endcode
 *
 *  The bundle definition model is filled out using the selected
 *  pipeline sources. After setting the bundle definition model's
 *  contents, you can check to see if any sources were added. The
 *  wizard can make a compound proxy without any sources, but you
 *  may not want to allow it. After the \c createPipelineBundle call,
 *  you can get the name of the newly created compound proxy.
 */
class PQWIDGETS_EXPORT pqBundleDefinitionWizard : public QDialog
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a bundle definition wizard.
  /// \param model The bundle definition model to use. The model should
  ///   not be null.
  /// \param parent The parent widget for the wizard.
  pqBundleDefinitionWizard(pqBundleDefinitionModel *model, QWidget *parent=0);
  virtual ~pqBundleDefinitionWizard();

  /// \brief
  ///   Gets the bundle definition model used by the wizard.
  /// \return
  ///   A pointer to the bundle definition model.
  pqBundleDefinitionModel *getModel() const {return this->Model;}

  /// \brief
  ///   Gets the name of the compound proxy created by the wizard.
  /// \return
  ///   The name of the new compound proxy definition. The name will
  ///   be empty if the compound proxy has not been created.
  /// \sa pqBundleDefinitionWizard::createPipelineBundle();
  QString getBundleName() const;

public slots:
  /// \brief
  ///   Creates a compound proxy definition.
  ///
  /// The compound proxy definition is created using the bundle
  /// definition model and the parameters entered by the user. The
  /// new definition is registered with the server manager.
  ///
  /// \sa pqBundleDefinitionWizard::getBundleName()
  void createPipelineBundle();

private slots:
  /// \name Page Navigation
  //@{
  /// Called when the user clicks the back button.
  void navigateBack();

  /// Called when the user clicks the next/finish button.
  void navigateNext();
  //@}

  /// \name Model Selection Updates
  //@{
  /// \brief
  ///   Updates the input form fields for the newly selected source.
  ///
  /// The input property combobox is updated to display the available
  /// inputs for the newly selected source.
  ///
  /// \param current The currently selected index.
  /// \param previous The previously selected index.
  void updateInputForm(const QModelIndex &current,
      const QModelIndex &previous);

  /// \brief
  ///   Updates the output form fields for the newly selected source.
  /// \param current The currently selected index.
  /// \param previous The previously selected index.
  void updateOutputForm(const QModelIndex &current,
      const QModelIndex &previous);

  /// \brief
  ///   Updates the property form fields for the newly selected source.
  ///
  /// The property combobox is updated to display the available
  /// properties for the newly selected source. The input properties
  /// are excluded since they are set on the input page.
  ///
  /// \param current The currently selected index.
  /// \param previous The previously selected index.
  void updatePropertyForm(const QModelIndex &current,
      const QModelIndex &previous);
  //@}

  /// \name Input List Buttons
  //@{
  /// Adds an input to the list based on the form parameters.
  void addInput();

  /// Removes the selected input from the list.
  void removeInput();

  /// Moves the selected input up in the list.
  void moveInputUp();

  /// Moves the selected input down in the list.
  void moveInputDown();
  //@}

  /// \name Output List Buttons
  //@{
  /// Adds an output to the list based on the form parameters.
  void addOutput();

  /// Removes the selected output from the list.
  void removeOutput();

  /// Moves the selected output up in the list.
  void moveOutputUp();

  /// Moves the selected output down in the list.
  void moveOutputDown();
  //@}

  /// \name Property List Buttons
  //@{
  /// Adds an property to the list based on the form parameters.
  void addProperty();

  /// Removes the selected property from the list.
  void removeProperty();

  /// Moves the selected property up in the list.
  void movePropertyUp();

  /// Moves the selected property down in the list.
  void movePropertyDown();
  //@}

  /// \name List Selection Updates
  //@{
  /// \brief
  ///   Updates the input list buttons for the newly selected index.
  /// \param current The currently selected index.
  /// \param previous The previously selected index.
  void updateInputButtons(const QModelIndex &current,
      const QModelIndex &previous);

  /// \brief
  ///   Updates the output list buttons for the newly selected index.
  /// \param current The currently selected index.
  /// \param previous The previously selected index.
  void updateOutputButtons(const QModelIndex &current,
      const QModelIndex &previous);

  /// \brief
  ///   Updates the property list buttons for the newly selected index.
  /// \param current The currently selected index.
  /// \param previous The previously selected index.
  void updatePropertyButtons(const QModelIndex &current,
      const QModelIndex &previous);
  //@}

private:
  int CurrentPage;                    ///< Stores the current page.
  vtkSMCompoundProxy *Bundle;         ///< Stores the bundle definition.
  pqBundleDefinitionModel *Model;     ///< Stores the source hierarchy.
  pqBundleDefinitionWizardForm *Form; ///< Defines the gui layout.
};

#endif
