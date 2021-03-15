/*=========================================================================

   Program: ParaView
   Module:  pqSelectionQueryPropertyWidget.cxx

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
#include "pqSelectionQueryPropertyWidget.h"

#include "pqComboBoxDomain.h"
#include "pqCoreUtilities.h"
#include "pqDoubleLineEdit.h"
#include "pqLineEdit.h"
#include "pqPropertiesPanel.h"
#include "pqTimer.h"
#include "vtkLogger.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMProperty.h"
#include "vtkSMSelectionQueryDomain.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <map>
#include <tuple>

namespace
{
QStringList splitTerms(const QString& value)
{
  QStringList result;
  int parentCount = 0;
  int start = 0, pos = 0;
  for (const int max = value.size(); pos < max; ++pos)
  {
    if (value[pos] == '(')
    {
      ++parentCount;
    }
    else if (value[pos] == ')')
    {
      --parentCount;
    }
    else if (parentCount == 0 && value[pos] == '&')
    {
      result.push_back(value.mid(start, pos - start).trimmed());
      start = pos + 1;
    }
  }

  if (parentCount != 0)
  {
    vtkLogF(WARNING, "mismatched parenthesis in expression: '%s'", qPrintable(value));
  }
  if (pos != start)
  {
    result.push_back(value.mid(start, pos - start).trimmed());
  }

  // remove extra parens, if any from each term.
  for (auto& part : result)
  {
    if (part.front() == '(' && part.back() == ')')
    {
      part = part.mid(1, part.size() - 2).trimmed();
    }
  }
  return result;
}

QString fmt(QString str, const std::map<QString, QString>& map)
{
  for (auto pair : map)
  {
    str.replace(QString("{%1}").arg(pair.first), pair.second);
  }
  return str;
}

QRegularExpression fmtRegEx(QString str, const std::map<QString, QString>& map)
{
  for (auto pair : map)
  {
    const auto key = QString("{%1}").arg(pair.first);
    int index = str.indexOf(key);
    // replace 1st occurrence by (?<$key>$pattern) and subsequent ones
    // by \g{$key}.
    if (index != -1)
    {
      str.replace(index, key.size(), QString("(?<%1>%2)").arg(pair.first).arg(pair.second));
    }
    str.replace(key, QString("\\g{%1}").arg(pair.first));
  }

  QRegularExpression r("^" + str + "$");
  vtkLogIfF(ERROR, !r.isValid(), "bad regex '%s'", qPrintable(r.pattern()));
  return r;
}
}

//=============================================================================
/**
 * pqValueWidget represents a QWidget in which the user can enter a value
 * (or multiple values). This widget has 4 forms identified by ValueType.
 * Based on the chosen form, the widget allows the user to enter multiple (or none)
 * values.
 */
//=============================================================================
class pqSelectionQueryPropertyWidget::pqValueWidget : public QWidget
{
  QList<pqLineEdit*> LineEdits;

public:
  pqValueWidget(pqSelectionQueryPropertyWidget::pqQueryWidget* prnt);

  enum ValueType
  {
    NO_VALUE,
    SINGLE_VALUE,
    COMMA_SEPARATED_VALUES,
    RANGE_PAIR,
    LOCATION_WITH_TOLERANCE,
    LOCATION,
  };

  /**
   * Set the type for the value widget. Based on the type the user will be
   * presented a UI with 0 or more input entries.
   */
  void setType(ValueType type)
  {
    if (this->Type != type)
    {
      this->Type = type;
      this->rebuildUI();
    }
  }

  /**
   * Returns current user-specified values are a string-list.
   */
  std::map<QString, QString> value() const
  {
    std::map<QString, QString> result;
    for (const auto& lineEdit : this->LineEdits)
    {
      result[lineEdit->objectName()] = lineEdit->text();
    }
    return result;
  }

  /**
   * Set the current value.
   */
  void setValue(const QRegularExpressionMatch& match)
  {
    for (auto lineEdit : this->LineEdits)
    {
      lineEdit->setText(match.captured(lineEdit->objectName()));
    }
  }

private:
  Q_DISABLE_COPY(pqValueWidget);
  ValueType Type = NO_VALUE;

