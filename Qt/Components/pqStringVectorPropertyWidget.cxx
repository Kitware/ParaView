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
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"

#include "pqApplicationCore.h"
#include "pqArrayListDomain.h"
#include "pqArrayListWidget.h"
#include "pqArraySelectionWidget.h"
#include "pqArraySelectorPropertyWidget.h"
#include "pqComboBoxDomain.h"
#include "pqCoreUtilities.h"
#include "pqDialog.h"
#include "pqExpandableTableView.h"
#include "pqExpressionChooserButton.h"
#include "pqExpressionsDialog.h"
#include "pqExpressionsManager.h"
#include "pqExpressionsWidget.h"
#include "pqFileChooserWidget.h"
#include "pqLineEdit.h"
#include "pqOneLinerTextEdit.h"
#include "pqPopOutWidget.h"
#include "pqQtDeprecated.h"
#include "pqSMAdaptor.h"
#include "pqScalarValueListPropertyWidget.h"
#include "pqServerManagerModel.h"
#include "pqSignalAdaptors.h"
#include "pqTextEdit.h"
#include "pqTreeView.h"
#include "pqTreeViewSelectionHelper.h"
#include "pqTreeWidget.h"

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
#include "pqPythonScriptEditor.h"
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
  bool wrap_text = false;
  bool python = false;
  bool showLabel = false;
  QString placeholderText;
  if (hints)
  {
    vtkPVXMLElement* widgetHint = hints->FindNestedElementByName("Widget");
    if (widgetHint && widgetHint->GetAttribute("type") &&
      strcmp(widgetHint->GetAttribute("type"), "multi_line") == 0)
    {
      multiline_text = true;
    }
    else if (widgetHint && widgetHint->GetAttribute("type") &&
      strcmp(widgetHint->GetAttribute("type"), "one_liner_wrapped") == 0)
    {
      wrap_text = true;
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
    showLabel = (hints->FindNestedElementByName("ShowLabel") != nullptr);
  }

  // find the domain(s)
  vtkSMEnumerationDomain* enumerationDomain = nullptr;
  vtkSMFileListDomain* fileListDomain = nullptr;
  vtkSMArrayListDomain* arrayListDomain = nullptr;
  vtkSMStringListDomain* stringListDomain = nullptr;
  vtkSMArraySelectionDomain* arraySelectionDomain = nullptr;

  vtkSMDomainIterator* domainIter = svp->NewDomainIterator();
  for (domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
  {
    vtkSMDomain* domain = domainIter->GetDomain();
    enumerationDomain =
      enumerationDomain ? enumerationDomain : vtkSMEnumerationDomain::SafeDownCast(domain);
    fileListDomain = fileListDomain ? fileListDomain : vtkSMFileListDomain::SafeDownCast(domain);
    arrayListDomain =
      arrayListDomain ? arrayListDomain : vtkSMArrayListDomain::SafeDownCast(domain);
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
    chooser->setTitle(QString("Select %1").arg(smProperty->GetXMLLabel()));

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

    // process hints.
    bool directoryMode, anyFile, browseLocalFileSystem;
    QString filter;
    pqStringVectorPropertyWidget::processFileChooserHints(
      hints, directoryMode, anyFile, filter, browseLocalFileSystem);
    chooser->setUseDirectoryMode(directoryMode);
    chooser->setAcceptAnyFile(anyFile);
    chooser->setExtension(filter);
    if (!browseLocalFileSystem)
    {
      pqServerManagerModel* smm = pqApplicationCore::instance()->getServerManagerModel();
      chooser->setServer(smm->findServer(smProxy->GetSession()));
    }
    else
    {
      chooser->setServer(nullptr);
    }

    vbox->addWidget(chooser);
  }
  else if (arrayListDomain)
  {
    assert(smProperty->GetRepeatable());

    int type = svp->GetElementType(1);
    int nbElements = svp->GetNumberOfElementsPerCommand();
    const char* property_name = smProxy->GetPropertyName(smProperty);
    QWidget* listWidget = nullptr;
    if (type == vtkSMStringVectorProperty::STRING && nbElements == 2)
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqArrayListWidget`.");

      auto arrayListWidget = new pqArrayListWidget(this);
      arrayListWidget->setObjectName("ArrayListWidget");
      arrayListWidget->setHeaderLabel(property_name);
      arrayListWidget->setMaximumRowCountBeforeScrolling(
        pqPropertyWidget::hintsWidgetHeightNumberOfRows(smProperty->GetHints()));

      if (auto aswHints = hints ? hints->FindNestedElementByName("ArraySelectionWidget") : nullptr)
      {
        arrayListWidget->setIconType(aswHints->GetAttribute("icon_type"));
      }

      listWidget = arrayListWidget;
    }
    else
    {
      vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqArraySelectionWidget`.");

      auto selectorWidget = new pqArraySelectionWidget(this);
      selectorWidget->setObjectName("ArraySelectionWidget");
      selectorWidget->setHeaderLabel(smProperty->GetXMLLabel());
      selectorWidget->setMaximumRowCountBeforeScrolling(
        pqPropertyWidget::hintsWidgetHeightNumberOfRows(smProperty->GetHints()));

      // add context menu and custom indicator for sorting and filtering.
      new pqTreeViewSelectionHelper(selectorWidget);
      // pass icon hints, if provided.
      if (auto aswhints = hints ? hints->FindNestedElementByName("ArraySelectionWidget") : nullptr)
      {
        selectorWidget->setIconType(property_name, aswhints->GetAttribute("icon_type"));
      }

      listWidget = selectorWidget;
    }

    // hide widget label
    this->setShowLabel(showLabel);

    vbox->addWidget(listWidget);

    // unlike vtkSMArraySelectionDomain which doesn't tend to change based
    // on values of other properties, typically, vtkSMArrayListDomain changes
    // on other property values e.g. when one switches from point-data to
    // cell-data, the available arrays change. Hence we "reset" the widget
    // every time the domain changes.
    new pqArrayListDomain(
      listWidget, smProxy->GetPropertyName(smProperty), smProxy, smProperty, arrayListDomain);

    this->addPropertyLink(listWidget, property_name, SIGNAL(widgetModified()), smProperty);
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
    this->setShowLabel(showLabel);

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
    // tab is 2 spaces
    textEdit->setTabStopDistance(this->fontMetrics().horizontalAdvance("  "));
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
      auto highlighter = new pqPythonSyntaxHighlighter(textEdit, *textEdit);
      highlighter->ConnectHighligter();

      QPushButton* pushButton = new QPushButton(this);
      pushButton->setIcon(this->style()->standardIcon(QStyle::SP_TitleBarMaxButton));
      this->connect(pushButton, &QPushButton::clicked, [textEdit]() {
        pqPythonScriptEditor::linkTo(textEdit);
        pqPythonScriptEditor::bringFront();
      });
      hbox->addWidget(pushButton);
      vbox->addWidget(textEdit);
#else
      vtkVLogF(
        PARAVIEW_LOG_APPLICATION_VERBOSITY(), "Python not enabled, no syntax highlighter support.");
      pqPopOutWidget* popOut = new pqPopOutWidget(textEdit,
        QString("%1 - %2").arg(smProperty->GetParent()->GetXMLLabel(), smProperty->GetXMLLabel()),
        this);
      QPushButton* popToDialogButton = new QPushButton(this);
      popOut->setPopOutButton(popToDialogButton);
      hbox->addWidget(popToDialogButton);
      vbox->addWidget(popOut);
#endif
    }
    else
    {
      vbox->addWidget(textEdit);
    }
    this->setShowLabel(showLabel);
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
      this->setShowLabel(showLabel);
    }
    else
    {
      // add a single line edit.
      if (wrap_text)
      {
        vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqOneLinerTextEdit`.");

        QString group = python ? pqExpressionsManager::PYTHON_EXPRESSION_GROUP()
                               : pqExpressionsManager::EXPRESSION_GROUP();
        auto expressionsWidget = new pqExpressionsWidget(this, group);
        expressionsWidget->setObjectName("ExpressionWidget");
        vbox->addWidget(expressionsWidget);

        pqOneLinerTextEdit* lineEdit = expressionsWidget->lineEdit();
        if (!placeholderText.isEmpty())
        {
          lineEdit->setPlaceholderText(placeholderText);
        }

        this->addPropertyLink(lineEdit, "plainText", SIGNAL(textChanged()), smProperty);
        this->connect(
          lineEdit, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(changeFinished()));
      }
      else
      {
        vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "use `pqLineEdit`.");
        pqLineEdit* lineEdit = new pqLineEdit(this);
        if (!placeholderText.isEmpty())
        {
          lineEdit->setPlaceholderText(placeholderText);
        }

        this->addPropertyLink(lineEdit, "text", SIGNAL(textChanged(const QString&)), smProperty);
        lineEdit->setObjectName(smProxy->GetPropertyName(smProperty));
        vbox->addWidget(lineEdit);
        this->connect(
          lineEdit, SIGNAL(textChangedAndEditingFinished()), this, SIGNAL(changeFinished()));
      }

      this->setChangeAvailableAsChangeFinished(false);
    }
  }
  this->setLayout(vbox);
}

//-----------------------------------------------------------------------------
pqStringVectorPropertyWidget::~pqStringVectorPropertyWidget() = default;

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

//-----------------------------------------------------------------------------
void pqStringVectorPropertyWidget::processFileChooserHints(vtkPVXMLElement* hints,
  bool& directoryMode, bool& anyFile, QString& filter, bool& browseLocalFileSystem)
{
  // If there's a hint on the smproperty indicating that this smproperty expects a
  // directory name, then, we will use the directory mode.
  directoryMode = (hints && hints->FindNestedElementByName("UseDirectoryName"));

  // If there's a hint on the smproperty indicating that this smproperty accepts
  // any file name, then, we will use the AnyFile mode.
  anyFile = (hints && hints->FindNestedElementByName("AcceptAnyFile"));

  // For browsing of local file system.
  browseLocalFileSystem = (hints && hints->FindNestedElementByName("BrowseLocalFileSystem"));

  QStringList supportedExtensions;
  for (unsigned int cc = 0, max = (hints ? hints->GetNumberOfNestedElements() : 0); cc < max; ++cc)
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
          QString(extensions).split(QRegExp("\\s+"), PV_QT_SKIP_EMPTY_PARTS);
        supportedExtensions.push_back(
          QString("%1 (*.%2)").arg(file_description).arg(lextensions.join(" *.")));
      }
    }
  }

  if (!supportedExtensions.empty())
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "using extensions `%s`",
      supportedExtensions.join(", ").toUtf8().data());
    filter = supportedExtensions.join(";;");
  }
}
