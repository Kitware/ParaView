// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCompositePropertyDecorator.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

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
  std::vector<vtkSmartPointer<vtkPropertyDecorator>> Decorators;
  std::vector<std::shared_ptr<BaseOperation>> Expressions;

  void Add(std::shared_ptr<BaseOperation>& op) { this->Expressions.push_back(op); }

  void Add(vtkPropertyDecorator* op) { this->Decorators.push_back(op); }

  virtual bool CanShow(bool show_advanced) const = 0;
  virtual bool Enable() const = 0;

  virtual ~BaseOperation() = default;
};

template <typename BinaryOperation, bool init_value, bool default_value = init_value>
struct Operation : public BaseOperation
{
  bool CanShow(bool show_advanced) const override
  {
    if (this->Expressions.empty() && this->Decorators.empty())
    {
      return default_value;
    }

    bool result = init_value;

    result = std::accumulate(this->Decorators.begin(), this->Decorators.end(), result,
      [=](const bool& lhs, const vtkSmartPointer<vtkPropertyDecorator>& rhs)
      { return BinaryOperation()(lhs, rhs->CanShow(show_advanced)); });

    result = std::accumulate(this->Expressions.begin(), this->Expressions.end(), result,
      [=](const bool& lhs, const std::shared_ptr<BaseOperation>& rhs)
      { return BinaryOperation()(lhs, rhs->CanShow(show_advanced)); });

    return result;
  }

  bool Enable() const override
  {
    if (this->Expressions.empty() && this->Decorators.empty())
    {
      return default_value;
    }

    bool result = init_value;

    result = std::accumulate(this->Decorators.begin(), this->Decorators.end(), result,
      [](const bool& lhs, const vtkSmartPointer<vtkPropertyDecorator>& rhs)
      { return BinaryOperation()(lhs, rhs->Enable()); });

    result = std::accumulate(this->Expressions.begin(), this->Expressions.end(), result,
      [=](const bool& lhs, const std::shared_ptr<BaseOperation>& rhs)
      { return BinaryOperation()(lhs, rhs->Enable()); });

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

class vtkCompositePropertyDecorator::vtkInternals
{
public:
  std::vector<vtkCompositePropertyDecorator::DecoratorCreationFunction> creators;
  std::shared_ptr<BaseOperation> Expression;

  std::shared_ptr<BaseOperation> Parse(vtkPVXMLElement* expXML, vtkCompositePropertyDecorator* self)
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
          expr->Add(childExpr);
        }
      }
      else
      {
        vtkSmartPointer<vtkPropertyDecorator> decorator;
        for (const auto& create : this->creators)
        {
          decorator = create(childXML, self->Proxy());
          if (decorator)
          {
            break;
          }
        }
        if (decorator)
        {
          this->HandleNestedDecorator(decorator, self);
          expr->Add(decorator);
        }
        else
        {
          vtkWarningWithObjectMacro(self, << "Could not create decorator");
          childXML->PrintXML();
        }
      }
    }
    return expr;
  }

  void HandleNestedDecorator(vtkPropertyDecorator* other, vtkCompositePropertyDecorator* self)
  {
    other->AddObserver(vtkPropertyDecorator::VisibilityChangedEvent, self,
      &vtkCompositePropertyDecorator::InvokeVisibilityChangedEvent);
    other->AddObserver(vtkPropertyDecorator::EnableStateChangedEvent, self,
      &vtkCompositePropertyDecorator::InvokeEnableStateChangedEvent);
  }
};

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCompositePropertyDecorator);

//-----------------------------------------------------------------------------
vtkCompositePropertyDecorator::vtkCompositePropertyDecorator()
{

  this->Internals = std::unique_ptr<vtkInternals>(new vtkInternals());
  // this is the default Create function of the parent class
  this->RegisterDecorator(vtkPropertyDecorator::Create);
}

//-----------------------------------------------------------------------------
vtkCompositePropertyDecorator::~vtkCompositePropertyDecorator() = default;

//-----------------------------------------------------------------------------
void vtkCompositePropertyDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Expression: " << this->Internals->Expression << std::endl;
}
//-----------------------------------------------------------------------------
void vtkCompositePropertyDecorator::RegisterDecorator(
  vtkCompositePropertyDecorator::DecoratorCreationFunction func)
{
  this->Internals->creators.push_back(func);
}

//-----------------------------------------------------------------------------
void vtkCompositePropertyDecorator::Initialize(vtkPVXMLElement* xmlConfig, vtkSMProxy* proxy)
{
  this->Superclass::Initialize(xmlConfig, proxy);

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
    if (!this->Internals->Expression)
    {
      vtkWarningMacro(<< "invalid expression " << stream.str());
    }
  }
}

//-----------------------------------------------------------------------------
bool vtkCompositePropertyDecorator::CanShow(bool show_advanced) const
{
  auto internals = (*this->Internals);
  return internals.Expression ? internals.Expression->CanShow(show_advanced)
                              : this->Superclass::CanShow(show_advanced);
}

//-----------------------------------------------------------------------------
bool vtkCompositePropertyDecorator::Enable() const
{
  auto internals = (*this->Internals);
  return internals.Expression ? internals.Expression->Enable() : this->Superclass::Enable();
}
