/*=========================================================================

  Program:   ParaView
  Module:    vtkSubsetInclusionLattice.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSubsetInclusionLattice.h"

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <vtk_pugixml.h>
#include <vtksys/SystemTools.hxx>

//=============================================================================
/**
 * Namespace of various pugi::xml_tree_walker subclasses
 */
//=============================================================================
namespace Walkers
{
/**
 * This is a xml_tree_walker that walks the subtree setting
 * `state` for every node it visits.
 *
 * If the node is `Node`, this simply updates the `state` attribute.
 * If the node is `ref:link`, it calls `vtkInternals::SetSelectionState` on
 * the linked node.
 * If the node is `ref:rev-link`, it calls `vtkInternals::UpdateState` on the
 * linked node.
 */
template <class T>
class SetState : public pugi::xml_tree_walker
{
  T* Parent;
  vtkSubsetInclusionLattice::SelectionStates State;

public:
  SetState(T* parent, vtkSubsetInclusionLattice::SelectionStates state)
    : Parent(parent)
    , State(state)
  {
  }

  bool for_each(pugi::xml_node& node) override
  {
    if (strcmp(node.name(), "Node") == 0)
    {
      auto attr = node.attribute("state");
      if (attr && attr.as_int() != this->State)
      {
        attr.set_value(this->State);
        this->Parent->TriggerSelectionChanged(node.attribute("uid").as_int());
      }
    }
    else if (strcmp(node.name(), "ref:link") == 0)
    {
      this->Parent->SetSelectionState(
        node.attribute("uid").as_int(), this->State == vtkSubsetInclusionLattice::Selected, false);
    }
    else if (strcmp(node.name(), "ref:rev-link") == 0)
    {
      this->Parent->UpdateState(node.attribute("uid").as_int());
    }
    return true;
  }

  void operator=(const SetState&) = delete;
};

/**
 * A tree walker that walks the subtree clearing the "explicit_state" attribute.
 */
class ClearExplicit : public pugi::xml_tree_walker
{
public:
  bool for_each(pugi::xml_node& node) override
  {
    node.remove_attribute("explicit_state");
    return true;
  }
  void operator=(const ClearExplicit&) = delete;
};

/**
 * A walker to build a selection-path map comprising of status for all leaf nodes.
 *
 * @section DefineSelection How to define selection
 *
 * I have not been able to decide which is the best way to define the selection.
 * Is the user interested in preserving the state for a non-leaf node he toggled
 * or the resolved leaves? If former, we need to preserve the order in which the
 * operations happen too. For now, I am leaving towards the easiest i.e. the
 * leaf node status.
 */
class SerializeSelection : public pugi::xml_tree_walker
{
  std::map<std::string, bool>& Selection;

