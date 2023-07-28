// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2007, 2009 Los Alamos National Security, LLC
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-LANL-USGov
/**
 * @class   vtkPLANLHaloFinder
 * @brief   find halos within a cosmology data file
 *
 * vtkPLANLHaloFinder is a filter object that operates on the unstructured
 * grid of all particles and assigns each particle a halo id.
 *
 */

#ifndef vtkPLANLHaloFinder_h
#define vtkPLANLHaloFinder_h

#include "vtkPVVTKExtensionsCosmoToolsModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

// Forward declarations
class vtkMultiProcessController;
class vtkIndent;
class vtkInformation;
class vtkInformationVector;
class vtkUnstructuredGrid;

// CosmoTools Forward declarations
namespace cosmotk
{
class CosmoHaloFinderP;
}

// HaloFinderInternals Forward declarations
namespace HaloFinderInternals
{
class ParticleData;
class HaloData;
}

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkPLANLHaloFinder : public vtkUnstructuredGridAlgorithm
{
public:
  enum
  {
    AVERAGE = 0,
    CENTER_OF_MASS = 1,
    MBP = 2,
    MCP = 3,
    NUMBER_OF_CENTER_FINDING_METHODS
  } CenterDetectionAlgorithm;

  static vtkPLANLHaloFinder* New();

  vtkTypeMacro(vtkPLANLHaloFinder, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  ///@{
  /**
   * Set the communicator object for interprocess communication
   */
  virtual vtkMultiProcessController* GetController();
  virtual void SetController(vtkMultiProcessController*);
  ///@}

  ///@{
  /**
   * Specify the number of seeded particles in one dimension (total = np^3)
   * (default 256)
   */
  vtkSetMacro(NP, int);
  vtkGetMacro(NP, int);
  ///@}

  ///@{
  /**
   * Specify the physical box dimensions size (rL)
   * (default 100.0)
   */
  vtkSetMacro(RL, float);
  vtkGetMacro(RL, float);
  ///@}

  ///@{
  /**
   * Specify the ghost cell spacing (in rL units)
   * (edge boundary of processor box)
   * (default 5)
   */
  vtkSetMacro(Overlap, float);
  vtkGetMacro(Overlap, float);
  ///@}

  ///@{
  /**
   * Specify the minimum number of particles for a halo (pmin)
   * (default 100)
   */
  vtkSetMacro(PMin, int);
  vtkGetMacro(PMin, int);
  ///@}

  ///@{
  /**
   * Specify the linking length (bb)
   * (default .2)
   */
  vtkSetMacro(BB, float);
  vtkGetMacro(BB, float);
  ///@}

  ///@{
  /**
   * Turn on calculation of SOD halos
   * (default off)
   */
  vtkSetMacro(ComputeSOD, int);
  vtkGetMacro(ComputeSOD, int);
  ///@}

  ///@{
  /**
   * Specify the FOF center to use in SOD calculations
   * (0 = default, center of mass, 1 = average, 2 = MBP, 3 = MCP)
   */
  vtkSetMacro(CenterFindingMethod, int);
  vtkGetMacro(CenterFindingMethod, int);
  ///@}

  ///@{
  /**
   * Specify rho_c (critical density)
   * (default 2.77536627e11)
   */
  vtkSetMacro(RhoC, float);
  vtkGetMacro(RhoC, float);
  ///@}

  ///@{
  /**
   * Specify the initial SOD mass
   * (default 1.0e14)
   */
  vtkSetMacro(SODMass, float);
  vtkGetMacro(SODMass, float);
  ///@}

  ///@{
  /**
   * Specify the minimum radius factor
   * (default 0.5)
   */
  vtkSetMacro(MinRadiusFactor, float);
  vtkGetMacro(MinRadiusFactor, float);
  ///@}

  ///@{
  /**
   * Specify the maximum radius factor
   * (default 2.0)
   */
  vtkSetMacro(MaxRadiusFactor, float);
  vtkGetMacro(MaxRadiusFactor, float);
  ///@}

  ///@{
  /**
   * Specify the number of bins for SOD finding
   * (default 20)
   */
  vtkSetMacro(SODBins, int);
  vtkGetMacro(SODBins, int);
  ///@}

  ///@{
  /**
   * Specify the minimum FOF size for an SOD halo
   * (default 1000)
   */
  vtkSetMacro(MinFOFSize, int);
  vtkGetMacro(MinFOFSize, int);
  ///@}

  ///@{
  /**
   * Specify the minimum FOF mass for an SOD halo
   * (default 5.0e12)
   */
  vtkSetMacro(MinFOFMass, float);
  vtkGetMacro(MinFOFMass, float);
  ///@}

protected:
  vtkPLANLHaloFinder();
  ~vtkPLANLHaloFinder();

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  /**
   * Checks the integrid of the output particles. Primarily the method ensures
   * that required arrays used in computation are available.
   */
  bool CheckOutputIntegrity(vtkUnstructuredGrid* outputParticles);

  /**
   * Computes the FOF halos based on the user-supplied linking length and
   * PMin parameters.
   */
  void ComputeFOFHalos(vtkUnstructuredGrid* particles, vtkUnstructuredGrid* haloCenters);

  /**
   * Given pre-computed FOF halos, this method computes the SOD halos.
   */
  void ComputeSODHalos(vtkUnstructuredGrid* particles, vtkUnstructuredGrid* haloCenters);

  /**
   * Vectorize the data since the halo-finder expects the data as different
   * vectors.
   */
  void VectorizeData(vtkUnstructuredGrid* particles);

  /**
   * Computes FOF halo properties, i.e., fofMass, fofXPos, etc.
   */
  void ComputeFOFHaloProperties();

  /**
   * Initializes the haloCenter output (output-2 of the filter) s.t. the
   * data-structure is ready to store all FOF properties for each of the
   * N halos.
   */
  void InitializeHaloCenters(vtkUnstructuredGrid* haloCenters, unsigned int N);

  /**
   * Marks the halos of the given halo and computes the center using the
   * prescribed center-finding method.
   */
  void MarkHaloParticlesAndGetCenter(const unsigned int halo, const int internalHaloIdx,
    double center[3], vtkUnstructuredGrid* particles);

  /**
   * Resets halo-finder internal data-structures
   */
  void ResetHaloFinderInternals();

  /**
   * Initialize the SOD haloArrays
   */
  void InitializeSODHaloArrays(vtkUnstructuredGrid* haloCenters);

  vtkMultiProcessController* Controller;

  int NP;        // num particles in the original simulation
  float RL;      // The physical box dimensions (rL)
  float Overlap; // The ghost cell boundary space
  int PMin;      // The minimum particles for a halo
  float BB;      // The linking length

  int CenterFindingMethod; // Halo center detection method
  int ComputeSOD;          // Turn on Spherical OverDensity (SOD) halos

  float RhoC;            // SOD rho_C (2.77536627e11)
  float SODMass;         // Initial SOD mass (1.0e14)
  float MinRadiusFactor; // Minimum factor of SOD radius (0.5)
  float MaxRadiusFactor; // Maximum factor of SOD radius (2.0)
  int SODBins;           // Number of log scale bins for SOD (20)
  int MinFOFSize;        // Minimum FOF size for SOD (1000)
  float MinFOFMass;      // Minimum FOF mass for SOD (5.0e12)

  HaloFinderInternals::ParticleData* Particles;
  HaloFinderInternals::HaloData* Halos;
  cosmotk::CosmoHaloFinderP* HaloFinder;

private:
  vtkPLANLHaloFinder(const vtkPLANLHaloFinder&) = delete;
  void operator=(const vtkPLANLHaloFinder&) = delete;
};

#endif //  vtkPLANLHaloFinder_h
