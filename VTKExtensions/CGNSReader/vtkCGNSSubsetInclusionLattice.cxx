/*=========================================================================

  Program:   ParaView
  Module:    vtkCGNSSubsetInclusionLattice.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCGNSSubsetInclusionLattice.h"

#include "vtkObjectFactory.h"
#include "vtksys/RegularExpression.hxx"

#include <sstream>

namespace
{

// we sanitize zone names to avoid complication with dealing with partitioned
// CGNS files where blocks in each file as suffixed with `_proc-(...)`.
static std::string SanitizeName(const char* basename)
{
  static vtksys::RegularExpression nameRe("^(.*)_proc-([0-9]+)$");
  if (basename && nameRe.find(basename))
  {
    return nameRe.match(1);
  }
  return basename;
}

static std::string GetPathForBaseWithName(const char* basename)
{
  std::ostringstream query;
  query << "/Hierarchy/" << basename;
  return query.str();
}

static std::string GetPathForZoneWithName(const char* basename, const char* zonename)
{
  std::ostringstream path;
  path << "/Hierarchy/" << basename << "/" << SanitizeName(zonename).c_str();
  return path.str();
}

static std::string GetPathForFamilyWithName(const char* familyname)
{
  std::ostringstream path;
  path << "/Families/" << familyname;
  return path.str();
}
}

vtkStandardNewMacro(vtkCGNSSubsetInclusionLattice);
//----------------------------------------------------------------------------
vtkCGNSSubsetInclusionLattice::vtkCGNSSubsetInclusionLattice()
{
}

//----------------------------------------------------------------------------
vtkCGNSSubsetInclusionLattice::~vtkCGNSSubsetInclusionLattice()
{
}

//----------------------------------------------------------------------------
void vtkCGNSSubsetInclusionLattice::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkCGNSSubsetInclusionLattice::SelectBase(const char* basename)
{
  this->Select(GetPathForBaseWithName(basename).c_str());
}

//----------------------------------------------------------------------------
void vtkCGNSSubsetInclusionLattice::DeselectBase(const char* basename)
{
  this->Deselect(GetPathForBaseWithName(basename).c_str());
}

//----------------------------------------------------------------------------
void vtkCGNSSubsetInclusionLattice::SelectAllBases()
{
  this->Select("/Hierarchy");
}

//----------------------------------------------------------------------------
void vtkCGNSSubsetInclusionLattice::DeselectAllBases()
{
  this->Deselect("/Hierarchy");
}

//----------------------------------------------------------------------------
vtkSubsetInclusionLattice::SelectionStates vtkCGNSSubsetInclusionLattice::GetBaseState(
  const char* basename) const
{
  return this->GetSelectionState(GetPathForBaseWithName(basename).c_str());
}

//----------------------------------------------------------------------------
int vtkCGNSSubsetInclusionLattice::GetNumberOfBases() const
{
  return static_cast<int>(this->GetChildren(this->FindNode("/Hierarchy")).size());
}

//----------------------------------------------------------------------------
const char* vtkCGNSSubsetInclusionLattice::GetBaseName(int index) const
{
  auto children = this->GetChildren(this->FindNode("/Hierarchy"));
  return (static_cast<int>(children.size()) > index) ? this->GetNodeName(children[index]) : nullptr;
}

//----------------------------------------------------------------------------
void vtkCGNSSubsetInclusionLattice::SelectFamily(const char* familyname)
{
  this->Select(GetPathForFamilyWithName(familyname).c_str());
}

//----------------------------------------------------------------------------
void vtkCGNSSubsetInclusionLattice::DeselectFamily(const char* familyname)
{
  this->Deselect(GetPathForFamilyWithName(familyname).c_str());
}

//----------------------------------------------------------------------------
void vtkCGNSSubsetInclusionLattice::SelectAllFamilies()
{
  this->Select("/Families");
}

//----------------------------------------------------------------------------
void vtkCGNSSubsetInclusionLattice::DeselectAllFamilies()
{
  this->Deselect("/Families");
}

//----------------------------------------------------------------------------
int vtkCGNSSubsetInclusionLattice::GetNumberOfFamilies() const
{
  return static_cast<int>(this->GetChildren(this->FindNode("/Families")).size());
}

//----------------------------------------------------------------------------
const char* vtkCGNSSubsetInclusionLattice::GetFamilyName(int index) const
{
  auto children = this->GetChildren(this->FindNode("/Families"));
  return (static_cast<int>(children.size()) > index) ? this->GetNodeName(children[index]) : nullptr;
}

//----------------------------------------------------------------------------
vtkSubsetInclusionLattice::SelectionStates vtkCGNSSubsetInclusionLattice::GetFamilyState(
  const char* familyname) const
{
  return this->GetSelectionState(GetPathForFamilyWithName(familyname).c_str());
}

//----------------------------------------------------------------------------
vtkSubsetInclusionLattice::SelectionStates vtkCGNSSubsetInclusionLattice::GetZoneState(
  const char* basename, const char* zonename) const
{
  return this->GetSelectionState(GetPathForZoneWithName(basename, zonename).c_str());
}

//----------------------------------------------------------------------------
bool vtkCGNSSubsetInclusionLattice::ReadGridForZone(
  const char* basename, const char* zonename) const
{
  std::ostringstream stream;
  stream << GetPathForZoneWithName(basename, zonename).c_str() << "/Grid";
  return this->GetSelectionState(stream.str().c_str()) != NotSelected;
}

//----------------------------------------------------------------------------
bool vtkCGNSSubsetInclusionLattice::ReadPatchesForZone(
  const char* basename, const char* zonename) const
{
  std::ostringstream stream;
  stream << "/Patches/" << basename << "/" << SanitizeName(zonename);
  return this->GetSelectionState(stream.str().c_str()) != NotSelected;
}

//----------------------------------------------------------------------------
bool vtkCGNSSubsetInclusionLattice::ReadPatchesForBase(const char* basename) const
{
  std::ostringstream stream;
  stream << "/Patches/" << basename;
  return this->GetSelectionState(stream.str().c_str()) != NotSelected;
}

//----------------------------------------------------------------------------
bool vtkCGNSSubsetInclusionLattice::ReadPatch(
  const char* basename, const char* zonename, const char* patchname) const
{
  std::ostringstream stream;
  stream << GetPathForZoneWithName(basename, zonename) << "/" << patchname;
  return this->GetSelectionState(stream.str().c_str()) != NotSelected;
}

//----------------------------------------------------------------------------
int vtkCGNSSubsetInclusionLattice::AddZoneNode(const char* zonename, int parentnode)
{
  std::string sname = SanitizeName(zonename);
  return this->AddNode(sname.c_str(), parentnode);
}

//----------------------------------------------------------------------------
std::string vtkCGNSSubsetInclusionLattice::SanitizeZoneName(const char* zonename)
{
  return SanitizeName(zonename);
}
