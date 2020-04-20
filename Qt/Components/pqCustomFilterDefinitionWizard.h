/*=========================================================================

   Program: ParaView
   Module:    pqCustomFilterDefinitionWizard.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

/**
* \file pqCustomFilterDefinitionWizard.h
* \date 6/19/2006
*/

#ifndef _pqCustomFilterDefinitionWizard_h
#define _pqCustomFilterDefinitionWizard_h

#include "pqComponentsModule.h"
#include <QDialog>

class pqCustomFilterDefinitionModel;
class pqCustomFilterDefinitionWizardForm;
class pqCustomFilterManagerModel;
class pqOutputPort;
class QModelIndex;
class vtkSMCompoundSourceProxy;

/** \class pqCustomFilterDefinitionWizard
 *  \brief
 *    The pqCustomFilterDefinitionWizard class is used to create a
 *    compound proxy definition one step at a time.
 *
 *  The wizard should be created with a pqCustomFilterDefinitionModel.
 *  The model stores the sources that will be placed in the custom
 *  filter. The model is also used when selecting the exposed
 *  properties.
 *
 *  The following is an example of how to use the wizard:
 *
 *  \code
 *  pqCustomFilterDefinitionModel filter(this);
 *  filter.setContents(
 *    pqApplicationCore::instance()->getSelectionModel()->selectedItems());
 *  pqCustomFilterDefinitionWizard wizard(&filter, this);
 *  if(wizard.exec() == QDialog::Accepted)
 *    {
 *    wizard.createCustomFilter();
 *    }
 *  \endcode
 *
 *  The custom filter definition model is filled out using the
 *  selected pipeline sources. After setting the custom filter
 *  definition model's contents, you can check to see if any sources
 *  were added. The wizard can make a compound proxy without any
 *  sources, but you may not want to allow it. After the
 *  \c createCustomFilter call, you can get the name of the newly
 *  created compound proxy.
 */
class PQCOMPONENTS_EXPORT pqCustomFilterDefinitionWizard : public QDialog
{
  Q_OBJECT

public:
  /**
  * \brief
  *   Creates a custom filter definition wizard.
  * \param model The custom filter definition model to use. The
  *   model should not be null.
  * \param parent The parent widget for the wizard.
  */
  pqCustomFilterDefinitionWizard(pqCustomFilterDefinitionModel* model, QWidget* parent = 0);
  ~pqCustomFilterDefinitionWizard() override;

  /**
  * \brief
  *   Gets the custom filter definition model used by the wizard.
  * \return
  *   A pointer to the custom filter definition model.
  */
  pqCustomFilterDefinitionModel* getModel() const { return this->Model; }

  /**
  * \brief
  *   Gets the name of the compound proxy created by the wizard.
  * \return
  *   The name of the new compound proxy definition. The name will
  *   be empty if the compound proxy has not been created.
  * \sa pqCustomFilterDefinitionWizard::createCustomFilter();
  */
  QString getCustomFilterName() const;

public Q_SLOTS:
  /**
  * \brief
  *   Creates a compound proxy definition.
  *
  * The compound proxy definition is created using the custom filter
  * definition model and the parameters entered by the user. The
  * new definition is registered with the server manager.
  *
  * \sa pqCustomFilterDefinitionWizard::getCustomFilterName()
  */
  void createCustomFilter();

private:
  /**
  * \brief
  *   Adds proxies referred to by the proxies in the custom filter that
  *   the user could not have explicitly selected/deselected.
  *
  * A custom filter may includes proxies which refer to other internal
  * proxies such as implicit functions, internal sources which are not shown
  * in the pipeline browser. We include these proxies into the custom filter
  * so that its definition is complete. This method is called after all proxies
  * have been added to the custom filter and before its definition is
  * created.
  */
  void addAutoIncludedProxies();

  /**
  * \brief
  *   Validates the custom filter name field.
  *
  * This method will pop up message boxes for the user if there is
  * something wrong with the name entered.
  *
  * \return
  *   True if the custom filter name is valid.
  */
  bool validateCustomFilterName();