  void modified();

  void rebuildUI()
  {
    // delete existing children.
    auto widgets = this->findChildren<QWidget*>();
    for (auto& wdg : widgets)
    {
      delete wdg;
    }
    this->LineEdits.clear();
    delete this->layout();

    switch (this->Type)
    {
      case SINGLE_VALUE:
      case COMMA_SEPARATED_VALUES:
      {
        auto vbox = new QVBoxLayout(this);
        vbox->setMargin(0);
        vbox->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
        auto edit = new pqLineEdit(this);
        this->LineEdits.push_back(edit);
        edit->setObjectName("value");
        vbox->addWidget(edit);

        edit->setPlaceholderText(this->Type == SINGLE_VALUE ? "value" : "comma separated values");
      }
      break;

      case RANGE_PAIR:
      {
        auto hbox = new QHBoxLayout(this);
        hbox->setMargin(0);
        hbox->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

        auto editMin = new pqLineEdit(this);
        editMin->setObjectName("value_min");
        editMin->setPlaceholderText("minimum");

        auto editMax = new pqLineEdit(this);
        editMax->setObjectName("value_max");
        editMax->setPlaceholderText("maximum");

        auto label = new QLabel("and");
        label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        hbox->addWidget(editMin, /*strech=*/1);
        hbox->addWidget(label);
        hbox->addWidget(editMax, /*strech=*/1);
        this->LineEdits.push_back(editMin);
        this->LineEdits.push_back(editMax);
      }
      break;

      case LOCATION:
      case LOCATION_WITH_TOLERANCE:
      {
        auto grid = new QGridLayout(this);
        grid->setMargin(0);
        grid->setVerticalSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
        grid->setHorizontalSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
        auto editX = new pqDoubleLineEdit(this);
        editX->setObjectName("value_x");
        editX->setPlaceholderText("X coordinate");
        auto editY = new pqDoubleLineEdit(this);
        editY->setObjectName("value_y");
        editY->setPlaceholderText("Y coordinate");
        auto editZ = new pqDoubleLineEdit(this);
        editZ->setObjectName("value_z");
        editZ->setPlaceholderText("Z coordinate");
        grid->addWidget(editX, 0, 0);
        grid->addWidget(editY, 0, 1);
        grid->addWidget(editZ, 0, 2);
        this->LineEdits.push_back(editX);
        this->LineEdits.push_back(editY);
        this->LineEdits.push_back(editZ);
        if (this->Type == LOCATION_WITH_TOLERANCE)
        {
          auto editTolerance = new pqDoubleLineEdit(this);
          editTolerance->setObjectName("value_tolerance");
          editTolerance->setPlaceholderText("within epsilon");
          grid->addWidget(editTolerance, 1, 0, 1, 3);
          this->LineEdits.push_back(editTolerance);
        }
      }
      break;

      case NO_VALUE:
      default:
        // nothing to do.
        break;
    }

    for (const auto& lineEdit : this->LineEdits)
    {
      QObject::connect(
        lineEdit, &pqLineEdit::textChangedAndEditingFinished, this, &pqValueWidget::modified);
    }
  }
};

//=============================================================================
/**
 * pqQueryWidget is a widget corresponding to a single query expression.
 * pqSelectionQueryPropertyWidget supports multiple expressions (all joined by `&`)
 * and this class represents a single one of those expressions. It has 3 parts:
 * term, operator and value.
 */
//=============================================================================
class pqSelectionQueryPropertyWidget::pqQueryWidget : public QWidget
{
  using Superclass = QWidget;
  using ValueWidgetType = pqSelectionQueryPropertyWidget::pqValueWidget;

