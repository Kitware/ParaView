// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCompositePropertyWidgetDecorator.h"

#include "pqCoreUtilities.h"
#include "pqPropertyWidget.h"
#include "pqPropertyWidgetDecorator.h"
#include "vtkObjectFactory.h"
#include "vtkPropertyDecorator.h"

#include <QPointer>
#include <cassert>

// A helper class that provides a vtkPropertyDecorator API for pqPropertyWidgetDecorator subclasses.
// This allows to handle any decorators that do not encapsulate their logic yet
// into a vtkPropertyDecorator subclass or for decorators loaded dynamically
// via plugins.
class vtkQtPropertyDecorator : public vtkPropertyDecorator
{
public:
  static vtkQtPropertyDecorator* New();
  vtkTypeMacro(vtkQtPropertyDecorator, vtkPropertyDecorator);
  // void PrintSelf(ostream& os, vtkIndent indent) override;

  bool CanShow(bool show_advanced) const override
  {
    return this->decoratorLogic->canShowWidget(show_advanced);
  }

  bool Enable() const override { return this->decoratorLogic->enableWidget(); }

  vtkQtPropertyDecorator() = default;
  ~vtkQtPropertyDecorator() override = default;

  void SetLogic(pqPropertyWidgetDecorator* logic)
  {
    if (this->decoratorLogic != logic)
    {
      // reset state
      if (this->decoratorLogic)
      {
        if (visibilityConnection)
        {
          QObject::disconnect(visibilityConnection);
        }
        if (enableStateConnection)
        {
          QObject::disconnect(enableStateConnection);
        }
      }
      this->decoratorLogic = logic;
      this->visibilityConnection =
        QObject::connect(logic, &pqPropertyWidgetDecorator::visibilityChanged,
          [this]() { this->InvokeVisibilityChangedEvent(); });

      this->enableStateConnection =
        QObject::connect(logic, &pqPropertyWidgetDecorator::enableStateChanged,
          [this]() { this->InvokeEnableStateChangedEvent(); });
    }
  }

private:
  QPointer<pqPropertyWidgetDecorator> decoratorLogic;
  QMetaObject::Connection visibilityConnection;
  QMetaObject::Connection enableStateConnection;
};

vtkStandardNewMacro(vtkQtPropertyDecorator);
//=============================================================================

//-----------------------------------------------------------------------------
pqCompositePropertyWidgetDecorator::pqCompositePropertyWidgetDecorator(
  vtkPVXMLElement* xmlConfig, pqPropertyWidget* parentObject)
  : Superclass(xmlConfig, parentObject)
{
  assert(xmlConfig);

  // provide callback to wrap pqPropertyWidgetDecorator subclasses
  auto creator = [parentObject](vtkPVXMLElement* xml, vtkSMProxy* proxy)
  {
    (void)(proxy);
    auto decorator = pqPropertyWidgetDecorator::create(xml, parentObject);
    auto vtkDecorator = vtkSmartPointer<vtkQtPropertyDecorator>::New();
    vtkDecorator->SetLogic(decorator);
    return vtkDecorator;
  };
  this->decoratorLogic->RegisterDecorator(creator);
  this->decoratorLogic->Initialize(xmlConfig, parentObject->proxy());

  pqCoreUtilities::connect(this->decoratorLogic, vtkPropertyDecorator::VisibilityChangedEvent, this,
    SIGNAL(visibilityChanged()));
  pqCoreUtilities::connect(this->decoratorLogic, vtkPropertyDecorator::EnableStateChangedEvent,
    this, SIGNAL(enableStateChanged()));
}

//-----------------------------------------------------------------------------
pqCompositePropertyWidgetDecorator::~pqCompositePropertyWidgetDecorator() = default;

//-----------------------------------------------------------------------------
bool pqCompositePropertyWidgetDecorator::canShowWidget(bool show_advanced) const
{
  return this->decoratorLogic->CanShow(show_advanced);
}

//-----------------------------------------------------------------------------
bool pqCompositePropertyWidgetDecorator::enableWidget() const
{
  return this->decoratorLogic->Enable();
}
