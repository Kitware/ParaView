/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeAnimationCue.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVComparativeAnimationCue.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMVectorProperty.h"

#include <sstream>
#include <string.h>
#include <string>
#include <vector>
#include <vtksys/SystemTools.hxx>

class vtkPVComparativeAnimationCue::vtkInternals
{
public:
  enum
  {
    SINGLE,
    XRANGE,
    YRANGE,
    TRANGE,
    TRANGE_VERTICAL_FIRST
  };

  // vtkCueCommand is a unit of computation used to compute the value at a given
  // position in the comparative grid.
  class vtkCueCommand
  {
  private:
    std::string ValuesToString(double* values)
    {
      std::ostringstream str;
      for (unsigned int cc = 0; cc < this->NumberOfValues; cc++)
      {
        str << setprecision(16) << values[cc];
        if (cc > 0)
        {
          str << ",";
        }
      }
      return str.str();
    }

    // don't call this if this->NumberOfValues == 1.
    double* ValuesFromString(const char* str)
    {
      double* values = nullptr;
      if (str != nullptr && *str != 0)
      {
        std::vector<std::string> parts = vtksys::SystemTools::SplitString(str, ',');
        if (static_cast<unsigned int>(parts.size()) == this->NumberOfValues)
        {
          values = new double[this->NumberOfValues];
          for (unsigned int cc = 0; cc < this->NumberOfValues; cc++)
          {
            values[cc] = atof(parts[cc].c_str());
          }
        }
      }
      return values;
    }

    void Duplicate(const vtkCueCommand& other)
    {
      this->Type = other.Type;
      this->AnchorX = other.AnchorX;
      this->AnchorY = other.AnchorY;
      this->NumberOfValues = other.NumberOfValues;
      this->MinValues = this->MaxValues = nullptr;
      if (this->NumberOfValues > 0)
      {
        this->MinValues = new double[this->NumberOfValues];
        memcpy(this->MinValues, other.MinValues, sizeof(double) * this->NumberOfValues);

        this->MaxValues = new double[this->NumberOfValues];
        memcpy(this->MaxValues, other.MaxValues, sizeof(double) * this->NumberOfValues);
      }
    }

  public:
    int Type;
    double* MinValues;
    double* MaxValues;
    unsigned int NumberOfValues;
    int AnchorX, AnchorY;
    vtkCueCommand()
    {
      this->Type = -1;
      this->AnchorX = this->AnchorY = -1;
      this->NumberOfValues = 0;
      this->MaxValues = nullptr;
      this->MinValues = nullptr;
    }

    vtkCueCommand(const vtkCueCommand& other) { this->Duplicate(other); }

    vtkCueCommand& operator=(const vtkCueCommand& other)
    {
      delete[] this->MinValues;
      delete[] this->MaxValues;
      this->Duplicate(other);
      return *this;
    }

    ~vtkCueCommand()
    {
      delete[] this->MinValues;
      this->MinValues = nullptr;

      delete[] this->MaxValues;
      this->MaxValues = nullptr;
    }

    void SetValues(double* minValues, double* maxValues, unsigned int num)
    {
      delete[] this->MaxValues;
      delete[] this->MinValues;
      this->MaxValues = nullptr;
      this->MinValues = nullptr;
      this->NumberOfValues = num;
      if (num > 0)
      {
        this->MinValues = new double[num];
        this->MaxValues = new double[num];
        memcpy(this->MinValues, minValues, num * sizeof(double));
        memcpy(this->MaxValues, maxValues, num * sizeof(double));
      }
    }

    bool operator==(const vtkCueCommand& other)
    {
      return this->Type == other.Type && this->NumberOfValues == other.NumberOfValues &&
        this->AnchorX == other.AnchorX && this->AnchorY == other.AnchorY &&
        (memcmp(this->MinValues, other.MinValues, sizeof(double) * this->NumberOfValues) == 0) &&
        (memcmp(this->MaxValues, other.MaxValues, sizeof(double) * this->NumberOfValues) == 0);
    }

