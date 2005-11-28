
#include "pqPipelineServer.h"

#include "pqPipelineObject.h"
#include <QHash>
#include <QList>


class pqPipelineServerInternal
{
public:
  pqPipelineServerInternal();
  ~pqPipelineServerInternal() {}

  QList<pqPipelineObject *> Sources;
  QList<pqPipelineObject *> Windows;
  QHash<vtkSMProxy *, pqPipelineObject *> Objects;
};


pqPipelineServerInternal::pqPipelineServerInternal()
  : Sources(), Windows(), Objects()
{
}


pqPipelineServer::pqPipelineServer()
{
  this->Internal = new pqPipelineServerInternal();
  this->Server = 0;
}

pqPipelineServer::~pqPipelineServer()
{
  this->ClearPipelines();
  if(this->Internal)
    delete this->Internal;
}

pqPipelineObject *pqPipelineServer::AddSource(vtkSMProxy *source)
{
  pqPipelineObject *object = 0;
  if(this->Internal && source)
    {
    object = new pqPipelineObject(source, pqPipelineObject::Source);
    if(object)
      {
      this->Internal->Objects.insert(source, object);
      this->Internal->Sources.append(object);
      }
    }

  return object;
}

pqPipelineObject *pqPipelineServer::AddFilter(vtkSMProxy *filter)
{
  pqPipelineObject *object = 0;
  if(this->Internal && filter)
    {
    object = new pqPipelineObject(filter, pqPipelineObject::Filter);
    if(object)
      this->Internal->Objects.insert(filter, object);
    }

  return object;
}

pqPipelineObject *pqPipelineServer::AddWindow(QWidget *window)
{
  pqPipelineObject *object = 0;
  if(this->Internal && window)
    {
    object = new pqPipelineObject(window);
    if(object)
      this->Internal->Windows.append(object);
    }

  return object;
}

pqPipelineObject *pqPipelineServer::GetObject(vtkSMProxy *proxy) const
{
  if(this->Internal)
    {
    QHash<vtkSMProxy *, pqPipelineObject *>::Iterator iter =
        this->Internal->Objects.find(proxy);
    if(iter != this->Internal->Objects.end())
      return *iter;
    }

  return 0;
}

pqPipelineObject *pqPipelineServer::GetWindow(QWidget *window) const
{
  if(this->Internal)
    {
    QList<pqPipelineObject *>::Iterator iter = this->Internal->Windows.begin();
    for( ; iter != this->Internal->Windows.end(); ++iter)
      {
      if(*iter && (*iter)->GetWidget() == window)
        return *iter;
      }
    }

  return 0;
}

bool pqPipelineServer::RemoveObject(vtkSMProxy *proxy)
{
  if(this->Internal && proxy)
    {
    QHash<vtkSMProxy *, pqPipelineObject *>::Iterator iter =
        this->Internal->Objects.find(proxy);
    if(iter != this->Internal->Objects.end())
      {
      pqPipelineObject *object = *iter;
      this->Internal->Objects.erase(iter);
      if(object && object->GetType() == pqPipelineObject::Source)
        {
        QList<pqPipelineObject *>::Iterator jter =
            this->Internal->Sources.begin();
        for( ; jter != this->Internal->Sources.end(); ++jter)
          {
          if(*jter && (*jter)->GetProxy() == proxy)
            {
            this->Internal->Sources.erase(jter);
            break;
            }
          }
        }

      return true;
      }
    }

  return false;
}

bool pqPipelineServer::RemoveWindow(QWidget *window)
{
  if(this->Internal && window)
    {
    QList<pqPipelineObject *>::Iterator iter = this->Internal->Windows.begin();
    for( ; iter != this->Internal->Windows.end(); ++iter)
      {
      if(*iter && (*iter)->GetWidget() == window)
        {
        delete *iter;
        this->Internal->Windows.erase(iter);
        return true;
        }
      }
    }

  return false;
}

int pqPipelineServer::GetSourceCount() const
{
  if(this->Internal)
    return this->Internal->Sources.size();
  return 0;
}

pqPipelineObject *pqPipelineServer::GetSource(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Sources.size())
    return this->Internal->Sources[index];
  return 0;
}

int pqPipelineServer::GetWindowCount() const
{
  if(this->Internal)
    return this->Internal->Windows.size();
  return 0;
}

pqPipelineObject *pqPipelineServer::GetWindow(int index) const
{
  if(this->Internal && index >= 0 && index < this->Internal->Windows.size())
    return this->Internal->Windows[index];
  return 0;
}

void pqPipelineServer::ClearPipelines()
{
  if(this->Internal)
    {
    // Clean up the objects in the map.
    QHash<vtkSMProxy *, pqPipelineObject *>::Iterator iter =
        this->Internal->Objects.begin();
    for( ; iter != this->Internal->Objects.end(); ++iter)
      {
      if(*iter)
        {
        delete *iter;
        *iter = 0;
        }
      }

    // Clean up the window objects.
    QList<pqPipelineObject *>::Iterator jter = this->Internal->Windows.begin();
    for( ; jter != this->Internal->Windows.end(); ++jter)
      {
      if(*jter)
        {
        delete *jter;
        *jter = 0;
        }
      }

    this->Internal->Sources.clear();
    this->Internal->Windows.clear();
    this->Internal->Objects.clear();
    }
}


