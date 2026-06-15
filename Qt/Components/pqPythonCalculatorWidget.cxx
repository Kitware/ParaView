// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPythonCalculatorWidget.h"
#include "ui_pqPythonCalculatorWidget.h"

#include "pqCoreUtilities.h"
#include "pqExpressionsWidget.h"
#include "pqOneLinerTextEdit.h"
#if VTK_MODULE_ENABLE_ParaView_pqPython
#include "pqPythonCalculatorCompleter.h"
#endif
#include "pqTextEdit.h"
#include "pqTreeWidget.h"

#include "vtkDataObject.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMFieldDataDomain.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include "vtksys/SystemTools.hxx"

#include <QIcon>
#include <QPointer>
#include <QSignalBlocker>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>

//-----------------------------------------------------------------------------
class pqPythonCalculatorWidget::pqInternals : public Ui::PythonCalculatorWidget
{
public:
  QPointer<QTextEdit> LineEdit;
};

//-----------------------------------------------------------------------------
namespace
{
QIcon getAssociationIcon(int assoc)
{
  switch (assoc)
  {
    case vtkDataObject::POINT:
      return QIcon(":/pqWidgets/Icons/pqPointData.svg");
    case vtkDataObject::CELL:
      return QIcon(":/pqWidgets/Icons/pqCellData.svg");
    case vtkDataObject::FIELD:
      return QIcon(":/pqWidgets/Icons/pqGlobalData.svg");
    case vtkDataObject::VERTEX:
      return QIcon(":/pqWidgets/Icons/pqPointData.svg");
    case vtkDataObject::EDGE:
      return QIcon(":/pqWidgets/Icons/pqEdgeCenterData.svg");
    case vtkDataObject::ROW:
      return QIcon(":/pqWidgets/Icons/pqSpreadsheet.svg");
    default:
      return QIcon();
  }
}
} // namespace

//-----------------------------------------------------------------------------
pqPythonCalculatorWidget::pqPythonCalculatorWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproperty, smproxy, parentObject)
  , Internals(new pqInternals())
{
  this->setShowLabel(false);

  // Grab the text editor created by the superclass for text insertion.
  // one_liner_wrapped: pqExpressionsWidget > pqOneLinerTextEdit
  // multi_line:        pqTextEdit
  if (auto* exprWidget = this->findChild<pqExpressionsWidget*>("ExpressionWidget"))
  {
    this->Internals->LineEdit = exprWidget->lineEdit();
  }
  else if (auto* textEdit = this->findChild<pqTextEdit*>())
  {
    this->Internals->LineEdit = textEdit;
  }
  if (this->Internals->LineEdit)
  {
    this->setFocusProxy(this->Internals->LineEdit);
  }

  // Append the controls defined in the UI file below the superclass's expression widget.
  auto* controlsContainer = new QWidget(this);
  this->Internals->setupUi(controlsContainer);
  qobject_cast<QVBoxLayout*>(this->layout())->addWidget(controlsContainer);

  this->Internals->FunctionTree->header()->setStretchLastSection(false);
  this->Internals->FunctionTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
  this->Internals->FunctionTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

  // Re-populate inputs/arrays whenever the relevant proxy properties change via the panel.
  pqCoreUtilities::connect(this->proxy()->GetProperty("Input"),
    vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(updateInputs()));
  pqCoreUtilities::connect(this->proxy()->GetProperty("ArrayAssociation"),
    vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(updateArrays()));

  QObject::connect(this->Internals->Input, QOverload<int>::of(&QComboBox::currentIndexChanged),
    this, &pqPythonCalculatorWidget::updateArrays);
  QObject::connect(this->Internals->FilterByAssociation, &QCheckBox::toggled, this,
    &pqPythonCalculatorWidget::updateArrays);
  QObject::connect(this->Internals->Arrays, QOverload<int>::of(&QComboBox::activated), this,
    &pqPythonCalculatorWidget::arrayChosen);
  QObject::connect(this->Internals->FunctionTree, &QTreeWidget::itemDoubleClicked, this,
    &pqPythonCalculatorWidget::functionActivated);
  QObject::connect(this->Internals->FunctionSearch, &QLineEdit::textChanged, this,
    &pqPythonCalculatorWidget::filterFunctions);

#if VTK_MODULE_ENABLE_ParaView_pqPython
  auto firstSentence = [](const QString& doc) -> QString
  {
    if (doc.isEmpty())
    {
      return {};
    }
    // Take the first paragraph (up to the first blank line), collapse internal newlines.
    return doc.section("\n\n", 0, 0).replace('\n', ' ').simplified();
  };

  const QStringList modulesToIgnore = { "vtkmodules.util.misc", "paraview.vtk.util.numpy_support" };
  for (const auto& info : pqPythonCalculatorCompleter::getFunctionInfos(modulesToIgnore))
  {
    const QString tooltip = info.docstring.isEmpty()
      ? tr("%1 does not provide documentation.").arg(info.name)
      : info.docstring;
    auto* functionItem = new QTreeWidgetItem(
      this->Internals->FunctionTree, QStringList({ info.name, firstSentence(info.docstring) }));
    functionItem->setToolTip(0, tooltip);
    functionItem->setToolTip(1, tooltip);
  }
#else
  this->Internals->FunctionSearch->setEnabled(false);
  this->Internals->FunctionTree->setEnabled(false);
#endif

  this->updateInputs();
}

