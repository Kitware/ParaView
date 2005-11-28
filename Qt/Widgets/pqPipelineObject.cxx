
#include "pqPipelineObject.h"

#include "vtkSMProxy.h"
#include <QList>
#include <QWidget>


class pqPipelineObjectInternal
{
public:
  pqPipelineObjectInternal();
  ~pqPipelineObjectInternal() {}

  QList<pqPipelineObject *> Inputs;
  QList<pqPipelineObject *> Outputs;
};


pqPipelineObjectInternal::pqPipelineObjectInternal()
  : Inputs(), Outputs()
{
}


pqPipelineObject::pqPipelineObject(vtkSMProxy *proxy, ObjectType type)
{
  this->Internal = new pqPipelineObjectInternal();
  this->Display = 0;
  this->Type = type == pqPipelineObject::Window ? pqPipelineObject::Filter : type;
  this->Data.Proxy = proxy;
  this->Parent.Window = 0;
}

pqPipelineObject::pqPipelineObject(QWidget *widget)
{
  this->Internal = new pqPipelineObjectInternal();
  this->Display = 0;
  this->Type = pqPipelineObject::Window;
  this->Data.Widget = widget;
  this->Parent.Server = 0;
}

pqPipelineObject::~pqPipelineObject()
{
  // Clean up the connection lists.
  if(this->Internal)
    delete this->Internal;
}

void pqPipelineObject::SetType(ObjectType type)
{
  if(this->Type != pqPipelineObject::Window && type != pqPipelineObject::Window)
    this->Type = type;
}

vtkSMProxy *pqPipelineObject::GetProxy() const
{
  if(this->Type != pqPipelineObject::Window)
    return this->Data.Proxy;
  return 0;
}

void pqPipelineObject::SetProxy(vtkSMProxy *proxy)
{
  this->Type = pqPipelineObject::Filter;
  this->Data.Proxy = proxy;
}

QWidget *pqPipelineObject::GetWidget() const
{
  if(this->Type == pqPipelineObject::Window)
    return this->Data.Widget;
  return 0;
}

void pqPipelineObject::SetWidget(QWidget *widget)
{
  this->Type = pqPipelineObject::Window;
  this->Data.Widget = widget;
}

pqPipelineObject *pqPipelineObject::GetParent() const
{
  if(this->Type != pqPipelineObject::Window)
    return this->Parent.Window;
  return 0;
}

void pqPipelineObject::SetParent(pqPipelineObject *parent)
{
  if(this->Type != pqPipelineObject::Window && parent &&
      parent->Type == pqPipelineObject::Window)
    {
    this->Parent.Window = parent;
    }
}

pqPipelineServer *pqPipelineObject::GetServer() const
{
  if(this->Type == pqPipelineObject::Window)
    return this->Parent.Server;
  return 0;
}

void pqPipelineObject::SetServer(pqPipelineServer *server)
{
  if(this->Type == pqPipelineObject::Window)
    this->Parent.Server = server;
}

int pqPipelineObject::GetInputCount() const
{
  if(this->Internal)
    return this->Internal->Inputs.size();
  return 0;
}

pqPipelineObject *pqPipelineObject::GetInput(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Inputs.size())
    return this->Internal->Inputs[index];
  return 0;
}

void pqPipelineObject::AddInput(pqPipelineObject *intput)
{
  if(this->Internal && intput)
    this->Internal->Inputs.append(intput);
}

void pqPipelineObject::RemoveInput(pqPipelineObject *input)
{
  if(this->Internal && input)
    {
    QList<pqPipelineObject *>::Iterator iter = this->Internal->Inputs.begin();
    for( ; iter != this->Internal->Inputs.end(); ++iter)
      {
      if(*iter == input)
        {
        this->Internal->Inputs.erase(iter);
        break;
        }
      }
    }
}

int pqPipelineObject::GetOutputCount() const
{
  if(this->Internal)
    return this->Internal->Outputs.size();
  return 0;
}

pqPipelineObject *pqPipelineObject::GetOutput(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Outputs.size())
    return this->Internal->Outputs[index];
  return 0;
}

void pqPipelineObject::AddOutput(pqPipelineObject *output)
{
  if(this->Internal && output)
    this->Internal->Outputs.append(output);
}

void pqPipelineObject::RemoveOutput(pqPipelineObject *output)
{
  if(this->Internal && output)
    {
    QList<pqPipelineObject *>::Iterator iter = this->Internal->Outputs.begin();
    for( ; iter != this->Internal->Outputs.end(); ++iter)
      {
      if(*iter == output)
        {
        this->Internal->Outputs.erase(iter);
        break;
        }
      }
    }
}

void pqPipelineObject::ClearConnections()
{
  if(this->Internal)
    {
    this->Internal->Inputs.clear();
    this->Internal->Outputs.clear();
    }
}