  enum TermType
  {
    ARRAY,
    POINT_NEAREST_TO,
    CELL_CONTAINING_POINT,
  };

public:
  pqQueryWidget(pqSelectionQueryPropertyWidget* parentWdg)
    : Superclass(parentWdg)
    , Term(new QComboBox(this))
    , Operator(new QComboBox(this))
    , Value(new pqValueWidget(this))
  {
    this->Term->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    this->Operator->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    auto hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());
    hbox->addWidget(this->Term, 0, Qt::AlignTop);
    hbox->addWidget(this->Operator, 0, Qt::AlignTop);
    hbox->addWidget(this->Value, 1, Qt::AlignTop);

    QObject::connect(this->Term, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int) {
      this->populateOperators(this->currentTermType());
      this->populateValue();
      this->modified();
    });

    QObject::connect(
      this->Operator, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int) {
        this->populateValue();
        this->modified();
      });
  }

  /**
   * Returns an expression.
   */
  QString expression() const
  {
    auto map = this->Value->value();
    map["term"] = this->currentTerm();
    auto expr = this->Operator->currentData(ExprRole).toString();
    return ::fmt(expr, map);
  }

  void update(const QString& expr, vtkSMProperty* smproperty)
  {
    const auto hasTerms = this->populateTerms(smproperty);
    this->populateOperators(this->currentTermType());
    this->populateValue();

    this->Term->setEnabled(hasTerms);
    this->Operator->setEnabled(hasTerms);
    this->Operator->setEnabled(hasTerms);

    this->setExpression(expr);
  }

  void setExpression(const QString& expr)
  {
    // we loop from reverse so when done, the operators is set to the first item
    // rather than last one.
    for (int termType = TermType::CELL_CONTAINING_POINT; termType >= 0; --termType)
    {
      this->populateOperators(termType);
      for (int cc = 0; cc < this->Operator->count(); ++cc)
      {
        const auto regex = this->Operator->itemData(cc, ExprRegExRole).toRegularExpression();
        auto match = regex.match(expr);
        if (match.hasMatch())
        {
          // vtkLogF(INFO, "match! expr='%s' pattern='%s'", qPrintable(expr),
          // qPrintable(regex.pattern()));
          // note, while the term "inputs" is same for CELL_CONTAINING_POINT and
          // POINT_NEAREST_TO there is no ambiguity since only 1 of them is
          // present at a time based on element type.
          this->setCurrentTerm(match.captured("term"));
          this->Operator->setCurrentIndex(cc);
          this->Value->setValue(match);
          return;
        }
        // else
        //{
        //  vtkLogF(INFO, "no match! expr='%s' pattern='%s'", qPrintable(expr),
        //  qPrintable(regex.pattern()));
        //}
      }
    }

    // TODO: we're providing an expression we can't adequately process,
    // maybe we simply want to show a line edit that the user can use to enter the values?
    this->Term->setCurrentIndex(0);
    this->Operator->setCurrentIndex(0);
    this->Value->setValue({});
  }

  void modified()
  {
    if (auto prnt = qobject_cast<pqSelectionQueryPropertyWidget*>(this->parentWidget()))
    {
      Q_EMIT prnt->queryChanged();
    }
  }