//-----------------------------------------------------------------------------
pqPythonCalculatorWidget::~pqPythonCalculatorWidget() = default;

//-----------------------------------------------------------------------------
void pqPythonCalculatorWidget::updateInputs()
{
  QSignalBlocker blocker(this->Internals->Input);
  const QVariant previousData = this->Internals->Input->currentData();
  this->Internals->Input->clear();

  vtkSMPropertyHelper inputHelper(this->proxy(), "Input");
  vtkSMPropertyHelper inputNameHelper(this->proxy(), "InputName");
  const unsigned int numInputs = inputHelper.GetNumberOfElements();
  if (numInputs == 0 || numInputs != inputNameHelper.GetNumberOfElements())
  {
    this->Internals->Input->addItem(tr("(none)"), -1);
    this->Internals->Input->setEnabled(false);
    this->updateArrays();
    return;
  }

  this->Internals->Input->setEnabled(numInputs > 1);
  for (unsigned int i = 0; i < numInputs; ++i)
  {
    this->Internals->Input->addItem(inputNameHelper.GetAsString(i), i);
  }

  const int previousIndex = this->Internals->Input->findData(previousData);
  if (previousIndex >= 0)
  {
    this->Internals->Input->setCurrentIndex(previousIndex);
  }
  this->updateArrays();
}

//-----------------------------------------------------------------------------
void pqPythonCalculatorWidget::updateArrays()
{
  QSignalBlocker blocker(this->Internals->Arrays);
  this->Internals->Arrays->clear();

  vtkSMUncheckedPropertyHelper arrayAssociationHelper(this->proxy(), "ArrayAssociation");
  const int currentAssociation = arrayAssociationHelper.GetAsInt();
  const bool filterByAssociation = this->Internals->FilterByAssociation->isChecked();

  const int inputIndex = this->currentInputIndex();
  if (inputIndex < 0)
  {
    return;
  }

  vtkSMPropertyHelper inputHelper(this->proxy(), "Input");
  if (inputIndex >= static_cast<int>(inputHelper.GetNumberOfElements()))
  {
    return;
  }

  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy(inputIndex));
  if (!input)
  {
    return;
  }

  vtkPVDataInformation* inputInfo =
    input->GetDataInformation(inputHelper.GetOutputPort(inputIndex));
  if (!inputInfo)
  {
    return;
  }

  if (filterByAssociation)
  {
    const std::string typeStr =
      vtkSMFieldDataDomain::GetElementTypeAsString(inputInfo, currentAssociation);
    this->Internals->Arrays->setPlaceholderText(
      !typeStr.empty() ? tr("%1 Arrays").arg(QString::fromStdString(typeStr)) : tr("Arrays"));
  }
  else
  {
    this->Internals->Arrays->setPlaceholderText(tr("All Arrays"));
  }

  auto addArrays = [&](vtkPVDataSetAttributesInformation* fdi, int assoc)
  {
    if (!fdi)
    {
      return;
    }
    for (int i = 0; i < fdi->GetNumberOfArrays(); ++i)
    {
      vtkPVArrayInformation* arrayInfo = fdi->GetArrayInformation(i);
      if (arrayInfo->GetDataType() == VTK_STRING || arrayInfo->GetDataType() == VTK_VARIANT)
      {
        continue;
      }
      QVariantList itemData;
      const QString name = arrayInfo->GetName();
      const bool isGlobal = arrayInfo->GetIsGlobal();
      itemData << QVariant(name) << QVariant(assoc) << QVariant(isGlobal);
      this->Internals->Arrays->addItem(getAssociationIcon(assoc), name, itemData);
    }
  };

  if (filterByAssociation)
  {
    addArrays(inputInfo->GetAttributeInformation(currentAssociation), currentAssociation);
  }
  else
  {
    for (int assoc : inputInfo->GetAttributeTypes())
    {
      addArrays(inputInfo->GetAttributeInformation(assoc), assoc);
    }
  }

  this->Internals->Arrays->setCurrentIndex(-1);
}