  /**
  * \brief
  *   Adds the default input and output to the forms.
  *
  * The current implementation only works for straight pipelines
  * with one input and no branches.
  */
  void setupDefaultInputOutput();

private Q_SLOTS:
  /**
  * \name Page Navigation
  */
  //@{
  /**
  * Called when the user clicks the back button.
  */
  void navigateBack();

  /**
  * Called when the user clicks the next button.
  */
  void navigateNext();

  /**
  * Called when the user clicks the finish button.
  */
  void finishWizard();

  /**
  * \brief
  *   Clears the custom filter overwrite flag.
  * \param text The changed name text.
  */
  void clearNameOverwrite(const QString& text);
  //@}

  /**
  * \name Model Selection Updates
  */
  //@{
  /**
  * \brief
  *   Updates the input form fields for the newly selected source.
  *
  * The input property combobox is updated to display the available
  * inputs for the newly selected source.
  *
  * \param current The currently selected index.
  * \param previous The previously selected index.
  */
  void updateInputForm(const QModelIndex& current, const QModelIndex& previous);

  /**
  * \brief
  *   Updates the output form fields for the newly selected source.
  * \param current The currently selected index.
  * \param previous The previously selected index.
  */
  void updateOutputForm(const QModelIndex& current, const QModelIndex& previous);

  /**
  * \brief
  *   Updates the property form fields for the newly selected source.
  *
  * The property combobox is updated to display the available
  * properties for the newly selected source. The input properties
  * are excluded since they are set on the input page.
  *
  * \param current The currently selected index.
  * \param previous The previously selected index.
  */
  void updatePropertyForm(const QModelIndex& current, const QModelIndex& previous);
  //@}

  /**
  * \name Input List Buttons
  */
  //@{
  /**
  * Adds an input to the list based on the form parameters.
  */
  void addInput();

  /**
  * Removes the selected input from the list.
  */
  void removeInput();

  /**
  * Moves the selected input up in the list.
  */
  void moveInputUp();

  /**
  * Moves the selected input down in the list.
  */
  void moveInputDown();
  //@}

  /**
  * \name Output List Buttons
  */
  //@{
  /**
  * Adds an output to the list based on the form parameters.
  */
  void addOutput();

  /**
  * Removes the selected output from the list.
  */
  void removeOutput();

  /**
  * Moves the selected output up in the list.
  */
  void moveOutputUp();

  /**
  * Moves the selected output down in the list.
  */
  void moveOutputDown();
  //@}

  /**
  * \name Property List Buttons
  */
  //@{
  /**
  * Adds an property to the list based on the form parameters.
  */
  void addProperty();

  /**
  * Removes the selected property from the list.
  */
  void removeProperty();

  /**
  * Moves the selected property up in the list.
  */
  void movePropertyUp();

  /**
  * Moves the selected property down in the list.
  */
  void movePropertyDown();
  //@}

  /**
  * \name List Selection Updates
  */
  //@{
  /**
  * \brief
  *   Updates the input list buttons for the newly selected index.
  * \param current The currently selected index.
  * \param previous The previously selected index.
  */
  void updateInputButtons(const QModelIndex& current, const QModelIndex& previous);

  /**
  * \brief
  *   Updates the output list buttons for the newly selected index.
  * \param current The currently selected index.
  * \param previous The previously selected index.
  */
  void updateOutputButtons(const QModelIndex& current, const QModelIndex& previous);

  /**
  * \brief
  *   Updates the property list buttons for the newly selected index.
  * \param current The currently selected index.
  * \param previous The previously selected index.
  */
  void updatePropertyButtons(const QModelIndex& current, const QModelIndex& previous);
  //@}

private:
  int CurrentPage;                          ///< Stores the current page.
  bool OverwriteOK;                         ///< Used with name validation.
  vtkSMCompoundSourceProxy* Filter;         ///< Stores the custom filter.
  pqCustomFilterDefinitionModel* Model;     ///< Stores the source hierarchy.
  pqCustomFilterDefinitionWizardForm* Form; ///< Defines the gui layout.

  /**
  * Internal method called by addOutput().
  */
  void addOutputInternal(pqOutputPort* port, const QString& string);
};

#endif