protected:
  bool populateTerms(vtkSMProperty* smproperty);
  void populateOperators(int termType);
  void populateValue();

  TermType currentTermType() const
  {
    return static_cast<TermType>(this->Term->currentData(TermTypeRole).toInt());
  }

  QString currentTerm() const { return this->Term->currentData(NameRole).toString(); }

  void setCurrentTerm(const QString& name)
  {
    const int index = this->Term->findData(name, NameRole);
    if (index != -1)
    {
      this->Term->setCurrentIndex(index);
    }
    else
    {
      this->Term->insertItem(0, name + "(?)");
      this->Term->setItemData(0, TermType::ARRAY, TermTypeRole);
      this->Term->setItemData(0, name, NameRole);
      this->Term->setCurrentIndex(0);
    }
  }

  constexpr static int TermTypeRole = Qt::UserRole;
  constexpr static int NameRole = Qt::UserRole + 1;
  constexpr static int ValueTypeRole = Qt::UserRole;
  constexpr static int ExprRole = Qt::UserRole + 1;
  constexpr static int ExprRegExRole = Qt::UserRole + 2;

  void addTerm(const QIcon& icon, const QString& text, TermType ttype, const QString& name) const
  {
    const int index = this->Term->count();
    this->Term->addItem(icon, text);
    this->Term->setItemData(index, ttype, TermTypeRole);
    this->Term->setItemData(index, name, NameRole);
  }

  void addOperator(const QString& text, ValueWidgetType::ValueType vtype, const QString& expr)
  {
    const int index = this->Operator->count();
    this->Operator->addItem(text);
    this->Operator->setItemData(index, vtype, ValueTypeRole);
    this->Operator->setItemData(index, expr, ExprRole);

    QString regex(expr);
    regex.replace("(", "\\(").replace(")", "\\)"); // escape parens.
    regex.replace("[", "\\[").replace("]", "\\]"); // escape brackets.

    std::map<QString, QString> map;
    map["term"] = "\\w+";
    map["value"] = "[a-zA-Z0-9\\_\\,\\ \\-]+";
    map["value_min"] = map["value_max"] = map["value_x"] = map["value_y"] = map["value_z"] =
      map["value_tolerance"] = "[a-zA-Z0-9\\_\\.\\-]+";

    this->Operator->setItemData(index, ::fmtRegEx(regex, map), ExprRegExRole);
  }

private:
  Q_DISABLE_COPY(pqQueryWidget);
  QComboBox* Term;
  QComboBox* Operator;
  pqValueWidget* Value;
};

bool pqSelectionQueryPropertyWidget::pqQueryWidget::populateTerms(vtkSMProperty* smproperty)
{
  const QSignalBlocker blocker(this->Term);

  this->Term->clear();
  // Populate items in the Term combobox based on the current input domain and
  // selected element type.
  auto domain = smproperty->FindDomain<vtkSMSelectionQueryDomain>();
  Q_ASSERT(domain);
  if (auto dataInfo = domain->GetInputDataInformation("Input"))
  {
    const auto elementType = domain->GetElementType();
    const auto icon = pqComboBoxDomain::getIcon(elementType);

    // add id.
    this->addTerm(icon, "ID", TermType::ARRAY, "id");

    auto attrInfo = dataInfo->GetAttributeInformation(elementType);
    for (int cc = 0, max = attrInfo->GetNumberOfArrays(); cc < max; ++cc)
    {
      auto arrayInfo = attrInfo->GetArrayInformation(cc);
      this->addTerm(icon, arrayInfo->GetName(), TermType::ARRAY,
        vtkSMCoreUtilities::SanitizeName(arrayInfo->GetName()).c_str());
    }

    if (elementType == vtkDataObject::POINT)
    {
      this->addTerm(icon, "Point", TermType::POINT_NEAREST_TO, "inputs");
    }
    else if (elementType == vtkDataObject::CELL)
    {
      this->addTerm(icon, "Cell", TermType::CELL_CONTAINING_POINT, "inputs");
    }
    return true;
  }
  return false;
}

