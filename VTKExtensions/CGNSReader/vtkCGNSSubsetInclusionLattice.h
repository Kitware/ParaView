/*=========================================================================

  Program:   ParaView
  Module:    vtkCGNSSubsetInclusionLattice.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkCGNSSubsetInclusionLattice
 * @brief extends vtkSubsetInclusionLattice to CGNS friendly API.
 *
 * vtkCGNSSubsetInclusionLattice simply makes it easier for users (and the
 * vtkCGNSReader) to query vtkSubsetInclusionLattice using CGNS terminology.
 *
 * @par Zone names and partitioned files
 *
 * vtkCGNSFileSeriesReader supports reading of a partitioned file series where
 * each file has all timesteps but only the data from the rank the file was
 * written out on. In those cases, some sims use suffixes for zone names to
 * identify proc number e.g. `blk-1_proc-0`, `blk-1_proc-1`, etc. Since this has
 * a tendency to make the SIL unwieldy, we sanitize zonenames by default to
 * remove the `_proc-.*` suffix. All API on vtkCGNSFileSeriesReader that takes a
 * zonename sanitizes it. If you use API on vtkSubsetInclusionLattice you may
 * need to sanitize zonenames manually by calling `SanitizeZoneName`.
 *
 */
#ifndef vtkCGNSSubsetInclusionLattice_h
#define vtkCGNSSubsetInclusionLattice_h

#include "vtkPVVTKExtensionsCGNSReaderModule.h" // for exports
#include "vtkSubsetInclusionLattice.h"

class VTKPVVTKEXTENSIONSCGNSREADER_EXPORT vtkCGNSSubsetInclusionLattice
  : public vtkSubsetInclusionLattice
{
public:
  static vtkCGNSSubsetInclusionLattice* New();
  vtkTypeMacro(vtkCGNSSubsetInclusionLattice, vtkSubsetInclusionLattice);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Select/deselect the base state.
   */
  void SelectBase(const char* basename);
  void DeselectBase(const char* basename);
  void SelectAllBases();
  void DeselectAllBases();
  //@}

  /**
   * Returns the status for a base with the given name.
   */
  SelectionStates GetBaseState(const char* basename) const;

  //@{
  /**
   * API to query information about bases.
   */
  int GetNumberOfBases() const;
  const char* GetBaseName(int index) const;
  //@}

  //@{
  /**
   * Select/deselect a family.
   */
  void SelectFamily(const char* familyname);
  void DeselectFamily(const char* familyname);
  void SelectAllFamilies();
  void DeselectAllFamilies();
  //@}

  //@{
  /**
   * API to query information about families.
   */
  int GetNumberOfFamilies() const;
  const char* GetFamilyName(int index) const;
  //@}

  /**
   * Returns the status for a family.
   */
  SelectionStates GetFamilyState(const char* familyname) const;

  //@}
  /**
   * Returns the state for a zone under a given base.
   * Note this indicates either the zone-grid and/or the zone-bcs are selected.
   */
  SelectionStates GetZoneState(const char* basename, const char* zonename) const;

  /**
   * Returns true if the grid/mesh for the given zone is selected.
   */
  bool ReadGridForZone(const char* basename, const char* zonename) const;

  /**
   * Returns true if any of the patches for a given zone are enabled.
   */
  bool ReadPatchesForZone(const char* basename, const char* zonename) const;

  /**
   * Returns true if any of the patches for a given base are enabled.
   */
  bool ReadPatchesForBase(const char* basename) const;

  /**
   * Returns true of a patch identified by basename, zonename and patchname is
   * enabled.
   * TODO: wonder if these should be called 'IsEnabled...` instead.
   */
  bool ReadPatch(const char* basename, const char* zonename, const char* patchname) const;

  //@{
  /**
   * Convenience API to add nodes.
   */
  int AddZoneNode(const char* basename, int parentnode);
  //@}

  /**
   * Sanitizes zonenames by removed any `_proc-[0-9]+` suffix.
   */
  static std::string SanitizeZoneName(const char* zonename);

protected:
  vtkCGNSSubsetInclusionLattice();
  ~vtkCGNSSubsetInclusionLattice();

private:
  vtkCGNSSubsetInclusionLattice(const vtkCGNSSubsetInclusionLattice&) = delete;
  void operator=(const vtkCGNSSubsetInclusionLattice&) = delete;
};

#endif
