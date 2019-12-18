/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPANLSubhaloFinder.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#ifndef vtkPANLSubhaloFinder_h
#define vtkPANLSubhaloFinder_h

/**
 * @class   vtkPANLSubhaloFinder
 *
 *
 * This filter takes the output of the HACC halo finder and runs the subhalo
 * finder on a user-specified list of halos.
 *
 * The input should be a vtkUnstructuredGrid with the data arrays vx, vy, vz,
 * id and fof_halo_tag.  AddHaloToProcess should be used to select which halos
 * to find subhalos of.
 *
 * The first output is a vtkUnstructuredGrid similar to the input but with the
 * data array subhalo_tag appended to indicate which subhalo a point lies in.
 * The value will be -1 for points in no halo.
 *
 * The second output is a vtkUnstructuredGrid with one point per subhalo and
 * data arrays for summary information such as average velocity, mass, and
 * center of mass.
*/

#include "vtkNew.h"                             // for vtkNew
#include "vtkPVVTKExtensionsCosmoToolsModule.h" // for export macro
#include "vtkPassInputTypeAlgorithm.h"

class vtkMultiProcessController;
class vtkIdList;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPANLSubhaloFinder : public vtkPassInputTypeAlgorithm
{
  vtkTypeMacro(vtkPANLSubhaloFinder, vtkPassInputTypeAlgorithm) public
    : static vtkPANLSubhaloFinder* New();
  void PrintSelf(ostream& os, vtkIndent indent);

  enum
  {
    ALL_HALOS = 0,
    HALOS_LARGER_THAN_THRESHOLD = 1,
    ONLY_SELECTED_HALOS = 2
  };

  //@{
  /**
   * Sets/Gets the mode of the subhalo finding filter.  It should be one of the values
   * ALL_HALOS - find subhalos of all halos (default)
   * HALOS_LARGER_THAN_THRESHOLD - finds subhalos of all halos with more than
   * SizeThreshold values (sizeThreshold is a settable
   * parameter)
   * ONLY_SELECTED_HALOS - finds subhalos of only the halos in the HalosToProcess list
   */
  vtkSetClampMacro(Mode, int, ALL_HALOS, ONLY_SELECTED_HALOS) vtkGetMacro(Mode, int)
    //@}

    //@{
    /**
     * Sets/Gets the size threshold which is the minimum number of particles in the halos
     * that will have the subhalo finder run on them in HALOS_LARGER_THAN_THRESHOLD mode.
     * Defaults to 1000.
     */
    vtkSetMacro(SizeThreshold, vtkIdType) vtkGetMacro(SizeThreshold, vtkIdType)
    //@}

    /**
     * Gets the halo to process at the given index
     * This list is used when the filter is in ONLY_SELECTED_HALOS mode
     */
    vtkIdType GetHaloToProcess(vtkIdType idx);
  /**
   * Adds a halo to the list of halos that will have the subhalo finder run on them
   * This list is used when the filter is in ONLY_SELECTED_HALOS mode
   */
  void AddHaloToProcess(vtkIdType haloId);
  /**
   * Sets the halo id to process at the given index in the list
   * This list is used when the filter is in ONLY_SELECTED_HALOS mode
   */
  void SetHaloToProcess(vtkIdType idx, vtkIdType haloId);
  /**
   * Sets the number of halos to process
   * This list is used when the filter is in ONLY_SELECTED_HALOS mode
   */
  void SetNumberOfHalosToProcess(vtkIdType num);
  /**
   * Gets the number of halos to process
   * This list is used when the filter is in ONLY_SELECTED_HALOS mode
   */
  vtkIdType GetNumberOfHalosToProcess();
  /**
   * Clears the list of halos that will have the subhalo finder run on them
   * This list is used when the filter is in ONLY_SELECTED_HALOS mode
   */
  void ClearHalosToProcess();

  //@{
  /**
   * Gets/Sets RL, the physical coordinate box size
   * Default: 256.0
   */
  vtkSetMacro(RL, double) vtkGetMacro(RL, double)
    //@}

    //@{
    /**
     * Gets/Sets the size of the ghost particle region around each process's particles
     * to exchange when creating ghost particles.
     * Default: 8.0
     */
    vtkSetMacro(DeadSize, double) vtkGetMacro(DeadSize, double)
    //@}

    //@{
    /**
     * Gets/Sets the particle mass.  For input datasets that do not have mass information
     * the mass of each particle defaults to this value.
     * Default: 1.307087181e+09
     */
    vtkSetMacro(ParticleMass, float) vtkGetMacro(ParticleMass, float)
    //@}

    //@{
    /**
     * Gets/Sets distance threshold for particles to be considered in the same
     * halo.  This is measured in grid units on a NP x NP x NP grid.
     * Default: 0.1679999998
     */
    vtkSetMacro(BB, double) vtkGetMacro(BB, double)
    //@}

    //@{
    /**
     * Gets/Sets alpha factor.  This controls how aggressively small subhalos
     * are grown.  Alpha factor of 1.0 is the least aggressive
     * Default: 1.0
     */
    vtkSetClampMacro(AlphaFactor, double, 0.0, 1.0) vtkGetMacro(AlphaFactor, double)
    //@}

    //@{
    /**
     * Gets/Sets beta factor.  This controls how saddle points between
     * subhalos are treated.  Larger values allow identification of smaller
     * scale structures such as tails.
     * Default: 0.0
     */
    vtkSetClampMacro(BetaFactor, double, 0.0, 1.0) vtkGetMacro(BetaFactor, double)
    //@}

    //@{
    /**
     * Gets/Sets the minimum size of a subhalo candidate
     * Default: 200
     */
    vtkSetMacro(MinCandidateSize, int) vtkGetMacro(MinCandidateSize, int)
    //@}

    //@{
    /**
     * Gets/Sets NumSPHNeighbors
     * Default: 64
     */
    vtkSetMacro(NumSPHNeighbors, int) vtkGetMacro(NumSPHNeighbors, int)
    //@}

    //@{
    /**
     * Gets/Sets the number of neighbors that are examined by the subhalo finder
     * to determine local clumps near the each particle
     * Default: 20
     */
    vtkSetMacro(NumNeighbors, int) vtkGetMacro(NumNeighbors, int)
    //@}

    protected : vtkPANLSubhaloFinder();
  virtual ~vtkPANLSubhaloFinder();

  int FillInputPortInformation(int port, vtkInformation* info);
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  double RL;
  double DeadSize;
  double BB;
  double ParticleMass;
  double AlphaFactor;
  double BetaFactor;
  int MinCandidateSize;
  int NumSPHNeighbors;
  int NumNeighbors;

  int Mode;
  vtkIdType SizeThreshold;

  vtkNew<vtkIdList> HalosToProcess;
  vtkMultiProcessController* Controller;

  class vtkInternals;
  vtkInternals* Internal;

private:
  vtkPANLSubhaloFinder(const vtkPANLSubhaloFinder&) = delete;
  void operator=(const vtkPANLSubhaloFinder&) = delete;

  void ExecuteSubHaloFinder(vtkUnstructuredGrid* input, vtkUnstructuredGrid* allParticles,
    vtkUnstructuredGrid* subFofProperties);
};

#endif
