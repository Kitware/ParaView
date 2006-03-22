
#include "pqPipelineObject.h"

#include "pqPipelineWindow.h"
#include "vtkSMProxy.h"
#include <QList>


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
  : ProxyName()
{
  this->Internal = new pqPipelineObjectInternal();
  this->Display = 0;
  this->Type = type;
  this->Proxy = proxy;
  this->Window = 0;
}

pqPipelineObject::~pqPipelineObject()
{
  // Clean up the connection lists.
  if(this->Internal)
    {
    delete this->Internal;
    }
}

int pqPipelineObject::GetInputCount() const
{
  if(this->Internal)
    {
    return this->Internal->Inputs.size();
    }
  return 0;
}

pqPipelineObject *pqPipelineObject::GetInput(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Inputs.size())
    {
    return this->Internal->Inputs[index];
    }
  return 0;
}

bool pqPipelineObject::HasInput(pqPipelineObject *input) const
{
  if(this->Internal && input)
    {
    QList<pqPipelineObject *>::Iterator iter = this->Internal->Inputs.begin();
    for( ; iter != this->Internal->Inputs.end(); ++iter)
      {
      if(*iter == input)
        {
        return true;
        }
      }
    }

  return false;
}

void pqPipelineObject::AddInput(pqPipelineObject *intput)
{
  if(this->Internal && intput)
    {
    this->Internal->Inputs.append(intput);
    }
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
    {
    return this->Internal->Outputs.size();
    }
  return 0;
}

pqPipelineObject *pqPipelineObject::GetOutput(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Outputs.size())
    {
    return this->Internal->Outputs[index];
    }
  return 0;
}

bool pqPipelineObject::HasOutput(pqPipelineObject *output) const
{
  if(this->Internal && output)
    {
    QList<pqPipelineObject *>::Iterator iter = this->Internal->Outputs.begin();
    for( ; iter != this->Internal->Outputs.end(); ++iter)
      {
      if(*iter == output)
        {
        return true;
        }
      }
    }

  return false;
}

void pqPipelineObject::AddOutput(pqPipelineObject *output)
{
  if(this->Internal && output)
    {
    this->Internal->Outputs.append(output);
    }
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