    vtkPVXMLElement* ToXML()
    {
      vtkPVXMLElement* elem = vtkPVXMLElement::New();
      elem->SetName("CueCommand");
      elem->AddAttribute("type", this->Type);
      elem->AddAttribute("anchorX", this->AnchorX);
      elem->AddAttribute("anchorY", this->AnchorY);
      elem->AddAttribute("num_values", this->NumberOfValues);
      elem->AddAttribute("min_values", this->ValuesToString(this->MinValues).c_str());
      elem->AddAttribute("max_values", this->ValuesToString(this->MaxValues).c_str());
      return elem;
    }

    bool FromXML(vtkPVXMLElement* elem)
    {
      if (!elem->GetName() || strcmp(elem->GetName(), "CueCommand") != 0)
      {
        return false;
      }
      int numvalues = 0;
      if (elem->GetScalarAttribute("type", &this->Type) &&
        elem->GetScalarAttribute("anchorX", &this->AnchorX) &&
        elem->GetScalarAttribute("anchorY", &this->AnchorY) &&
        elem->GetScalarAttribute("num_values", &numvalues))
      {
        this->NumberOfValues = static_cast<unsigned int>(numvalues);
        if (this->NumberOfValues > 1)
        {
          delete[] this->MinValues;
          delete[] this->MaxValues;

          this->MinValues = this->ValuesFromString(elem->GetAttribute("min_values"));
          this->MaxValues = this->ValuesFromString(elem->GetAttribute("max_values"));
          return this->MaxValues != nullptr && this->MinValues != nullptr;
        }
        else
        {
          delete[] this->MinValues;
          this->MinValues = new double[1];
          this->MinValues[0] = 0;

          delete[] this->MaxValues;
          this->MaxValues = new double[1];
          this->MaxValues[0] = 0;

          return elem->GetScalarAttribute("min_values", this->MinValues) &&
            elem->GetScalarAttribute("max_values", this->MaxValues);
        }
      }
      return false;
    }
  };

  void RemoveCommand(const vtkCueCommand& cmd)
  {
    std::vector<vtkCueCommand>::iterator iter;
    for (iter = this->CommandQueue.begin(); iter != this->CommandQueue.end(); ++iter)
    {
      if (*iter == cmd)
      {
        this->CommandQueue.erase(iter);
        break;
      }
    }
  }

  void InsertCommand(const vtkCueCommand& cmd, int pos)
  {
    std::vector<vtkCueCommand>::iterator iter;
    int cc = 0;
    for (iter = this->CommandQueue.begin(); iter != this->CommandQueue.end() && cc < pos;
         ++iter, ++cc)
    {
    }
    this->CommandQueue.insert(iter, cmd);
  }

  std::vector<vtkCueCommand> CommandQueue;
};

vtkStandardNewMacro(vtkPVComparativeAnimationCue);
vtkCxxSetObjectMacro(vtkPVComparativeAnimationCue, AnimatedProxy, vtkSMProxy);
//----------------------------------------------------------------------------
vtkPVComparativeAnimationCue::vtkPVComparativeAnimationCue()
{
  this->Internals = new vtkInternals();
  this->Values = new double[128]; // some large limit.
  this->AnimatedProxy = nullptr;
  this->AnimatedPropertyName = nullptr;
  this->AnimatedDomainName = nullptr;
  this->AnimatedElement = 0;
  this->Enabled = true;
}

//----------------------------------------------------------------------------
vtkPVComparativeAnimationCue::~vtkPVComparativeAnimationCue()
{
  delete this->Internals;
  this->Internals = nullptr;
  delete[] this->Values;
  this->Values = nullptr;
  this->SetAnimatedProxy(nullptr);
  this->SetAnimatedPropertyName(nullptr);
  this->SetAnimatedDomainName(nullptr);
}

//----------------------------------------------------------------------------
void vtkPVComparativeAnimationCue::RemoveAnimatedProxy()
{
  this->SetAnimatedProxy(nullptr);
}
//----------------------------------------------------------------------------
vtkSMProperty* vtkPVComparativeAnimationCue::GetAnimatedProperty()
{
  if (!this->AnimatedPropertyName || !this->AnimatedProxy)
  {
    return nullptr;
  }

  return this->AnimatedProxy->GetProperty(this->AnimatedPropertyName);
}