//-----------------------------------------------------------------------------
void pqPythonCalculatorWidget::arrayChosen(int index)
{
  if (!this->Internals->LineEdit)
  {
    return;
  }

  const int inputIndex = this->currentInputIndex();
  if (inputIndex < 0)
  {
    return;
  }

  vtkSMPropertyHelper inputHelper(this->proxy(), "Input");
  if (inputIndex >= static_cast<int>(inputHelper.GetNumberOfElements()))
  {
    return;
  }

  vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(inputHelper.GetAsProxy(inputIndex));
  if (!input)
  {
    return;
  }

  vtkPVDataInformation* inputInfo =
    input->GetDataInformation(inputHelper.GetOutputPort(inputIndex));
  if (!inputInfo)
  {
    return;
  }

  const QVariantList itemData = this->Internals->Arrays->itemData(index).toList();
  if (itemData.size() < 3)
  {
    return;
  }

  const QString arrayName = itemData[0].toString();
  const int association = itemData[1].toInt();
  const bool isGlobal = itemData[2].toBool();

  if (arrayName.isEmpty())
  {
    return;
  }

  const bool singleInput = inputHelper.GetNumberOfElements() == 1;
  const bool filterByAssociation = this->Internals->FilterByAssociation->isChecked();
  const bool useNamedInputs = this->Internals->UseNamedInputs->isChecked();

  QString expression;
  if (singleInput && filterByAssociation)
  {
    // calculator.py exposes all association arrays AND GlobalData arrays by bare name.
    expression = this->escapeArrayName(arrayName);
  }
  else
  {
    std::string accessor;
    if (isGlobal)
    {
      accessor = "GlobalData";
    }
    else
    {
      accessor = vtkSMFieldDataDomain::GetAttributeTypeAsString(inputInfo, association);
      if (accessor.empty())
      {
        return;
      }
      vtksys::SystemTools::ReplaceString(accessor, " ", "");
    }
    expression = QString("inputs[%1].%2[\"%3\"]")
                   .arg(useNamedInputs ? "\"" + this->Internals->Input->currentText() + "\""
                                       : QString::number(inputIndex))
                   .arg(accessor.c_str(), this->escapeArrayName(arrayName));
  }
  this->Internals->LineEdit->insertPlainText(expression);
  this->Internals->LineEdit->setFocus();

  QSignalBlocker blocker(this->Internals->Arrays);
  this->Internals->Arrays->setCurrentIndex(-1);
}

//-----------------------------------------------------------------------------
void pqPythonCalculatorWidget::functionActivated(QTreeWidgetItem* item, int /*column*/)
{
  if (!this->Internals->LineEdit || !item)
  {
    return;
  }
  const QString functionName = item->text(0);
  if (functionName.isEmpty())
  {
    return;
  }

  QTextCursor cursor = this->Internals->LineEdit->textCursor();
  cursor.insertText(functionName + "()");
  cursor.movePosition(QTextCursor::Left);
  this->Internals->LineEdit->setTextCursor(cursor);
  this->Internals->LineEdit->setFocus();
}

//-----------------------------------------------------------------------------
void pqPythonCalculatorWidget::filterFunctions(const QString& text)
{
  QTreeWidget* tree = this->Internals->FunctionTree;
  for (int i = 0; i < tree->topLevelItemCount(); ++i)
  {
    QTreeWidgetItem* fnItem = tree->topLevelItem(i);
    fnItem->setHidden(!text.isEmpty() && !fnItem->text(0).contains(text, Qt::CaseInsensitive));
  }
}

//-----------------------------------------------------------------------------
QString pqPythonCalculatorWidget::escapeArrayName(const QString& name) const
{
  QString escaped = name;
  escaped.replace("\\", "\\\\");
  escaped.replace("\"", "\\\"");
  return escaped;
}

//-----------------------------------------------------------------------------
int pqPythonCalculatorWidget::currentInputIndex() const
{
  return this->Internals->Input->currentData().toInt();
}
