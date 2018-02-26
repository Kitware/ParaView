/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkPANLHaloFinder.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#ifndef vtkPANLHaloFinder_h
#define vtkPANLHaloFinder_h

/**
 * @class   vtkPANLHaloFinder
 *
 *
 * Given an input a vtkUnstructuredGrid of points with arrays vx, vy, vz, and
 * id, finds clumps of points (halos) using the cosmotools halo finder.  The
 * first output is a vtkUnstructuredGrid similar to the input but with the data
 * array fof_halo_tag appended to indicate which halo a point lies in.  The
 * value will be -1 for points in no halo.  Optionally, subhalo finding can be
 * turned on which will find subhalos of all halos over a certain size.  The
 * subhalo id a point is in will be appended as the data array subhalo_tag.
 *
 * The second output is a summary of halo properties such as mass, center of mass,
 * center (computed via one of several algorithms), and net velocity.  This
 * vtkUnstructuredGrid has one point per halo.
 *
 * The third output is empty unless subhalo finding is turned on.  If subhalo
 * finding is on, this output is similar to the second output except with data
 * for each subhalo rather than each halo.  It contains one point per subhalo.
*/

#include "vtkPVVTKExtensionsCosmoToolsModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkMultiProcessController;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPANLHaloFinder : public vtkUnstructuredGridAlgorithm
{
  vtkTypeMacro(vtkPANLHaloFinder, vtkUnstructuredGridAlgorithm) public
    : static vtkPANLHaloFinder* New();
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Turns on/off the subhalo finder part of the algorithm
   * Default: Off
   */
  vtkSetMacro(RunSubHaloFinder, bool) vtkGetMacro(RunSubHaloFinder, bool)
    vtkBooleanMacro(RunSubHaloFinder, bool)
    //@}

    //@{
    /**
     * Gets/Sets RL, the physical coordinate box size
     * Default: 256.0
     */
    vtkSetMacro(RL, double) vtkGetMacro(RL, double)
    //@}

    //@{
    /**
     * Gets/Sets the distance conversion factor.  This is multiplied into all position
     * coordinates before the halo finder is run.
     * Default: 1.0
     */
    vtkSetMacro(DistanceConvertFactor, double) vtkGetMacro(DistanceConvertFactor, double)
    //@}

    //@{
    /**
     * Gets/Sets the mass conversion factor.  This is multiplied into the particle mass
     * before the halo finder is run.
     * Default: 1.0
     */
    vtkSetMacro(MassConvertFactor, double) vtkGetMacro(MassConvertFactor, double)
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
     * Gets/Sets NP
     * Default: 1024
     */
    vtkSetMacro(NP, int) vtkGetMacro(NP, int)
    //@}

    //@{
    /**
     * Gets/Sets the minimum number of close neighbors for a halo candidate to
     * include a particle.
     * Default: 1
     */
    vtkSetMacro(NMin, int) vtkGetMacro(NMin, int)
    //@}

    //@{
    /**
     * Gets/Sets the minimum number of particles required for a halo candidate to
     * be considered a halo and output
     * Default: 10000
     */
    vtkSetMacro(PMin, int) vtkGetMacro(PMin, int)
    //@}

    //@{
    /**
     * Gets/Sets the minimum halo size to run the subhalo finder on.
     * Default: 10000
     */
    vtkSetMacro(MinFOFSubhaloSize, long) vtkGetMacro(MinFOFSubhaloSize, long)
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

    enum CenterFindingType {
      NONE = 0,
      MOST_BOUND_PARTICLE = 1,
      MOST_CONNECTED_PARTICLE = 2,
      HIST_CENTER_FINDING = 3
    };

  //@{
  /**
   * Gets/Sets the center finding method used by the halo finder once halos are
   * identified.
   * Default: NONE
   */
  vtkSetMacro(CenterFindingMode, int) vtkGetMacro(CenterFindingMode, int)
    //@}

    //@{
    /**
     * Gets/Sets the smoothing length used by the center finders
     * Default: 0.0
     */
    vtkSetMacro(SmoothingLength, double) vtkGetMacro(SmoothingLength, double)
    //@}

    //@{
    /**
     * Gets/Sets the OmegaDM parameter of the simulation.  Used by the center
     * finding algorithms.
     * Default: 0.26627
     */
    vtkSetMacro(OmegaDM, double) vtkGetMacro(OmegaDM, double)
    //@}

    //@{
    /**
     * Gets/Sets the OmegaNU parameter of the simulation.  Used by the center
     * finding algorithms.
     * Default: 0.0
     */
    vtkSetMacro(OmegaNU, double) vtkGetMacro(OmegaNU, double)
    //@}

    //@{
    /**
     * Gets/Sets the Deut parameter of the simulation.  Used by the center
     * finding algorithms.
     * Default: 0.02258
     */
    vtkSetMacro(Deut, double) vtkGetMacro(Deut, double)
    //@}

    //@{
    /**
     * Gets/Sets the Hubble parameter of the simulation.  Used by the center
     * finding algorithms.
     * Default: 0.673
     */
    vtkSetMacro(Hubble, double) vtkGetMacro(Hubble, double)
    //@}

    //@{
    /**
     * Gets/Sets the current redshift.  Used by the center finding algorithms.
     * Default: 0.0
     */
    vtkSetMacro(RedShift, double) vtkGetMacro(RedShift, double)
    //@}

    protected : vtkPANLHaloFinder();
  virtual ~vtkPANLHaloFinder();

  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int FillInputPortInformation(int port, vtkInformation* info);

  double RL;
  double DistanceConvertFactor;
  double MassConvertFactor;
  double DeadSize;
  float ParticleMass;
  double BB;
  double AlphaFactor;
  double BetaFactor;
  int NP;
  int NMin;
  int PMin;
  long MinFOFSubhaloSize;
  int MinCandidateSize;
  int NumSPHNeighbors;
  int NumNeighbors;

  bool RunSubHaloFinder;

  // Center finding parameters
  int CenterFindingMode;
  double SmoothingLength;
  double OmegaNU;
  double OmegaDM;
  double Deut;
  double Hubble;
  double RedShift;

  vtkMultiProcessController* Controller;

  class vtkInternals;
  vtkInternals* Internal;

private:
  vtkPANLHaloFinder(const vtkPANLHaloFinder&) = delete;
  void operator=(const vtkPANLHaloFinder&) = delete;

  void ExtractDataArrays(vtkUnstructuredGrid* input, vtkIdType offset);
  void DistributeInput();
  void CreateGhostParticles();
  void ExecuteHaloFinder(vtkUnstructuredGrid* allParticles, vtkUnstructuredGrid* fofProperties);
  void ExecuteSubHaloFinder(
    vtkUnstructuredGrid* allParticles, vtkUnstructuredGrid* subFofProperties);
  void FindCenters(vtkUnstructuredGrid* allParticles, vtkUnstructuredGrid* fofProperties);
};

#endif // vtkPANLHaloFinder_h
