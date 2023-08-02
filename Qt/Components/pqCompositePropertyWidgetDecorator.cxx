// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCompositePropertyWidgetDecorator.h"

#include "pqPropertyWidget.h"
#include "pqPropertyWidgetDecorator.h"
#include "vtkLogger.h"
#include "vtkPVXMLElement.h"

#include <QPointer>

#include <algorithm>
#include <functional>
#include <memory>
#include <numeric>
#include <sstream>
#include <vector>

#include <cassert>

namespace
{

struct BaseOperation
{
  std::vector<QPointer<pqPropertyWidgetDecorator>> Decorators;
  std::vector<std::shared_ptr<BaseOperation>> Expressions;

  void add(std::shared_ptr<BaseOperation>& op) { this->Expressions.push_back(op); }

  void add(pqPropertyWidgetDecorator* op) { this->Decorators.push_back(op); }

  virtual bool canShowWidget(bool show_advanced) const = 0;
  virtual bool enableWidget() const = 0;

  virtual ~BaseOperation() = default;
};

template <typename BinaryOperation, bool init_value, bool default_value = init_value>
struct Operation : public BaseOperation
{
  bool canShowWidget(bool show_advanced) const override
  {
    if (this->Expressions.empty() && this->Decorators.empty())
    {
      return default_value;
    }

    bool result = init_value;

    result = std::accumulate(this->Decorators.begin(), this->Decorators.end(), result,
      [=](const bool& lhs, const QPointer<pqPropertyWidgetDecorator>& rhs) {
        return BinaryOperation()(lhs, rhs->canShowWidget(show_advanced));
      });

    result = std::accumulate(this->Expressions.begin(), this->Expressions.end(), result,
      [=](const bool& lhs, const std::shared_ptr<BaseOperation>& rhs) {
        return BinaryOperation()(lhs, rhs->canShowWidget(show_advanced));
      });

    return result;
  }

  bool enableWidget() const override
  {
    if (this->Expressions.empty() && this->Decorators.empty())
    {
      return default_value;
    }

    bool result = init_value;

    result = std::accumulate(this->Decorators.begin(), this->Decorators.end(), result,
      [](const bool& lhs, const QPointer<pqPropertyWidgetDecorator>& rhs) {
        return BinaryOperation()(lhs, rhs->enableWidget());
      });

    result = std::accumulate(this->Expressions.begin(), this->Expressions.end(), result,
      [=](const bool& lhs, const std::shared_ptr<BaseOperation>& rhs) {
        return BinaryOperation()(lhs, rhs->enableWidget());
      });

    return result;
  }
};

struct OperationAnd : public Operation<std::logical_and<bool>, true, true>
{
};

struct OperationOr : public Operation<std::logical_or<bool>, false, true>
{
};
}

class pqCompositePropertyWidgetDecorator::pqInternals
{
public:
  std::shared_ptr<BaseOperation> Expression;

  std::shared_ptr<BaseOperation> Parse(
    vtkPVXMLElement* expXML, pqCompositePropertyWidgetDecorator* self)
  {
    if (expXML == nullptr)
    {
      return nullptr;
    }

    std::shared_ptr<BaseOperation> expr;
    if (strcmp(expXML->GetAttributeOrEmpty("type"), "and") == 0)
    {
      expr = std::make_shared<OperationAnd>();
    }
    else if (strcmp(expXML->GetAttributeOrEmpty("type"), "or") == 0)
    {
      expr = std::make_shared<OperationOr>();
    }
    else
    {
      return nullptr;
    }

    for (unsigned int cc = 0, max = expXML->GetNumberOfNestedElements(); cc < max; ++cc)
    {
      vtkPVXMLElement* childXML = expXML->GetNestedElement(cc);
      if (childXML->GetName() && strcmp(childXML->GetName(), "Expression") == 0)
      {
        if (auto childExpr = this->Parse(childXML, self))
        {
          expr->add(childExpr);
        }
      }
      else if (auto decorator = pqPropertyWidgetDecorator::create(childXML, self->parentWidget()))
      {
        self->handleNestedDecorator(decorator);
        expr->add(decorator);
      }
    }
    return expr;
  }
};

//-----------------------------------------------------------------------------
pqCompositePropertyWidgetDecorator::pqCompositePropertyWidgetDecorator(
  vtkPVXMLElement* xmlConfig, pqPropertyWidget* parentObject)
  : Superclass(xmlConfig, parentObject)
  , Internals(new pqCompositePropertyWidgetDecorator::pqInternals())
{
  assert(xmlConfig);

  auto expressionXML = xmlConfig->FindNestedElementByName("Expression");
  this->Internals->Expression = this->Internals->Parse(expressionXML, this);
  if (this->Internals->Expression == nullptr)
  {
    std::ostringstream stream;
    if (expressionXML)
    {
      expressionXML->PrintXML(stream, vtkIndent());
    }
    else
    {
      stream << "(null)";
    }
    vtkLogIfF(
      WARNING, (!this->Internals->Expression), "invalid expression `%s`", stream.str().c_str());
  }
}

//-----------------------------------------------------------------------------
pqCompositePropertyWidgetDecorator::~pqCompositePropertyWidgetDecorator() = default;

//-----------------------------------------------------------------------------
bool pqCompositePropertyWidgetDecorator::canShowWidget(bool show_advanced) const
{
  auto internals = (*this->Internals);
  return internals.Expression ? internals.Expression->canShowWidget(show_advanced)
                              : this->Superclass::canShowWidget(show_advanced);
}

//-----------------------------------------------------------------------------
bool pqCompositePropertyWidgetDecorator::enableWidget() const
{
  auto internals = (*this->Internals);
  return internals.Expression ? internals.Expression->enableWidget()
                              : this->Superclass::enableWidget();
}

//-----------------------------------------------------------------------------
void pqCompositePropertyWidgetDecorator::handleNestedDecorator(pqPropertyWidgetDecorator* other)
{
  this->parentWidget()->removeDecorator(other);
  this->connect(other, SIGNAL(visibilityChanged()), SIGNAL(visibilityChanged()));
  this->connect(other, SIGNAL(enableStateChanged()), SIGNAL(enableStateChanged()));
}