  std::string path(const pugi::xml_node node) const
  {
    if (!node)
    {
      return std::string();
    }
    assert(strcmp(node.name(), "Node") == 0);
    if (strcmp(node.attribute("name").value(), "SIL") == 0)
    {
      return std::string();
    }
    return this->path(node.parent()) + "/" + node.attribute("name").value();
  }

public:
  SerializeSelection(std::map<std::string, bool>& sel)
    : Selection(sel)
  {
  }
  bool for_each(pugi::xml_node& node) override
  {
    if (node && strcmp(node.name(), "Node") == 0 && !node.child("Node") && !node.child("ref:link"))
    {
      this->Selection[this->path(node)] =
        node.attribute("state").as_int() == vtkSubsetInclusionLattice::Selected;
    }
    return true;
  }
  void operator=(const SerializeSelection&) = delete;
};

/**
 * A walker that walks the tree and offsets all uids by a fixed amount.
 */
class OffsetUid : public pugi::xml_tree_walker
{
  int Offset;

public:
  OffsetUid(int offset)
    : Offset(offset)
  {
  }
  bool for_each(pugi::xml_node& node) override
  {
    auto attr = node.attribute("uid");
    attr.set_value(this->Offset + attr.as_int());
    return true;
  }
  void operator=(const OffsetUid&) = delete;
};

/**
 * Given a map of old ids to new ids, updates all uid references in the tree
 * that use the old ids to use the new ids instead.
 */
class UpdateIds : public pugi::xml_tree_walker
{
  const std::map<int, int>& Map;

public:
  UpdateIds(const std::map<int, int>& amap)
    : Map(amap)
  {
  }
  bool for_each(pugi::xml_node& node) override
  {
    auto attr = node.attribute("uid");
    auto iter = this->Map.find(attr.as_int());
    if (iter != this->Map.end())
    {
      attr.set_value(iter->second);
    }
    return true;
  }
  void operator=(const UpdateIds&) = delete;
};

/**
 * A walker that cleans up uids in the tree by reassigning
 * the ids for all nodes. Useful to keep the uids from increasing
 * too much as trees are merged, for example.
 */
class ReassignUids : public pugi::xml_tree_walker
{
  int& NextUID;
  std::map<int, int> IdsMap;

public:
  ReassignUids(int& nextuid)
    : NextUID(nextuid)
  {
  }
  bool for_each(pugi::xml_node& node) override
  {
    auto attr = node.attribute("uid");
    auto iter = this->IdsMap.find(attr.as_int());
    if (attr && iter == this->IdsMap.end())
    {
      const int newid = this->NextUID++;
      this->IdsMap[attr.as_int()] = newid;
      attr.set_value(newid);
    }
    else if (attr)
    {
      attr.set_value(iter->second);
    }
    return true;
  }
  void operator=(const ReassignUids&) = delete;
};
}

//=============================================================================
class vtkSubsetInclusionLattice::vtkInternals
{
  int NextUID;
  pugi::xml_document Document;
  vtkSubsetInclusionLattice* Parent;

public:
  vtkInternals(vtkSubsetInclusionLattice* self)
    : NextUID(0)
    , Parent(self)
  {
    this->Initialize();
  }

  void Initialize()
  {
    this->Document.load_string("<Node name='SIL' version='1.0' uid='0' next_uid='1' state='0' />");
    this->NextUID = 1;
  }

  std::string Serialize() const
  {
    std::ostringstream str;
    this->Document.save(str);
    return str.str();
  }

  bool Deserialize(const char* data, vtkSubsetInclusionLattice* self)
  {
    pugi::xml_document document;
    if (!document.load_string(data))
    {
      // leave state untouched.
      return false;
    }
    if (document.child("Node"))
    {
      this->Document.reset(document);
      this->NextUID = this->Document.first_child().attribute("next_uid").as_int();
      self->Modified();
      return true;
    }
    return false;
  }

  vtkSubsetInclusionLattice::SelectionType GetSelection() const
  {
    vtkSubsetInclusionLattice::SelectionType selection;
    Walkers::SerializeSelection walker(selection);
    this->Document.first_child().traverse(walker);
    return selection;
  }

  pugi::xml_node Find(int uid) const
  {
    if (uid < 0)
    {
      return pugi::xml_node();
    }

    return this->Document.find_node([=](const pugi::xml_node& node) {
      return strcmp(node.name(), "Node") == 0 && node.attribute("uid").as_int() == uid;
    });
  }

  pugi::xml_node GetRoot() const { return this->Document.first_child(); }

  pugi::xml_node Find(const char* xpath) const { return this->Document.select_node(xpath).node(); }

  int GetNextUID()
  {
    int next = this->NextUID++;
    this->Document.first_child().attribute("next_uid").set_value(this->NextUID);
    return next;
  }

  void Print(ostream& os, vtkIndent indent)
  {
    std::ostringstream str;
    str << indent;
    this->Document.save(os, str.str().c_str());
  }

  bool SetSelectionState(int id, bool value, bool isexplicit = false)
  {
    auto node = this->Find(id);
    return node ? this->SetSelectionState(node, value, isexplicit) : false;
  }

