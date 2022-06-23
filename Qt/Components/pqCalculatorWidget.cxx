/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "pqCalculatorWidget.h"
#include "ui_pqCalculatorWidget.h"

#include "pqCoreUtilities.h"
#include "pqExpressionsDialog.h"
#include "pqExpressionsManager.h"
#include "pqOneLinerTextEdit.h"
#include "pqOutputPort.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QMenu>
#include <QPointer>
#include <QRegularExpression>

class pqCalculatorWidget::pqInternals : public Ui::CalculatorWidget
{
public:
  QPointer<QMenu> ScalarsMenu;
  QPointer<QMenu> VectorsMenu;

  pqInternals()
    : ScalarsMenu(new QMenu())
    , VectorsMenu(new QMenu())
  {
  }
  ~pqInternals()
  {
    delete this->ScalarsMenu;
    delete this->VectorsMenu;
  }
};

//-----------------------------------------------------------------------------
pqCalculatorWidget::pqCalculatorWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqInternals())
{
  this->Internals->setupUi(this);
  this->setShowLabel(false);
  this->setChangeAvailableAsChangeFinished(false);

  this->Internals->Vectors->setMenu(this->Internals->VectorsMenu);
  this->Internals->Scalars->setMenu(this->Internals->ScalarsMenu);

  // before the menus are popped up, fill them up with the list of available
  // arrays.
  QObject::connect(
    this->Internals->ScalarsMenu, SIGNAL(aboutToShow()), this, SLOT(updateVariableNames()));
  QObject::connect(
    this->Internals->VectorsMenu, SIGNAL(aboutToShow()), this, SLOT(updateVariableNames()));

  // update the text when the user choses an variable in the menu.
  QObject::connect(this->Internals->VectorsMenu, SIGNAL(triggered(QAction*)), this,
    SLOT(variableChosen(QAction*)));
  QObject::connect(this->Internals->ScalarsMenu, SIGNAL(triggered(QAction*)), this,
    SLOT(variableChosen(QAction*)));

  //--------------------------------------------------------------------------
  // connect all buttons for which the text of the button
  // is the same as what goes into the function
  QRegularExpression regexp("^([ijk]Hat|ln|log10|sin|cos|"
                            "tan|asin|acos|atan|sinh|cosh|tanh|"
                            "sqrt|exp|ceil|floor|abs|norm|mag|"
                            "LeftParentheses|RightParentheses|"
                            "Divide|Multiply|Minus|Plus)$");

  QList<QToolButton*> buttons;
  buttons = this->findChildren<QToolButton*>(regexp);
  Q_FOREACH (QToolButton* tb, buttons)
  {
    QObject::connect(tb, &QToolButton::pressed, this, [=]() { this->buttonPressed(tb->text()); });
  }

  QObject::connect(
    this->Internals->xy, &QToolButton::pressed, this, [=]() { this->buttonPressed("^"); });
  QObject::connect(
    this->Internals->dot, &QToolButton::pressed, this, [=]() { this->buttonPressed("dot"); });

  //--------------------------------------------------------------------------
  static const QString expressionGroup = pqExpressionsManager::EXPRESSION_GROUP();
  this->Internals->Function->setupButtons(expressionGroup);

  this->addPropertyLink(
    this->Internals->Function->lineEdit(), "plainText", SIGNAL(textChanged()), smproperty);

  // now when editing is finished, we will fire the changeFinished() signal.
  this->connect(this->Internals->Function->lineEdit(), SIGNAL(textChangedAndEditingFinished()),
    this, SIGNAL(changeFinished()));

  this->updateButtons();

  // clicking on any button or any part of the panel where another button
  // doesn't take focus will cause the line edit to have focus
  this->setFocusProxy(this->Internals->Function->lineEdit());
}

//-----------------------------------------------------------------------------
pqCalculatorWidget::~pqCalculatorWidget()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//-----------------------------------------------------------------------------
void pqCalculatorWidget::variableChosen(QAction* menuAction)
{
  if (menuAction)
  {
    this->Internals->Function->lineEdit()->insertPlainText(menuAction->text());
  }
}

//-----------------------------------------------------------------------------
void pqCalculatorWidget::buttonPressed(const QString& buttonText)
{
  this->Internals->Function->lineEdit()->insertPlainText(buttonText);
}