void pqSelectionQueryPropertyWidget::pqQueryWidget::populateOperators(int termType)
{
  // Setup the Operator combobox based the currently chosen value in the Term
  // combobox.
  using ValueWidget = pqSelectionQueryPropertyWidget::pqValueWidget;

  const QSignalBlocker blocker(this->Operator);
  this->Operator->clear();

  switch (termType)
  {
    case TermType::ARRAY:
      this->addOperator("is", ValueWidget::SINGLE_VALUE, "{term} == {value}");
      this->addOperator(
        "is in range", ValueWidget::RANGE_PAIR, "({term} > {value_min}) & ({term} < {value_max})");
      this->addOperator(
        "is one of", ValueWidget::COMMA_SEPARATED_VALUES, "in1d({term}, [{value}])");
      this->addOperator("is >=", ValueWidget::SINGLE_VALUE, "{term} >= {value}");
      this->addOperator("is <=", ValueWidget::SINGLE_VALUE, "{term} <= {value}");
      this->addOperator("is min", ValueWidget::NO_VALUE, "{term} == min({term})");
      this->addOperator("is max", ValueWidget::NO_VALUE, "{term} == max({term})");
      this->addOperator("is NaN", ValueWidget::NO_VALUE, "isnan({term})");
      this->addOperator("is <= mean", ValueWidget::NO_VALUE, "{term} <= mean({term})");
      this->addOperator("is >= mean", ValueWidget::NO_VALUE, "{term} >= mean({term})");
      this->addOperator(
        "is mean", ValueWidget::SINGLE_VALUE, "abs({term} - mean({term})) <= {value}");
      break;

    case TermType::POINT_NEAREST_TO:
      this->addOperator("nearest to", ValueWidget::LOCATION_WITH_TOLERANCE,
        "pointIsNear([({value_x}, {value_y}, {value_z}),], {value_tolerance}, {term})");
      break;

    case TermType::CELL_CONTAINING_POINT:
      this->addOperator("containing", ValueWidget::LOCATION,
        "cellContainsPoint({term}, [({value_x}, {value_y}, {value_z}),])");
      break;
  }
}

void pqSelectionQueryPropertyWidget::pqQueryWidget::populateValue()
{
  // Setup the Value widget based on the currently chosen operator type.
  using ValueWidget = pqSelectionQueryPropertyWidget::pqValueWidget;
  const QSignalBlocker blocker(this->Value);
  this->Value->setType(
    static_cast<ValueWidget::ValueType>(this->Operator->currentData(ValueTypeRole).toInt()));
}

pqSelectionQueryPropertyWidget::pqValueWidget::pqValueWidget(pqQueryWidget* prnt)
  : QWidget(prnt)
{
}

void pqSelectionQueryPropertyWidget::pqValueWidget::modified()
{
  auto prnt = reinterpret_cast<pqQueryWidget*>(this->parentWidget());
  Q_ASSERT(prnt);
  prnt->modified();
}

//=============================================================================
class pqSelectionQueryPropertyWidget::pqInternals
{
  using QueryWidgetType = pqSelectionQueryPropertyWidget::pqQueryWidget;

  /**
   * Inserts a new pqQueryWidget at the requested index.
   */
  void insertQuery(int index, const QString& expr = QString())
  {
    auto hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(pqPropertiesPanel::suggestedHorizontalSpacing());

    auto wdg = new QueryWidgetType(this->Parent);
    wdg->update(expr, this->Parent->property());
    hbox->addWidget(wdg, /*stretch=*/1, Qt::AlignTop);

    auto plus = new QPushButton(QIcon(":/QtWidgets/Icons/pqPlus.svg"), QString(), this->Parent);
    hbox->addWidget(plus, 0, Qt::AlignTop);

    auto minus = new QPushButton(QIcon(":/QtWidgets/Icons/pqMinus.svg"), QString(), this->Parent);
    hbox->addWidget(minus, 0, Qt::AlignTop);

    this->QueryWidgets.insert(index, std::make_tuple(wdg, plus, minus));

    auto vbox = qobject_cast<QVBoxLayout*>(this->Parent->layout());
    vbox->insertLayout(index, hbox);

    QObject::connect(plus, &QAbstractButton::clicked,
      [this, wdg](bool) { this->insertQuery(this->indexOf(wdg) + 1); });

    QObject::connect(minus, &QAbstractButton::clicked,
      [this, wdg](bool) { this->removeQuery(this->indexOf(wdg)); });

    Q_EMIT this->Parent->queryChanged();

    this->updateEnabledState();
  }

  /**
   * Removes a pqQueryWidget (and other supporting UI elements) for expression
   * at a given index.
   */
  void removeQuery(int index)
  {
    auto layout = this->Parent->layout();
    if (QLayoutItem* litem = layout->takeAt(index))
    {
      delete litem->widget();
      delete litem;

      auto tuple = this->QueryWidgets.takeAt(index);
      delete std::get<0>(tuple);
      delete std::get<1>(tuple);
      delete std::get<2>(tuple);

      Q_EMIT this->Parent->queryChanged();
    }

    this->updateEnabledState();
  }