  bool SetSelectionState(pugi::xml_node node, bool value, bool isexplicit = false)
  {
    if (!node)
    {
      return false;
    }

    const vtkSubsetInclusionLattice::SelectionStates state =
      value ? vtkSubsetInclusionLattice::Selected : vtkSubsetInclusionLattice::NotSelected;
    auto state_attr = node.attribute("state");
    // since node's state has been modified, ensure none of its children are
    // flagged as `explicit_state`.
    Walkers::ClearExplicit clearExplicit;
    node.traverse(clearExplicit);
    node.remove_attribute("explicit_state");
    if (isexplicit)
    {
      node.append_attribute("explicit_state").set_value(state);
    }

    if (state_attr.as_int() != state)
    {
      state_attr.set_value(state);

      // notify that the node's selection state has changed.
      this->Parent->TriggerSelectionChanged(node.attribute("uid").as_int());

      // navigate down the tree and update state for all children.
      // this will walk through `ref:link`.
      Walkers::SetState<vtkSubsetInclusionLattice::vtkInternals> setState(this, state);
      node.traverse(setState);

      // navigate up and update state.
      this->UpdateState(node.parent());
      return true; // state changed.
    }
    return false; // state not changed.
  }

  void UpdateState(int id) { this->UpdateState(this->Find(id)); }
  void UpdateState(pugi::xml_node node)
  {
    while (node)
    {
      auto state = this->GetStateFromImmediateChildren(node);
      if (node.attribute("state").as_int() == state)
      {
        // nothing to do. changing node's state has no effect on node.
        break;
      }
      node.attribute("state").set_value(state);
      // notify that the node's selection state has changed.
      this->Parent->TriggerSelectionChanged(node.attribute("uid").as_int());
      node = node.parent();
    }
  }

  vtkSubsetInclusionLattice::SelectionStates GetSelectionState(int id) const
  {
    return this->GetSelectionState(this->Find(id));
  }
  vtkSubsetInclusionLattice::SelectionStates GetSelectionState(const pugi::xml_node node) const
  {
    switch (node.attribute("state").as_int())
    {
      case vtkSubsetInclusionLattice::Selected:
        return vtkSubsetInclusionLattice::Selected;
      case vtkSubsetInclusionLattice::PartiallySelected:
        return vtkSubsetInclusionLattice::PartiallySelected;
      case vtkSubsetInclusionLattice::NotSelected:
      default:
        return vtkSubsetInclusionLattice::NotSelected;
    }
  }

  static std::string ConvertXPath(const std::string& simplePath)
  {
    bool startsWithSep;
    auto parts = vtkInternals::SplitString(simplePath, startsWithSep);
    std::ostringstream stream;
    stream << "/Node[@name='SIL']";
    for (size_t cc = 0; cc < parts.size(); ++cc)
    {
      if (parts[cc].size() == 0)
      {
        stream << "/";
      }
      else
      {
        stream << "/Node[@name='" << parts[cc].c_str() << "']";
      }
    }
    return stream.str();
  }

  void Merge(const std::string& state)
  {
    pugi::xml_document other;
    if (!other.load_string(state.c_str()))
    {
      return;
    }

    // make all uid in `other` unique. this is necessary to avoid conflicts with
    // ids in `this`.
    Walkers::OffsetUid walker(this->NextUID);
    other.traverse(walker);

    // key: id in `other`, value: id in `this`.
    std::map<int, int> merged_ids;
    this->MergeNodes(merged_ids, this->Document.first_child(), other.first_child());

    // now iterate and update all merged ids.
    Walkers::UpdateIds walker2(merged_ids);
    this->Document.traverse(walker2);

    // now merge all duplicate "ref:link" and "ref:rev-link" nodes.
    this->CleanDuplicateLinks(this->Document.first_child());

    // now let's cleanup uids.
    this->NextUID = 0;
    Walkers::ReassignUids walker3(this->NextUID);
    this->Document.traverse(walker3);
    this->Document.first_child().attribute("next_uid").set_value(this->NextUID);
  }