//----------------------------------------------------------------------------
vtkSMDomain* vtkPVComparativeAnimationCue::GetAnimatedDomain()
{
  vtkSMProperty* property = this->GetAnimatedProperty();
  if (!property)
  {
    return nullptr;
  }
  vtkSMDomain* domain = nullptr;
  vtkSMDomainIterator* iter = property->NewDomainIterator();
  iter->Begin();
  if (!iter->IsAtEnd())
  {
    domain = iter->GetDomain();
  }
  iter->Delete();
  return domain;
}

//----------------------------------------------------------------------------
void vtkPVComparativeAnimationCue::UpdateXRange(
  int y, double* minx, double* maxx, unsigned int numValues)
{
  vtkInternals::vtkCueCommand cmd;
  cmd.Type = vtkInternals::XRANGE;
  cmd.AnchorX = -1;
  cmd.AnchorY = y;
  cmd.SetValues(minx, maxx, numValues);

  vtkPVXMLElement* changeXML = vtkPVXMLElement::New();
  changeXML->SetName("StateChange");

  int position = 0;
  // remove obsolete values.
  std::vector<vtkInternals::vtkCueCommand>::iterator iter;
  for (iter = this->Internals->CommandQueue.begin(); iter != this->Internals->CommandQueue.end();
       position++)
  {
    bool remove = false;
    if (iter->Type == vtkInternals::SINGLE && iter->AnchorY == y)
    {
      remove = true;
    }
    if (iter->Type == vtkInternals::XRANGE && iter->AnchorY == y)
    {
      remove = true;
    }
    if (remove)
    {
      vtkPVXMLElement* removeXML = iter->ToXML();
      removeXML->AddAttribute("position", position);
      removeXML->AddAttribute("remove", 1);
      changeXML->AddNestedElement(removeXML);
      removeXML->FastDelete();

      iter = this->Internals->CommandQueue.erase(iter);
    }
    else
    {
      iter++;
    }
  }

  this->Internals->CommandQueue.push_back(cmd);

  vtkPVXMLElement* addXML = cmd.ToXML();
  changeXML->AddNestedElement(addXML);
  addXML->FastDelete();
  this->InvokeEvent(vtkCommand::StateChangedEvent, changeXML);
  changeXML->Delete();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVComparativeAnimationCue::UpdateYRange(
  int x, double* miny, double* maxy, unsigned int numValues)
{
  vtkInternals::vtkCueCommand cmd;
  cmd.Type = vtkInternals::YRANGE;
  cmd.AnchorX = x;
  cmd.AnchorY = -1;
  cmd.SetValues(miny, maxy, numValues);

  vtkPVXMLElement* changeXML = vtkPVXMLElement::New();
  changeXML->SetName("StateChange");

  int position = 0;
  // remove obsolete values.
  std::vector<vtkInternals::vtkCueCommand>::iterator iter;
  for (iter = this->Internals->CommandQueue.begin(); iter != this->Internals->CommandQueue.end();
       position++)
  {
    bool remove = false;
    if (iter->Type == vtkInternals::SINGLE && iter->AnchorX == x)
    {
      remove = true;
    }
    if (iter->Type == vtkInternals::YRANGE && iter->AnchorX == x)
    {
      remove = true;
    }
    if (remove)
    {
      vtkPVXMLElement* removeXML = iter->ToXML();
      removeXML->AddAttribute("position", position);
      removeXML->AddAttribute("remove", 1);
      changeXML->AddNestedElement(removeXML);
      removeXML->FastDelete();

      iter = this->Internals->CommandQueue.erase(iter);
    }
    else
    {
      iter++;
    }
  }

  this->Internals->CommandQueue.push_back(cmd);

  vtkPVXMLElement* addXML = cmd.ToXML();
  changeXML->AddNestedElement(addXML);
  addXML->FastDelete();
  this->InvokeEvent(vtkCommand::StateChangedEvent, changeXML);
  changeXML->Delete();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVComparativeAnimationCue::UpdateWholeRange(
  double* mint, double* maxt, unsigned int numValues, bool vertical_first)
{
  vtkPVXMLElement* changeXML = vtkPVXMLElement::New();
  changeXML->SetName("StateChange");

  // remove all  values (we are just recording the state change here).
  int position = 0;
  std::vector<vtkInternals::vtkCueCommand>::iterator iter;
  for (iter = this->Internals->CommandQueue.begin(); iter != this->Internals->CommandQueue.end();
       ++position, ++iter)
  {
    vtkPVXMLElement* removeXML = iter->ToXML();
    removeXML->AddAttribute("position", position);
    removeXML->AddAttribute("remove", 1);
    changeXML->AddNestedElement(removeXML);
    removeXML->FastDelete();
  }

  this->Internals->CommandQueue.clear();
  vtkInternals::vtkCueCommand cmd;
  cmd.Type = vertical_first ? vtkInternals::TRANGE_VERTICAL_FIRST : vtkInternals::TRANGE;
  cmd.SetValues(mint, maxt, numValues);

  this->Internals->CommandQueue.push_back(cmd);

  vtkPVXMLElement* addXML = cmd.ToXML();
  changeXML->AddNestedElement(addXML);
  addXML->FastDelete();
  this->InvokeEvent(vtkCommand::StateChangedEvent, changeXML);
  changeXML->Delete();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVComparativeAnimationCue::UpdateValue(int x, int y, double* value, unsigned int numValues)
{
  vtkInternals::vtkCueCommand cmd;
  cmd.Type = vtkInternals::SINGLE;
  cmd.AnchorX = x;
  cmd.AnchorY = y;
  cmd.SetValues(value, value, numValues);

  vtkPVXMLElement* changeXML = vtkPVXMLElement::New();
  changeXML->SetName("StateChange");

  int position = 0;
  std::vector<vtkInternals::vtkCueCommand>::iterator iter;
  for (iter = this->Internals->CommandQueue.begin(); iter != this->Internals->CommandQueue.end();
       ++position)
  {
    bool remove = false;
    if (iter->Type == vtkInternals::SINGLE && iter->AnchorX == x && iter->AnchorY == y)
    {
      remove = true;
    }
    if (remove)
    {
      vtkPVXMLElement* removeXML = iter->ToXML();
      removeXML->AddAttribute("position", position);
      removeXML->AddAttribute("remove", 1);
      changeXML->AddNestedElement(removeXML);
      removeXML->FastDelete();

      iter = this->Internals->CommandQueue.erase(iter);
    }
    else
    {
      iter++;
    }
  }
  this->Internals->CommandQueue.push_back(cmd);

  vtkPVXMLElement* addXML = cmd.ToXML();
  changeXML->AddNestedElement(addXML);
  addXML->FastDelete();
  this->InvokeEvent(vtkCommand::StateChangedEvent, changeXML);
  changeXML->Delete();
  this->Modified();
}

//----------------------------------------------------------------------------
double* vtkPVComparativeAnimationCue::GetValues(
  int x, int y, int dx, int dy, unsigned int& numValues)
{
  numValues = 0;
  std::vector<vtkInternals::vtkCueCommand>::iterator iter;
  for (iter = this->Internals->CommandQueue.begin(); iter != this->Internals->CommandQueue.end();
       ++iter)
  {
    unsigned int count = iter->NumberOfValues > 128 ? 128 : iter->NumberOfValues;
    switch (iter->Type)
    {
      case vtkInternals::SINGLE:
        if (x == iter->AnchorX && y == iter->AnchorY)
        {
          memcpy(this->Values, iter->MinValues, sizeof(double) * count);
          numValues = count;
        }
        break;

      case vtkInternals::XRANGE:
        if (y == iter->AnchorY || iter->AnchorY == -1)
        {
          for (unsigned int cc = 0; cc < count; cc++)
          {
            this->Values[cc] = dx > 1
              ? iter->MinValues[cc] + (x * (iter->MaxValues[cc] - iter->MinValues[cc])) / (dx - 1)
              : iter->MinValues[cc];
          }
          numValues = count;
        }
        break;

      case vtkInternals::YRANGE:
        if (x == iter->AnchorX || iter->AnchorX == -1)
        {
          for (unsigned int cc = 0; cc < count; cc++)
          {
            this->Values[cc] = dy > 1
              ? iter->MinValues[cc] + (y * (iter->MaxValues[cc] - iter->MinValues[cc])) / (dy - 1)
              : iter->MinValues[cc];
          }
          numValues = count;
        }
        break;

      case vtkInternals::TRANGE:
      {
        for (unsigned int cc = 0; cc < count; cc++)
        {
          this->Values[cc] = (dx * dy > 1)
            ? iter->MinValues[cc] +
              (y * dx + x) * (iter->MaxValues[cc] - iter->MinValues[cc]) / (dx * dy - 1)
            : iter->MinValues[cc];
        }
        numValues = count;
      }
      break;

      case vtkInternals::TRANGE_VERTICAL_FIRST:
      {
        for (unsigned int cc = 0; cc < count; cc++)
        {
          this->Values[cc] = (dx * dy > 1)
            ? iter->MinValues[cc] +
              (x * dy + y) * (iter->MaxValues[cc] - iter->MinValues[cc]) / (dx * dy - 1)
            : iter->MinValues[cc];
        }
        numValues = count;
      }
      break;
    }
  }

  return numValues > 0 ? this->Values : nullptr;
}

//----------------------------------------------------------------------------
void vtkPVComparativeAnimationCue::UpdateAnimatedValue(int x, int y, int dx, int dy)
{
  if (!this->GetEnabled())
  {
    return;
  }

  vtkSMDomain* domain = this->GetAnimatedDomain();
  vtkSMProperty* property = this->GetAnimatedProperty();
  vtkSMProxy* proxy = this->GetAnimatedProxy();
  int animated_element = this->GetAnimatedElement();
  if (!proxy || !domain || !property)
  {
    vtkErrorMacro("Cue does not have domain or property set!");
    return;
  }

  unsigned int numValues = 0;
  double* values = this->GetValues(x, y, dx, dy, numValues);

  if (numValues == 0)
  {
    vtkErrorMacro("Failed to determine any value: " << x << ", " << y);
  }
  else if (numValues == 1 && animated_element >= 0)
  {
    domain->SetAnimationValue(property, animated_element, values[0]);
  }
  else if (numValues >= 1 && animated_element == -1)
  {
    vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(property);
    if (vp)
    {
      vp->SetNumberOfElements(numValues);
    }
    for (unsigned int cc = 0; cc < numValues; cc++)
    {
      domain->SetAnimationValue(property, cc, values[cc]);
    }
  }
  else
  {
    vtkErrorMacro("Failed to change parameter.");
  }
  proxy->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVComparativeAnimationCue::AppendCommandInfo(vtkPVXMLElement* proxyElem)
{
  if (!proxyElem)
  {
    return nullptr;
  }

  std::vector<vtkInternals::vtkCueCommand>::iterator iter;
  for (iter = this->Internals->CommandQueue.begin(); iter != this->Internals->CommandQueue.end();
       ++iter)
  {
    vtkPVXMLElement* commandElem = iter->ToXML();
    proxyElem->AddNestedElement(commandElem);
    commandElem->Delete();
  }
  return proxyElem;
}

//----------------------------------------------------------------------------
int vtkPVComparativeAnimationCue::LoadCommandInfo(vtkPVXMLElement* proxyElement)
{
  bool state_change_xml = (strcmp(proxyElement->GetName(), "StateChange") != 0);
  if (state_change_xml)
  {
    // unless the state being loaded is a StateChange, we start from scratch.
    this->Internals->CommandQueue.clear();
  }

  // NOTE: In case of state_change_xml,
  // this assumes that all "removes" happen before any inserts which are
  // always appends.
  unsigned int numElems = proxyElement->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = proxyElement->GetNestedElement(i);
    const char* name = currentElement->GetName();
    if (name && strcmp(name, "CueCommand") == 0)
    {
      vtkInternals::vtkCueCommand cmd;
      if (cmd.FromXML(currentElement) == false)
      {
        vtkErrorMacro("Error when loading CueCommand.");
        return 0;
      }
      int remove = 0;
      if (state_change_xml && currentElement->GetScalarAttribute("remove", &remove) && remove != 0)
      {
        this->Internals->RemoveCommand(cmd);
      }
      else
      {
        this->Internals->CommandQueue.push_back(cmd);
      }
    }
  }
  this->Modified();
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVComparativeAnimationCue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
