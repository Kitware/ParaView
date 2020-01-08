/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPLANLHaloFinder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkPLANLHaloFinder.h

Copyright (c) 2007, 2009, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007, 2009. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
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

  //@{
  /**
   * Set the communicator object for interprocess communication
   */
  virtual vtkMultiProcessController* GetController();
  virtual void SetController(vtkMultiProcessController*);
  //@}

  //@{
  /**
   * Specify the number of seeded particles in one dimension (total = np^3)
   * (default 256)
   */
  vtkSetMacro(NP, int);
  vtkGetMacro(NP, int);
  //@}

  //@{
  /**
   * Specify the physical box dimensions size (rL)
   * (default 100.0)
   */
  vtkSetMacro(RL, float);
  vtkGetMacro(RL, float);
  //@}

  //@{
  /**
   * Specify the ghost cell spacing (in rL units)
   * (edge boundary of processor box)
   * (default 5)
   */
  vtkSetMacro(Overlap, float);
  vtkGetMacro(Overlap, float);
  //@}

  //@{
  /**
   * Specify the minimum number of particles for a halo (pmin)
   * (default 100)
   */
  vtkSetMacro(PMin, int);
  vtkGetMacro(PMin, int);
  //@}

  //@{
  /**
   * Specify the linking length (bb)
   * (default .2)
   */
  vtkSetMacro(BB, float);
  vtkGetMacro(BB, float);
  //@}

  //@{
  /**
   * Turn on calculation of SOD halos
   * (default off)
   */
  vtkSetMacro(ComputeSOD, int);
  vtkGetMacro(ComputeSOD, int);
  //@}

  //@{
  /**
   * Specify the FOF center to use in SOD calculations
   * (0 = default, center of mass, 1 = average, 2 = MBP, 3 = MCP)
   */
  vtkSetMacro(CenterFindingMethod, int);
  vtkGetMacro(CenterFindingMethod, int);
  //@}

  //@{
  /**
   * Specify rho_c (critical density)
   * (default 2.77536627e11)
   */
  vtkSetMacro(RhoC, float);
  vtkGetMacro(RhoC, float);
  //@}

  //@{
  /**
   * Specify the initial SOD mass
   * (default 1.0e14)
   */
  vtkSetMacro(SODMass, float);
  vtkGetMacro(SODMass, float);
  //@}

  //@{
  /**
   * Specify the minimum radius factor
   * (default 0.5)
   */
  vtkSetMacro(MinRadiusFactor, float);
  vtkGetMacro(MinRadiusFactor, float);
  //@}

  //@{
  /**
   * Specify the maximum radius factor
   * (default 2.0)
   */
  vtkSetMacro(MaxRadiusFactor, float);
  vtkGetMacro(MaxRadiusFactor, float);
  //@}

  //@{
  /**
   * Specify the number of bins for SOD finding
   * (default 20)
   */
  vtkSetMacro(SODBins, int);
  vtkGetMacro(SODBins, int);
  //@}

  //@{
  /**
   * Specify the minimum FOF size for an SOD halo
   * (default 1000)
   */
  vtkSetMacro(MinFOFSize, int);
  vtkGetMacro(MinFOFSize, int);
  //@}

  //@{
  /**
   * Specify the minimum FOF mass for an SOD halo
   * (default 5.0e12)
   */
  vtkSetMacro(MinFOFMass, float);
  vtkGetMacro(MinFOFMass, float);
  //@}

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