  void TriggerSelectionChanged(int id)
  {
    if (id >= 0)
    {
      this->Parent->TriggerSelectionChanged(id);
    }
  }

private:
  int GetState(const pugi::xml_node node) const
  {
    if (node && strcmp(node.name(), "Node") == 0)
    {
      return node.attribute("state").as_int();
    }
    else if (node && strcmp(node.name(), "ref:link") == 0)
    {
      return this->GetState(this->Find(node.attribute("uid").as_int()));
    }
    return vtkSubsetInclusionLattice::NotSelected;
  }

  vtkSubsetInclusionLattice::SelectionStates GetStateFromImmediateChildren(
    const pugi::xml_node node) const
  {
    using vtkSIL = vtkSubsetInclusionLattice;
    int selected_count = 0;
    int notselected_count = 0;
    int children_count = 0;
    for (auto child : node.children())
    {
      int child_state;
      if (strcmp(child.name(), "Node") == 0 || strcmp(child.name(), "ref:link") == 0)
      {
        children_count++;
        child_state = this->GetState(child);
        switch (child_state)
        {
          case vtkSIL::NotSelected:
            notselected_count++;
            break;

          case vtkSIL::Selected:
            selected_count++;
            break;

          case vtkSIL::PartiallySelected:
            return vtkSIL::PartiallySelected;
        }
        if (selected_count > 0 && notselected_count > 0)
        {
          return vtkSIL::PartiallySelected;
        }
      }
    }
    if (selected_count == children_count)
    {
      return vtkSIL::Selected;
    }
    else if (notselected_count == children_count)
    {
      return vtkSIL::NotSelected;
    }
    return vtkSIL::PartiallySelected;
  }

  void MergeNodes(
    std::map<int, int>& merged_ids, pugi::xml_node self, const pugi::xml_node other) const
  {
    merged_ids[other.attribute("uid").as_int()] = self.attribute("uid").as_int();

    std::map<std::string, pugi::xml_node> selfChildrenMap;
    for (auto node : self.children("Node"))
    {
      selfChildrenMap[node.attribute("name").value()] = node;
    }

    for (auto onode : other.children("Node"))
    {
      std::string name = onode.attribute("name").value();
      auto iter = selfChildrenMap.find(name);
      if (iter != selfChildrenMap.end())
      {
        this->MergeNodes(merged_ids, iter->second, onode);
      }
      else
      {
        self.append_copy(onode);
      }
    }

    // links, we just append. we'll squash duplicates later.
    for (auto onode : other.children("ref:link"))
    {
      self.append_copy(onode);
    }

    for (auto onode : other.children("ref:rev-link"))
    {
      self.append_copy(onode);
    }
  }

  void CleanDuplicateLinks(pugi::xml_node self) const
  {
    for (auto node : self.children("Node"))
    {
      this->CleanDuplicateLinks(node);
    }

    std::set<int> seen_ids;
    std::vector<pugi::xml_node> to_remove;
    for (auto node : self.children("ref:link"))
    {
      int uid = node.attribute("uid").as_int();
      if (seen_ids.find(uid) != seen_ids.end())
      {
        to_remove.push_back(node);
      }
      seen_ids.insert(uid);
    }

    seen_ids.clear();
    for (auto node : self.children("ref:rev-link"))
    {
      int uid = node.attribute("uid").as_int();
      if (seen_ids.find(uid) != seen_ids.end())
      {
        to_remove.push_back(node);
      }
      seen_ids.insert(uid);
    }

    for (auto node : to_remove)
    {
      self.remove_child(node);
    }
  }

