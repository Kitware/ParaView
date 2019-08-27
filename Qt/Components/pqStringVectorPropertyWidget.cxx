/*=========================================================================

   Program: ParaView
   Module: pqStringVectorPropertyWidget.cxx

   Copyright (c) 2005-2012 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#include "pqStringVectorPropertyWidget.h"

#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMFileListDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMSILDomain.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMSubsetInclusionLatticeDomain.h"

#include "pqApplicationCore.h"
#include "pqArrayListDomain.h"
#include "pqArraySelectionWidget.h"
#include "pqArraySelectorPropertyWidget.h"
#include "pqComboBoxDomain.h"
#include "pqDialog.h"
#include "pqFileChooserWidget.h"
#include "pqLineEdit.h"
#include "pqPopOutWidget.h"
#include "pqProxySILModel.h"
#include "pqSILModel.h"
#include "pqSILWidget.h"
#include "pqSMAdaptor.h"
#include "pqScalarValueListPropertyWidget.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptors.h"
#include "pqSubsetInclusionLatticeTreeModel.h"
#include "pqSubsetInclusionLatticeWidget.h"
#include "pqTextEdit.h"
#include "pqTreeView.h"
#include "pqTreeViewSelectionHelper.h"
#include "pqTreeWidget.h"
#include "vtkPVConfig.h"

#include <QComboBox>
#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QRegExp>
#include <QStyle>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <cassert>

#if VTK_MODULE_ENABLE_ParaView_pqPython
#include "pqPythonSyntaxHighlighter.h"
#endif

pqStringVectorPropertyWidget::pqStringVectorPropertyWidget(
  vtkSMProperty* smProperty, vtkSMProxy* smProxy, QWidget* pWidget)
  : pqPropertyWidget(smProxy, pWidget)
{
  this->setChangeAvailableAsChangeFinished(false);

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(smProperty);
  if (!svp)
  {
    return;
  }

  vtkPVXMLElement* hints = svp->GetHints();

  bool multiline_text = false;
  bool python = false;
  QString placeholderText;
  if (hints)
  {
    vtkPVXMLElement* widgetHint = hints->FindNestedElementByName("Widget");
    if (widgetHint && widgetHint->GetAttribute("type") &&
      strcmp(widgetHint->GetAttribute("type"), "multi_line") == 0)
    {
      multiline_text = true;
    }
    if (widgetHint && widgetHint->GetAttribute("syntax") &&
      strcmp(widgetHint->GetAttribute("syntax"), "python") == 0)
    {
      python = true;
    }
    if (vtkPVXMLElement* phtElement = hints->FindNestedElementByName("PlaceholderText"))
    {
      placeholderText = phtElement->GetCharacterData();
      placeholderText = placeholderText.trimmed();
    }
  }

  // find the domain(s)
  vtkSMEnumerationDomain* enumerationDomain = 0;
  vtkSMFileListDomain* fileListDomain = 0;
  vtkSMArrayListDomain* arrayListDomain = 0;
  vtkSMStringListDomain* stringListDomain = 0;
  vtkSMSILDomain* silDomain = 0;
  vtkSMArraySelectionDomain* arraySelectionDomain = 0;
  vtkSMSubsetInclusionLatticeDomain* silDomain2 = nullptr;

  vtkSMDomainIterator* domainIter = svp->NewDomainIterator();
  for (domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
  {
    vtkSMDomain* domain = domainIter->GetDomain();
    enumerationDomain =
      enumerationDomain ? enumerationDomain : vtkSMEnumerationDomain::SafeDownCast(domain);
    fileListDomain = fileListDomain ? fileListDomain : vtkSMFileListDomain::SafeDownCast(domain);
    arrayListDomain =
      arrayListDomain ? arrayListDomain : vtkSMArrayListDomain::SafeDownCast(domain);
    silDomain = silDomain ? silDomain : vtkSMSILDomain::SafeDownCast(domain);
    silDomain2 = silDomain2 ? silDomain2 : vtkSMSubsetInclusionLatticeDomain::SafeDownCast(domain);
    arraySelectionDomain =
      arraySelectionDomain ? arraySelectionDomain : vtkSMArraySelectionDomain::SafeDownCast(domain);
    stringListDomain =
      stringListDomain ? stringListDomain : vtkSMStringListDomain::SafeDownCast(domain);
  }
  domainIter->Delete();

  QVBoxLayout* vbox = new QVBoxLayout;
  vbox->setMargin(0);
  vbox->setSpacing(0);

  if (fileListDomain)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqFileChooserWidget`.");
    pqFileChooserWidget* chooser = new pqFileChooserWidget(this);
    chooser->setObjectName("FileChooser");

    // decide whether to allow multiple files
    if (smProperty->GetRepeatable())
    {
      // multiple file names allowed
      chooser->setForceSingleFile(false);

      this->addPropertyLink(
        chooser, "filenames", SIGNAL(filenamesChanged(QStringList)), smProperty);
      this->setChangeAvailableAsChangeFinished(true);
    }
    else
    {
      // single file name
      chooser->setForceSingleFile(true);
      this->addPropertyLink(
        chooser, "singleFilename", SIGNAL(filenameChanged(QString)), smProperty);
      this->setChangeAvailableAsChangeFinished(true);
    }

    // If there's a hint on the smproperty indicating that this smproperty expects a
    // directory name, then, we will use the directory mode.

    if (hints && hints->FindNestedElementByName("UseDirectoryName"))
    {
      chooser->setUseDirectoryMode(true);
    }

    // If there's a hint on the smproperty indicating that this smproperty accepts
    // any file name, then, we will use the AnyFile mode.

    if (hints && hints->FindNestedElementByName("AcceptAnyFile"))
    {
      chooser->setAcceptAnyFile(true);
    }

    QStringList supportedExtensions;
    for (unsigned int cc = 0, max = (hints ? hints->GetNumberOfNestedElements() : 0); cc < max;
         ++cc)
    {
      vtkPVXMLElement* childXML = hints->GetNestedElement(cc);
      if (childXML && childXML->GetName() && strcmp(childXML->GetName(), "FileChooser") == 0)
      {
        const char* extensions = childXML->GetAttribute("extensions");
        const char* file_description = childXML->GetAttribute("file_description");
        if (!extensions || !file_description)
        {
          vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "incomplete `FileChooser` hint skipped.");
        }
        else
        {
          QStringList lextensions =
            QString(extensions).split(QRegExp("\\s+"), QString::SkipEmptyParts);
          supportedExtensions.push_back(
            QString("%1 (*.%2)").arg(file_description).arg(lextensions.join(" *.")));
        }
      }
    }
    if (supportedExtensions.size() > 0)
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "using extensions `%s`",
        supportedExtensions.join(", ").toLocal8Bit().data());
      chooser->setExtension(supportedExtensions.join(";;"));
    }

    pqServerManagerModel* smm = pqApplicationCore::instance()->getServerManagerModel();
    chooser->setServer(smm->findServer(smProxy->GetSession()));

    vbox->addWidget(chooser);
  }
  else if (arrayListDomain)
  {
    assert(smProperty->GetRepeatable());

    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqArraySelectionWidget`.");

    // repeatable array list domains get a the pqArraySelectionWidget
    // that lists each array name with a check box
    auto selectorWidget = new pqArraySelectionWidget(this);
    selectorWidget->setObjectName("ArraySelectionWidget");
    selectorWidget->setHeaderLabel(smProperty->GetXMLLabel());
    selectorWidget->setMaximumRowCountBeforeScrolling(
      pqPropertyWidget::hintsWidgetHeightNumberOfRows(smProperty->GetHints()));

    // add context menu and custom indicator for sorting and filtering.
    new pqTreeViewSelectionHelper(selectorWidget);

    // hide widget label
    this->setShowLabel(false);

    vbox->addWidget(selectorWidget);

    // unlike vtkSMArraySelectionDomain which is doesn't tend to change based
    // on values of other properties, typically, vtkSMArrayListDomain changes
    // on other property values e.g. when one switches from point-data to
    // cell-data, the available arrays change. Hence we "reset" the widget
    // every time the domain changes.
    new pqArrayListDomain(
      selectorWidget, smProxy->GetPropertyName(smProperty), smProxy, smProperty, arrayListDomain);

    const char* property_name = smProxy->GetPropertyName(smProperty);
    // pass icon hints, if provided.
    if (auto aswhints = hints ? hints->FindNestedElementByName("ArraySelectionWidget") : nullptr)
    {
      selectorWidget->setIconType(property_name, aswhints->GetAttribute("icon_type"));
    }
    this->addPropertyLink(selectorWidget, property_name, SIGNAL(widgetModified()), smProperty);
    this->setChangeAvailableAsChangeFinished(true);
  }
  else if (silDomain)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqSILWidget`.");
    pqSILWidget* tree = new pqSILWidget(silDomain->GetSubTree(), this);
    tree->setObjectName("BlockSelectionWidget");

    pqSILModel* silModel = new pqSILModel(tree);

    // FIXME: This needs to be automated, we want the model to automatically
    // fetch the SIL when the domain is updated.
    silModel->setSILDomain(silDomain);
    silModel->update();
    tree->setModel(silModel);

    this->addPropertyLink(tree->activeModel(), "values", SIGNAL(valuesChanged()), smProperty);
    this->setChangeAvailableAsChangeFinished(true);

    // hide widget label
    setShowLabel(false);

    vbox->addWidget(tree);
  }
  else if (silDomain2)
  {
    this->setShowLabel(false);

    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqSubsetInclusionLatticeWidget`.");
    auto model = new pqSubsetInclusionLatticeTreeModel(this);

    model->setSubsetInclusionLattice(silDomain2->GetSIL());
    pqSubsetInclusionLatticeWidget* silWidget = new pqSubsetInclusionLatticeWidget(model, this);
    vbox->addWidget(silWidget);
    this->addPropertyLink(model, "selection", SIGNAL(selectionModified()), smProperty);
    this->setChangeAvailableAsChangeFinished(true);
  }
  else if (arraySelectionDomain)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqArraySelectionWidget`.");
    auto selectorWidget = new pqArraySelectionWidget(this);
    selectorWidget->setObjectName("ArraySelectionWidget");
    selectorWidget->setHeaderLabel(smProperty->GetXMLLabel());
    selectorWidget->setMaximumRowCountBeforeScrolling(
      pqPropertyWidget::hintsWidgetHeightNumberOfRows(smProperty->GetHints()));

    // add context menu and custom indicator for sorting and filtering.
    new pqTreeViewSelectionHelper(selectorWidget);

    this->addPropertyLink(
      selectorWidget, smProxy->GetPropertyName(smProperty), SIGNAL(widgetModified()), smProperty);
    this->setChangeAvailableAsChangeFinished(true);

    // hide widget label
    setShowLabel(false);

    vbox->addWidget(selectorWidget);

    new pqArrayListDomain(selectorWidget, smProxy->GetPropertyName(smProperty), smProxy, smProperty,
      arraySelectionDomain);
  }
  else if (stringListDomain)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `QComboBox`.");
    QComboBox* comboBox = new QComboBox(this);
    comboBox->setObjectName("ComboBox");

    pqSignalAdaptorComboBox* adaptor = new pqSignalAdaptorComboBox(comboBox);
    new pqComboBoxDomain(comboBox, smProperty);
    this->addPropertyLink(adaptor, "currentText", SIGNAL(currentTextChanged(QString)), svp);
    this->setChangeAvailableAsChangeFinished(true);

    vbox->addWidget(comboBox);
  }
  else if (multiline_text)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqTextEdit`.");

    QWidget* w = new QWidget(this);
    QHBoxLayout* hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    QLabel* label = new QLabel(smProperty->GetXMLLabel(), w);
    hbox->addWidget(label);
    hbox->addStretch();

    // add a multiline text widget
    pqTextEdit* textEdit = new pqTextEdit(this);
    QFont textFont("Courier");
    textEdit->setFont(textFont);
    textEdit->setObjectName(smProxy->GetPropertyName(smProperty));
    textEdit->setAcceptRichText(false);
    textEdit->setTabStopWidth(2);
    textEdit->setLineWrapMode(QTextEdit::NoWrap);

    this->setChangeAvailableAsChangeFinished(false);
    this->addPropertyLink(textEdit, "plainText", SIGNAL(textChanged()), smProperty);
    this->connect(
      textEdit, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(changeFinished()));

    w->setLayout(hbox);
    vbox->addWidget(w);
    if (python)
    {
#if VTK_MODULE_ENABLE_ParaView_pqPython
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "supports Python syntax highlighter.");
      new pqPythonSyntaxHighlighter(textEdit, textEdit);
#else
      vtkVLogF(
        PARAVIEW_LOG_APPLICATION_VERBOSITY(), "Python not enabled, no syntax highlighter support.");
#endif
      pqPopOutWidget* popOut = new pqPopOutWidget(textEdit,
        QString("%1 - %2").arg(smProperty->GetParent()->GetXMLLabel(), smProperty->GetXMLLabel()),
        this);
      QPushButton* popToDialogButton = new QPushButton(this);
      popOut->setPopOutButton(popToDialogButton);
      hbox->addWidget(popToDialogButton);
      vbox->addWidget(popOut);
    }
    else
    {
      vbox->addWidget(textEdit);
    }
    this->setShowLabel(false);
  }
  else if (enumerationDomain)
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqComboBoxDomain`.");
    QComboBox* comboBox = new QComboBox(this);
    comboBox->setObjectName("ComboBox");

    pqSignalAdaptorComboBox* adaptor = new pqSignalAdaptorComboBox(comboBox);

    this->addPropertyLink(adaptor, "currentText", SIGNAL(currentTextChanged(QString)), svp);
    this->setChangeAvailableAsChangeFinished(true);

    for (unsigned int i = 0; i < enumerationDomain->GetNumberOfEntries(); i++)
    {
      comboBox->addItem(enumerationDomain->GetEntryText(i));
    }

    vbox->addWidget(comboBox);
  }
  else
  {
    if (smProperty->GetRepeatable())
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqScalarValueListPropertyWidget`.");
      pqScalarValueListPropertyWidget* widget =
        new pqScalarValueListPropertyWidget(smProperty, smProxy, this);
      widget->setObjectName("ScalarValueList");
      this->addPropertyLink(widget, "scalars", SIGNAL(scalarsChanged()), smProperty);
      this->setChangeAvailableAsChangeFinished(true);
      vbox->addWidget(widget);
      this->setShowLabel(false);
    }
    else
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqLineEdit`.");
      // add a single line edit.
      QLineEdit* lineEdit = new pqLineEdit(this);
      lineEdit->setObjectName(smProxy->GetPropertyName(smProperty));
      this->addPropertyLink(lineEdit, "text", SIGNAL(textChanged(const QString&)), smProperty);
      this->connect(
        lineEdit, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(changeFinished()));
      this->setChangeAvailableAsChangeFinished(false);

      if (!placeholderText.isEmpty())
      {
        lineEdit->setPlaceholderText(placeholderText);
      }

      vbox->addWidget(lineEdit);
    }
  }
  this->setLayout(vbox);
}

//-----------------------------------------------------------------------------
pqStringVectorPropertyWidget::~pqStringVectorPropertyWidget()
{
}

//-----------------------------------------------------------------------------
pqPropertyWidget* pqStringVectorPropertyWidget::createWidget(
  vtkSMStringVectorProperty* svp, vtkSMProxy* smproxy, QWidget* parent)
{
  if (svp && svp->FindDomain<vtkSMArrayListDomain>() && !svp->GetRepeatable())
  {
    return new pqArraySelectorPropertyWidget(svp, smproxy, parent);
  }
  return new pqStringVectorPropertyWidget(svp, smproxy, parent);
}