//-----------------------------------------------------------------------------
void pqCalculatorWidget::updateVariableNames()
{
  vtkSMUncheckedPropertyHelper attributeType(this->proxy(), "AttributeType");
  this->updateVariables(attributeType.GetAsString(0));
}

//-----------------------------------------------------------------------------
void pqCalculatorWidget::updateVariables(const QString& mode)
{
  this->Internals->VectorsMenu->clear();
  this->Internals->ScalarsMenu->clear();

  if (mode == "Point Data")
  {
    this->Internals->VectorsMenu->addAction("coords");
    this->Internals->ScalarsMenu->addAction("coordsX");
    this->Internals->ScalarsMenu->addAction("coordsY");
    this->Internals->ScalarsMenu->addAction("coordsZ");
  }

  vtkPVDataSetAttributesInformation* fdi = nullptr;
  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(
    vtkSMUncheckedPropertyHelper(this->proxy(), "Input").GetAsProxy(0));
  if (!input)
  {
    return;
  }

  if (mode == "Point Data")
  {
    fdi = input->GetDataInformation(0)->GetPointDataInformation();
  }
  else if (mode == "Cell Data")
  {
    fdi = input->GetDataInformation(0)->GetCellDataInformation();
  }
  else if (mode == "Vertex Data")
  {
    fdi = input->GetDataInformation(0)->GetVertexDataInformation();
  }
  else if (mode == "Edge Data")
  {
    fdi = input->GetDataInformation(0)->GetEdgeDataInformation();
  }
  else if (mode == "Row Data")
  {
    fdi = input->GetDataInformation(0)->GetRowDataInformation();
  }

  if (!fdi)
  {
    return;
  }

  for (int i = 0; i < fdi->GetNumberOfArrays(); i++)
  {
    vtkPVArrayInformation* arrayInfo = fdi->GetArrayInformation(i);
    if (arrayInfo->GetDataType() == VTK_STRING || arrayInfo->GetDataType() == VTK_VARIANT)
    {
      continue;
    }

    int numComponents = arrayInfo->GetNumberOfComponents();
    // the following check is performed to ensure that the user will use an acceptable
    // name (function-wise) for the desired array
    bool arrayNameSanitized =
      vtkSMCoreUtilities::SanitizeName(arrayInfo->GetName()) == arrayInfo->GetName();

    for (int j = 0; j < numComponents; j++)
    {
      if (numComponents == 1)
      {
        QString name = arrayNameSanitized
          ? arrayInfo->GetName()
          : std::string('\"' + std::string(arrayInfo->GetName()) + '\"').c_str();
        this->Internals->ScalarsMenu->addAction(name);
      }
      else
      {
        bool componentNameSanitized = vtkSMCoreUtilities::SanitizeName(arrayInfo->GetComponentName(
                                        j)) == arrayInfo->GetComponentName(j);
        QString n;
        if (!arrayNameSanitized || !componentNameSanitized)
        {
          n = QString("\"") + QString(arrayInfo->GetName()) +
            QString("_%1").arg(arrayInfo->GetComponentName(j)) + QString("\"");
        }
        else
        {
          n = QString(arrayInfo->GetName()) + QString("_%1").arg(arrayInfo->GetComponentName(j));
        }
        QAction* a = new QAction(n, this->Internals->ScalarsMenu);
        QStringList d;
        QString name = arrayNameSanitized
          ? arrayInfo->GetName()
          : std::string('\"' + std::string(arrayInfo->GetName()) + '\"').c_str();
        d.append(name);
        d.append(QString("%1").arg(j));
        a->setData(d);
        this->Internals->ScalarsMenu->addAction(a);
      }
    }

    if (numComponents == 3)
    {
      QString name = arrayNameSanitized
        ? arrayInfo->GetName()
        : std::string('\"' + std::string(arrayInfo->GetName()) + '\"').c_str();
      this->Internals->VectorsMenu->addAction(name);
    }
  }
}

//-----------------------------------------------------------------------------
void pqCalculatorWidget::updateButtons()
{
  vtkSMUncheckedPropertyHelper parserType(this->proxy(), "FunctionParserType");
  if (parserType.GetAsInt() == 1)
  {
    this->Internals->dot->setText("dot");
  }
  else
  {
    this->Internals->dot->setText("v1.v2");
  }
}