  // We don't use vtksys::SystemTools since it doesn't handle "//foo" correctly.
  static std::vector<std::string> SplitString(const std::string& str, bool& startsWithSep)
  {
    std::vector<std::string> ret;
    size_t pos = 0, posPrev = 0;
    while ((pos = str.find('/', posPrev)) != std::string::npos)
    {
      if (pos == 0)
      {
        startsWithSep = true;
      }
      else
      {
        size_t len = pos - posPrev;
        if (len > 0)
        {
          ret.push_back(str.substr(posPrev, len));
        }
        else
        {
          ret.push_back(std::string());
        }
      }
      posPrev = pos + 1;
    }
    if (posPrev < str.size())
    {
      ret.push_back(str.substr(posPrev));
    }
    return ret;
  }
};

vtkStandardNewMacro(vtkSubsetInclusionLattice);
vtkInformationKeyMacro(vtkSubsetInclusionLattice, SUBSET_INCLUSION_LATTICE, ObjectBase);
//----------------------------------------------------------------------------
vtkSubsetInclusionLattice::vtkSubsetInclusionLattice()
  : Internals(new vtkSubsetInclusionLattice::vtkInternals(this))
{
}

//----------------------------------------------------------------------------
vtkSubsetInclusionLattice::~vtkSubsetInclusionLattice()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkSubsetInclusionLattice::Initialize()
{
  vtkInternals& internals = (*this->Internals);
  internals.Initialize();
  this->Modified();
}

//----------------------------------------------------------------------------
std::string vtkSubsetInclusionLattice::Serialize() const
{
  const vtkInternals& internals = (*this->Internals);
  return internals.Serialize();
}

//----------------------------------------------------------------------------
bool vtkSubsetInclusionLattice::Deserialize(const std::string& data)
{
  vtkInternals& internals = (*this->Internals);
  return internals.Deserialize(data.c_str(), this);
}

//----------------------------------------------------------------------------
bool vtkSubsetInclusionLattice::Deserialize(const char* data)
{
  vtkInternals& internals = (*this->Internals);
  return internals.Deserialize(data, this);
}

//----------------------------------------------------------------------------
int vtkSubsetInclusionLattice::AddNode(const char* name, int parent)
{
  vtkInternals& internals = (*this->Internals);
  auto node = internals.Find(parent);
  if (!node)
  {
    vtkErrorMacro("Invalid `parent` specified: " << parent);
    return -1;
  }

  auto child = node.append_child("Node");
  int uid = internals.GetNextUID();
  child.append_attribute("name").set_value(name);
  child.append_attribute("uid").set_value(uid);
  child.append_attribute("state").set_value(0);
  this->Modified();
  return uid;
}

//----------------------------------------------------------------------------
int vtkSubsetInclusionLattice::AddNodeAtPath(const char* path)
{
  if (path == nullptr || path[0] == 0)
  {
    return -1;
  }

  const int id = this->FindNode(path);
  if (id != -1)
  {
    // path already exists.
    return id;
  }

  std::string spath(path);

  // confirm that path is full-qualified.
  // i.e. starts with `/` and no `//`.
  if (spath[0] != '/' || spath.find("//") != std::string::npos)
  {
    return -1;
  }

  bool modified = false;

  vtkInternals& internals = (*this->Internals);
  auto node = internals.Find(static_cast<int>(0));

  // iterate component-by-component and then add nodes as needed.
  spath.erase(0, 1); // remove leading '/'.
  const auto components = vtksys::SystemTools::SplitString(spath);
  for (const std::string& part : components)
  {
    pugi::xml_node nchild;
    for (auto child : node.children("Node"))
    {
      auto attr = child.attribute("name");
      if (attr && part == attr.value())
      {
        nchild = child;
        break;
      }
    }
    if (!nchild)
    {
      nchild = node.append_child("Node");
      nchild.append_attribute("name").set_value(part.c_str());
      nchild.append_attribute("uid").set_value(internals.GetNextUID());
      nchild.append_attribute("state").set_value(0);
      modified = true;
    }
    node = nchild;
  }

  if (modified)
  {
    this->Modified();
  }

  return node.attribute("uid").as_int();
}