  int indexOf(QueryWidgetType* wdg) const
  {
    for (int cc = 0, max = this->QueryWidgets.size(); cc < max; ++cc)
    {
      if (std::get<0>(this->QueryWidgets[cc]) == wdg)
      {
        return cc;
      }
    }
    return -1;
  }

  void updateEnabledState() const
  {
    const bool can_remove = (this->QueryWidgets.size() > 1);
    for (auto& tuple : this->QueryWidgets)
    {
      std::get<2>(tuple)->setEnabled(can_remove);
    }
  }

  mutable QString Query;

public:
  pqSelectionQueryPropertyWidget* Parent;
  pqTimer DomainTimer;
  QList<std::tuple<QueryWidgetType*, QPushButton*, QPushButton*> > QueryWidgets;

  pqInternals(pqSelectionQueryPropertyWidget* self)
    : Parent(self)
  {
    auto vbox = new QVBoxLayout(self);
    vbox->setMargin(0);
    vbox->setSpacing(pqPropertiesPanel::suggestedVerticalSpacing());
  }

  void populateUI()
  {
    auto parts = splitTerms(this->Query);
    if (parts.size() == 0)
    {
      // we need at least 1 expression.
      parts.push_back(QString());
    }

    // remove extra ones.
    while (this->QueryWidgets.size() > parts.size())
    {
      this->removeQuery(this->QueryWidgets.size() - 1);
    }

    // update existing and add new as needed.
    for (int cc = 0; cc < this->QueryWidgets.size(); ++cc)
    {
      auto queryWidget = std::get<0>(this->QueryWidgets[cc]);
      queryWidget->update(parts[cc], this->Parent->property());
    }
    for (int cc = this->QueryWidgets.size(); cc < parts.size(); ++cc)
    {
      this->insertQuery(cc, parts[cc]);
    }
  }

  const QString& query() const
  {
    QStringList exprs;
    for (const auto& tuple : this->QueryWidgets)
    {
      auto singleQuery = std::get<0>(tuple)->expression();
      if (!singleQuery.isEmpty())
      {
        exprs.push_back(QString("(%1)").arg(singleQuery));
      }
    }
    this->Query = exprs.join("&");
    // vtkLogF(INFO, "expr = %s", this->Query.toStdString().c_str());
    return this->Query;
  }

  void setQuery(const QString& value)
  {
    if (this->Query != value)
    {
      this->Query = value;
      this->populateUI();
    }
  }
};

//=============================================================================
pqSelectionQueryPropertyWidget::pqSelectionQueryPropertyWidget(
  vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject)
  : Superclass(smproxy, parentObject)
  , Internals(new pqSelectionQueryPropertyWidget::pqInternals(this))
{
  this->setShowLabel(false);
  this->setProperty(smproperty);

  auto& internals = (*this->Internals);
  internals.DomainTimer.setSingleShot(true);
  internals.DomainTimer.setInterval(0);

  auto domain = smproperty->FindDomain<vtkSMSelectionQueryDomain>();
  if (domain)
  {
    pqCoreUtilities::connect(
      domain, vtkCommand::DomainModifiedEvent, &internals.DomainTimer, SLOT(start()));
  }

  QObject::connect(&internals.DomainTimer, &QTimer::timeout, [this, &internals]() {
    const QSignalBlocker blocker(this);
    internals.populateUI();
  });

  internals.populateUI();
  this->addPropertyLink(this, "query", SIGNAL(queryChanged()), smproperty);
}

//-----------------------------------------------------------------------------
pqSelectionQueryPropertyWidget::~pqSelectionQueryPropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqSelectionQueryPropertyWidget::setQuery(const QString& value)
{
  auto& internals = (*this->Internals);
  internals.setQuery(value);
  Q_EMIT this->queryChanged();
}

//-----------------------------------------------------------------------------
const QString& pqSelectionQueryPropertyWidget::query() const
{
  auto& internals = (*this->Internals);
  return internals.query();
}
