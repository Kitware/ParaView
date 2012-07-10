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

#include "pqApplicationCore.h"
#include "pqComboBoxDomain.h"
#include "pqExodusIIVariableSelectionWidget.h"
#include "pqFileChooserWidget.h"
#include "pqProxySILModel.h"
#include "pqServerManagerModel.h"
#include "pqSILModel.h"
#include "pqSILWidget.h"
#include "pqTreeWidget.h"
#include "pqTreeWidgetSelectionHelper.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <QDebug>

pqStringVectorPropertyWidget::pqStringVectorPropertyWidget(vtkSMProperty *smProperty,
                                                           vtkSMProxy *smProxy,
                                                           QWidget *parentWidget)
  : pqPropertyWidget(smProxy, parentWidget)
{
  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(smProperty);
  if(!svp)
    {
    return;
    }

  bool multiline_text = false;
  if (svp->GetHints())
    {
    vtkPVXMLElement* widgetHint = svp->GetHints()->FindNestedElementByName("Widget");
    if (widgetHint && widgetHint->GetAttribute("type") &&
      strcmp(widgetHint->GetAttribute("type"), "multi_line") == 0)
      {
      multiline_text = true;
      }
    }
  
  // find the domain(s)
  vtkSMEnumerationDomain *enumerationDomain = 0;
  vtkSMFileListDomain *fileListDomain = 0;
  vtkSMArrayListDomain *arrayListDomain = 0;
  vtkSMStringListDomain *stringListDomain = 0;
  vtkSMSILDomain *silDomain = 0;
  vtkSMArraySelectionDomain *arraySelectionDomain = 0;

  vtkSMDomainIterator *domainIter = svp->NewDomainIterator();
  for(domainIter->Begin(); !domainIter->IsAtEnd(); domainIter->Next())
    {
    vtkSMDomain *domain = domainIter->GetDomain();

    if(!enumerationDomain)
      {
      enumerationDomain = vtkSMEnumerationDomain::SafeDownCast(domain);
      }
    if(!fileListDomain)
      {
      fileListDomain = vtkSMFileListDomain::SafeDownCast(domain);
      }
    if(!arrayListDomain)
      {
      arrayListDomain = vtkSMArrayListDomain::SafeDownCast(domain);
      }
    if(!stringListDomain)
      {
      stringListDomain = vtkSMStringListDomain::SafeDownCast(domain);
      }
    if(!silDomain)
      {
      silDomain = vtkSMSILDomain::SafeDownCast(domain);
      }
    if(!arraySelectionDomain)
      {
      arraySelectionDomain = vtkSMArraySelectionDomain::SafeDownCast(domain);
      }
    }
  domainIter->Delete();

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->setMargin(0);
  vbox->setSpacing(0);

  if (fileListDomain)
    {
    // dont show properties with filename
//    if(smproperty->GetXMLLabel() == "FileName")
//      {
//      }

    pqFileChooserWidget *chooser = new pqFileChooserWidget(this);
    chooser->setObjectName("FileChooser");

    // decide whether to allow multiple files
    if(smProperty->GetRepeatable())
      {
      // multiple file names allowed
      chooser->setForceSingleFile(false);

      this->addPropertyLink(chooser,
                            "filenames",
                            SIGNAL(filenamesChanged(QStringList)),
                            smProperty);
      }
    else
      {
      // single file name
      chooser->setForceSingleFile(true);
      this->addPropertyLink(chooser,
                            "singleFilename",
                            SIGNAL(filenameChanged(QString)),
                            smProperty);
      }

    // If there's a hint on the smproperty indicating that this smproperty expects a
    // directory name, then, we will use the directory mode.
    if (smProperty->IsA("vtkSMStringVectorProperty") &&
        smProperty->GetHints() &&
        smProperty->GetHints()->FindNestedElementByName("UseDirectoryName"))
      {
      chooser->setUseDirectoryMode(true);
      }

    pqServerManagerModel *smm =
      pqApplicationCore::instance()->getServerManagerModel();
    chooser->setServer(smm->findServer(smProxy->GetSession()));

    vbox->addWidget(chooser);
    }
  else if(arrayListDomain)
    {
    if(smProperty->GetRepeatable())
      {
      // repeatable array list domains get a tree widget
      // listing each array name with a check box
      pqExodusIIVariableSelectionWidget* selectorWidget =
        new pqExodusIIVariableSelectionWidget(this);
      selectorWidget->setObjectName("ArraySelectionWidget");
      selectorWidget->setRootIsDecorated(false);
      selectorWidget->setHeaderLabel(smProperty->GetXMLLabel());
      this->addPropertyLink(
        selectorWidget, smProxy->GetPropertyName(smProperty),
        SIGNAL(widgetModified()), smProperty);

      // hide widget label
      this->setShowLabel(false);

      vbox->addWidget(selectorWidget);
      }
    else
      {
      // non-repeatable array list domains get a combo box
      // listing each array name
      QComboBox *comboBox = new QComboBox(this);
      comboBox->setObjectName("ComboBox");

      new pqComboBoxDomain(comboBox, smProperty, "array_list");

      this->addPropertyLink(comboBox,
                            "currentText",
                            SIGNAL(currentIndexChanged(QString)),
                            svp);

      vbox->addWidget(comboBox);
      }
    }
  else if(stringListDomain)
    {
    QComboBox *comboBox = new QComboBox(this);
    comboBox->setObjectName("ComboBox");

    for(unsigned int i = 0; i < stringListDomain->GetNumberOfStrings(); i++)
      {
      comboBox->addItem(stringListDomain->GetString(i));
      }

    this->addPropertyLink(comboBox, "currentText", SIGNAL(currentIndexChanged(QString)), svp);

    vbox->addWidget(comboBox);
    }
  else if (silDomain)
    {
    pqSILWidget* tree = new pqSILWidget(silDomain->GetSubTree(), this);
    tree->setObjectName("BlockSelectionWidget");

    pqSILModel* silModel = new pqSILModel(tree);

    // FIXME: This needs to be automated, we want the model to automatically
    // fetch the SIL when the domain is updated.
    silModel->update(silDomain->GetSIL());
    tree->setModel(silModel);

    this->addPropertyLink(tree->activeModel(), "values",
      SIGNAL(valuesChanged()), smProperty);

    // hide widget label
    setShowLabel(false);

    vbox->addWidget(tree);
    }
  else if (arraySelectionDomain)
    {
    pqExodusIIVariableSelectionWidget* selectorWidget =
      new pqExodusIIVariableSelectionWidget(this);
    selectorWidget->setObjectName("ArraySelectionWidget");
    selectorWidget->setRootIsDecorated(false);
    selectorWidget->setHeaderLabel(smProperty->GetXMLLabel());
    this->addPropertyLink(
      selectorWidget, smProxy->GetPropertyName(smProperty),
      SIGNAL(widgetModified()), smProperty);

    // hide widget label
    setShowLabel(false);

    vbox->addWidget(selectorWidget);
   }
  else if (multiline_text)
    {
    QLabel* label = new QLabel(smProperty->GetXMLLabel(), this);
    vbox->addWidget(label);

    // add a multiline text widget
    QTextEdit *textEdit = new QTextEdit(this);
    QFont textFont("Courier");
    textEdit->setFont(textFont);
    textEdit->setObjectName(smProxy->GetPropertyName(smProperty));
    textEdit->setAcceptRichText(false);
    textEdit->setTabStopWidth(2);
    textEdit->setLineWrapMode(QTextEdit::NoWrap);

    this->addPropertyLink(textEdit, "plainText",
      SIGNAL(textChanged()), smProperty);
    
    vbox->addWidget(textEdit);
    this->setShowLabel(false);
    }
  else if(enumerationDomain)
    {
    QComboBox *comboBox = new QComboBox(this);
    comboBox->setObjectName("ComboBox");

    for(unsigned int i = 0; i < enumerationDomain->GetNumberOfEntries(); i++)
      {
      comboBox->addItem(enumerationDomain->GetEntryText(i));
      }

    vbox->addWidget(comboBox);
    }
  else
    {
    // add a single line edit.
    QLineEdit* lineEdit = new QLineEdit(this);
    lineEdit->setObjectName(smProxy->GetPropertyName(smProperty));
    this->addPropertyLink(lineEdit, "text",
      SIGNAL(textChanged(const QString&)), smProperty);

    vbox->addWidget(lineEdit);
    }
  this->setLayout(vbox);
}