//----------------------------------------------------------------------------
bool vtkSubsetInclusionLattice::AddCrossLink(int src, int dst)
{
  vtkInternals& internals = (*this->Internals);
  auto srcnode = internals.Find(src);
  if (!srcnode)
  {
    vtkErrorMacro("Invalid `src` specified: " << src);
    return false;
  }

  auto dstnode = internals.Find(dst);
  if (!dstnode)
  {
    vtkErrorMacro("Invalid `dst` specified: " << dst);
    return false;
  }

  auto ref = srcnode.append_child("ref:link");
  ref.append_attribute("uid").set_value(dst);

  auto rref = dstnode.append_child("ref:rev-link");
  rref.append_attribute("uid").set_value(src);

  this->Modified();
  return true;
}

//----------------------------------------------------------------------------
int vtkSubsetInclusionLattice::FindNode(const char* spath) const
{
  if (!spath)
  {
    return -1;
  }

  std::string convertedxpath = vtkInternals::ConvertXPath(spath);
  const vtkInternals& internals = (*this->Internals);
  auto srcnode = internals.Find(convertedxpath.c_str());
  if (!srcnode)
  {
    return -1;
  }
  return srcnode.attribute("uid").as_int();
}

//----------------------------------------------------------------------------
bool vtkSubsetInclusionLattice::Select(int node)
{
  vtkInternals& internals = (*this->Internals);
  return internals.SetSelectionState(node, true, true);
}

//----------------------------------------------------------------------------
bool vtkSubsetInclusionLattice::Deselect(int node)
{
  vtkInternals& internals = (*this->Internals);
  return internals.SetSelectionState(node, false, true);
}

//----------------------------------------------------------------------------
void vtkSubsetInclusionLattice::ClearSelections()
{
  vtkInternals& internals = (*this->Internals);
  internals.SetSelectionState(0, false, false);
}

//----------------------------------------------------------------------------
vtkSubsetInclusionLattice::SelectionStates vtkSubsetInclusionLattice::GetSelectionState(
  int node) const
{
  const vtkInternals& internals = (*this->Internals);
  return node >= 0 ? internals.GetSelectionState(node) : NotSelected;
}

//----------------------------------------------------------------------------
bool vtkSubsetInclusionLattice::Select(const char* path)
{
  // `AddNodeAtPath` doesn't add a new node if one already exists.
  return this->Select(this->AddNodeAtPath(path));
}

//----------------------------------------------------------------------------
bool vtkSubsetInclusionLattice::Deselect(const char* path)
{
  // `AddNodeAtPath` doesn't add a new node if one already exists.
  return this->Deselect(this->AddNodeAtPath(path));
}

//----------------------------------------------------------------------------
bool vtkSubsetInclusionLattice::SelectAll(const char* spath)
{
  std::string convertedxpath = vtkInternals::ConvertXPath(spath);

  vtkInternals& internals = (*this->Internals);
  auto root = internals.GetRoot();
  bool retval = false;
  for (auto xnode : root.select_nodes(convertedxpath.c_str()))
  {
    retval = internals.SetSelectionState(xnode.node(), true, true) || retval;
  }
  return retval;
}

//----------------------------------------------------------------------------
bool vtkSubsetInclusionLattice::DeselectAll(const char* spath)
{
  std::string convertedxpath = vtkInternals::ConvertXPath(spath);

  vtkInternals& internals = (*this->Internals);
  auto root = internals.GetRoot();
  bool retval = false;
  for (auto xnode : root.select_nodes(convertedxpath.c_str()))
  {
    retval = internals.SetSelectionState(xnode.node(), false, true) || retval;
  }
  return retval;
}

//----------------------------------------------------------------------------
std::vector<int> vtkSubsetInclusionLattice::GetChildren(int node) const
{
  std::vector<int> retval;

  vtkInternals& internals = (*this->Internals);
  auto xmlNode = internals.Find(node);
  for (auto child : xmlNode.children("Node"))
  {
    retval.push_back(child.attribute("uid").as_int());
  }

  return retval;
}

//----------------------------------------------------------------------------
int vtkSubsetInclusionLattice::GetParent(int node, int* childIndex) const
{
  int retval = -1;
  if (childIndex)
  {
    *childIndex = -1;
  }

  vtkInternals& internals = (*this->Internals);
  if (auto xmlNode = internals.Find(node))
  {
    if (auto parentNode = xmlNode.parent())
    {
      retval = parentNode.attribute("uid").as_int();
      if (childIndex)
      {
        *childIndex = 0;
        for (auto child : parentNode.children("Node"))
        {
          if (child == xmlNode)
          {
            break;
          }
          ++(*childIndex);
        }
      }
    }
  }

  return retval;
}

//----------------------------------------------------------------------------
const char* vtkSubsetInclusionLattice::GetNodeName(int node) const
{
  vtkInternals& internals = (*this->Internals);
  if (auto xmlNode = internals.Find(node))
  {
    return xmlNode.attribute("name").value();
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkSubsetInclusionLattice::SelectionType vtkSubsetInclusionLattice::GetSelection() const
{
  return this->Internals->GetSelection();
}

//----------------------------------------------------------------------------
void vtkSubsetInclusionLattice::SetSelection(
  const vtkSubsetInclusionLattice::SelectionType& selection)
{
  vtkInternals& internals = (*this->Internals);
  internals.SetSelectionState(0, NotSelected, /*explicit=*/false);
  for (auto s : selection)
  {
    if (s.second)
    {
      this->Select(s.first.c_str());
    }
    else
    {
      this->Deselect(s.first.c_str());
    }
  }
}

//----------------------------------------------------------------------------
void vtkSubsetInclusionLattice::TriggerSelectionChanged(int node)
{
  this->InvokeEvent(vtkCommand::StateChangedEvent, &node);
  this->SelectionChangeTime.Modified();
}

//----------------------------------------------------------------------------
void vtkSubsetInclusionLattice::Modified()
{
  this->Superclass::Modified();
  this->SelectionChangeTime.Modified();
}

//----------------------------------------------------------------------------
void vtkSubsetInclusionLattice::DeepCopy(const vtkSubsetInclusionLattice* other)
{
  if (other)
  {
    this->Deserialize(other->Serialize());
  }
  else
  {
    this->Initialize();
  }
}

//----------------------------------------------------------------------------
void vtkSubsetInclusionLattice::Merge(const std::string& other)
{
  this->Internals->Merge(other);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSubsetInclusionLattice::Merge(const vtkSubsetInclusionLattice* other)
{
  this->Internals->Merge(other->Serialize());
}

//----------------------------------------------------------------------------
vtkSubsetInclusionLattice* vtkSubsetInclusionLattice::GetSIL(vtkInformation* info)
{
  return info ? vtkSubsetInclusionLattice::SafeDownCast(info->Get(SUBSET_INCLUSION_LATTICE()))
              : nullptr;
}

//----------------------------------------------------------------------------
vtkSubsetInclusionLattice* vtkSubsetInclusionLattice::GetSIL(vtkInformationVector* v, int i)
{
  return vtkSubsetInclusionLattice::GetSIL(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkSubsetInclusionLattice> vtkSubsetInclusionLattice::Clone(
  const vtkSubsetInclusionLattice* other)
{
  if (other)
  {
    vtkSmartPointer<vtkSubsetInclusionLattice> clone;
    clone.TakeReference(other->NewInstance());
    clone->DeepCopy(other);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSubsetInclusionLattice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkInternals& internals = (*this->Internals);
  internals.Print(os, vtkIndent().GetNextIndent());
}
