/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridConnectivity.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRectilinearGridConnectivity
 * @brief   Extracts material fragments from
 *  multi-block vtkRectilinearGrid datasets based on the selected volume
 *  fraction array(s) and a fraction isovalue and integrates the associated
 *  attributes.
 *
 *
 *  Given one or multiple vtkRectilinearGrid datasets with one or multiple
 *  volume fraction arrays (representing some materials) and possibly other
 *  attributes (like pressure, density, and et al) as the cell data, this
 *  filter extracts fragments from the dual grids (with the aforementioned
 *  values as the point data) based on the (at least one) selected fraction
 *  array(s) in combination with a specified fraction value and integrates
 *  the attributes (e.g., volume, pressure, density) across the surface of
 *  each fragment. Each material, made up of one or multiple disconnected
 *  fragments, is exported to the output vtkMultiBlockDataSet as a single
 *  block that is a vtkPolyData storing the exterior polygons of the fragment
 *  (s) and the associated cell data attributes as the integrated result (of
 *  the fragment) in terms of the volume, pressure, density, and et al.
 *
 *  This filter differs from a closely related filter vtkGridConnectivity in
 *  that the former extracts fragments at a sub-cell resolution to create
 *  relatively smooth surfaces while the latter works at the cell granularity
 *  (a whole cell is taken as either inside or outside a fragment) to cause
 *  staircasing artifacts. In fact, an extended 256-entry marching cubes
 *  LUT is designed for generating cube faces (either truncated by iso-lines
 *  or not) in addition to iso-triangles. These two kinds of polygons in
 *  combination represent the surface(s) of the greater-than-isovalue sub-
 *  volume(s) extracted in a cube.
 *
 *  vtkRectilinearGridConnectivity performs fragments extraction using a
 *  three-level mechanism, i.e., intra-process intra-block, intra-process
 *  inter-block, and inter-process in increasing order, with the fragments
 *  extracted (in the form of polygons stored as a vtkPolyData) at a lower
 *  level submitted to its upper level for further extraction (specifically
 *  by combining multiple disconnected fragments into a single one wherever
 *  possible). Since a fragment is represented by means of its exterior faces
 *  / polygons, extracting fragments turns into the task of detecting and
 *  removing internal faces (an internal face is the one shared by two sub-
 *  volumes or fragments) in a way of combining the associated sub-volumes or
 *  fragments. For the fragemnts extraction conducted at any level, the
 *  polygons of the input (e.g., greater-than-isovalue sub-volumes resulting
 *  from marching cubes for the lowest level extraction) are pushed to a face
 *  hash (that accepts the three smallest point Ids of a polygon: triangle,
 *  quad, or pentagon) on a per sub-volume or fragment basis. Once the face
 *  hash detects an internal face, an entry is added to an equivalence set
 *  (by means of class vtkEquivalenceSet) to correlate the two fragment Ids
 *  that are attached to the two associated sub-volumes or fragments' polygons.
 *  After resolving the equivalence set, each face that remains in the face
 *  hash (internal faces are masked as invalid) is updated with a resolved
 *  fragment Id. In this way the original complete polygons (triangles, quads,
 *  pentagons) pointed to by the remaining hashed faces with the same resolved
 *  fragment Id are retrieved from the input vtkPolyData and hence combined by
 *  means of the same fragemnt Id.
 *
 * @sa
 *  vtkGridConnectivity vtkExtractCTHPart vtkPolyData vtkRectilinearGrid
 *  vtkMultiBlockDataSetAlgorithm
*/

#ifndef vtkRectilinearGridConnectivity_h
#define vtkRectilinearGridConnectivity_h

#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports

class vtkPolyData;
class vtkDoubleArray;
class vtkInformation;
class vtkEquivalenceSet;
class vtkRectilinearGrid;
class vtkInformationVector;
class vtkMultiProcessController;
class vtkIncrementalOctreePointLocator;
class vtkRectilinearGridConnectivityFaceHash;
class vtkRectilinearGridConnectivityInternal;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkRectilinearGridConnectivity
  : public vtkMultiBlockDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkRectilinearGridConnectivity, vtkMultiBlockDataSetAlgorithm);
  static vtkRectilinearGridConnectivity* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set / get the volume fraction value [0, 1] used for extracting fragments.
   */
  vtkSetClampMacro(VolumeFractionSurfaceValue, double, 0.0, 1.0);
  vtkGetMacro(VolumeFractionSurfaceValue, double);
  //@}

  /**
   * Remove all volume array names.
   */
  void RemoveAllVolumeArrayNames();

  /**
   * Remove double-type volume array names.
   */
  void RemoveDoubleVolumeArrayNames();

  /**
   * Remove float-type volume array names.
   */
  void RemoveFloatVolumeArrayNames();

  /**
   * Remove unsigned char-type volume array names.
   */
  void RemoveUnsignedCharVolumeArrayNames();

  /**
   * Add a double-type volume array name to the selection list.
   */
  void AddDoubleVolumeArrayName(char* arayName);

  /**
   * Add a float-type volume array name to the selection list.
   */
  void AddFloatVolumeArrayName(char* arayName);

  /**
   * Add an unsigned char-type volume array name to the selection list.
   */
  void AddUnsignedCharVolumeArrayName(char* arayName);

  /**
   * Add a volume array (of any type) name to the selection list.
   */
  void AddVolumeArrayName(char* arayName);

protected:
  vtkRectilinearGridConnectivity();
  ~vtkRectilinearGridConnectivity() override;

  int DualGridsReady;
  int NumberOfBlocks;
  double DataBlocksTime;
  double DualGridBounds[6];
  double VolumeFractionSurfaceValue;
  vtkDoubleArray* FragmentValues;
  vtkEquivalenceSet* EquivalenceSet;
  vtkRectilinearGrid** DualGridBlocks;
  vtkMultiProcessController* Controller;
  vtkRectilinearGridConnectivityFaceHash* FaceHash;
  vtkRectilinearGridConnectivityInternal* Internal;

  vtkExecutive* CreateDefaultExecutive() override;
  int FillInputPortInformation(int, vtkInformation*) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // ---------------------------------------------------------------------- //
  // --------------------------- Volume  arrays --------------------------- //
  // ---------------------------------------------------------------------- //

  /**
   * Get the number of selected volume fraction arrays.
   */
  int GetNumberOfVolumeFractionArrays();

  /**
   * Get the number of all volume arrays (not necessarily selected as the
   * ones for extracting fragemnts).
   */
  int GetNumberOfVolumeArrays();

  /**
   * Get the name of a selected volume fraction array specified by arrayIdx.
   */
  const char* GetVolumeFractionArrayName(int arrayIdx);

  /**
   * This function determines whether the specified name (arayName) refers to
   * a selected volume fraction array (1) or not (0).
   */
  bool IsVolumeFractionArray(const char* arayName);

  /**
   * This function determines whether the specified name (arayName) refers to
   * a volume array (1) or not (0). Note the array is not necessarily a
   * selected one used for extracting fragments.
   */
  bool IsVolumeArray(const char* arayName);

  /**
   * This function checks the consistency between a number (numGrids) of
   * vtkRectilinearGrid blocks (recGrids, the original ones still with cell
   * data attributes) in terms of the number of data attributes and specific
   * data attribute array names. It also checks the consistency of multiple
   * (unless one) volume fraction arrays in the data type. This function
   * returns 1 for consistency and 0 for inconsistency.
   */
  int CheckVolumeDataArrays(vtkRectilinearGrid** recGrids, int numGrids);

  // ---------------------------------------------------------------------- //
  // ----------- Dual grids generation and fragments extraction ----------- //
  // ---------------------------------------------------------------------- //

  /**
   * Given an input vtkRectilinearGrid dataset (rectGrid) with cell data
   * attributes, this function creates a dual vtkRectilinearGrid dataset with
   * point data attributes.
   */
  void CreateDualRectilinearGrid(vtkRectilinearGrid* rectGrid, vtkRectilinearGrid* dualGrid);

  /**
   * Given numBlcks vtkRectilinearGrid blocks with point data attributes
   * (i.e.,the dual grid version of the original blocks with cell data
   * attributes), the bounding box (boundBox) covering these blocks, and
   * a selected volume fraction array specified by the material index
   * (partIndx), this function extracts fragments based on this volume
   * fraction array and exports the result to a vtkPolyData (polyData).
   */
  void ExtractFragments(vtkRectilinearGrid** dualGrds, int numBlcks, double boundBox[6],
    unsigned char partIndx, vtkPolyData* polyData);

  // ---------------------------------------------------------------------- //
  // ------------- Common functions  for fragments extraction ------------- //
  // ---------------------------------------------------------------------- //

  /**
   * Given the raw fragment index (fragIndx) of either an original sub-volume
   * extracted from marching cubes of a block or a macro-volume (combination
   * of intra-process inter-block sub-volumes), the number (numComps) of all
   * integrated components (of multiple cell data attributes), and the array
   * (attrVals) storing the specific integrated values, this function
   * integrates the attributes based on the fragment index, specifically by
   * accumulating the input integration values to the target entry (specified
   * by the fragment index) of a global integration array. In this way,
   * different sub-volumes or macro-volumes belonging to the same fragment
   * have their integrated results summed together.
   */
  void IntegrateFragmentAttributes(int fragIndx, int numComps, double* attrVals);

  /**
   * This function resolves the equivalence set (intra-process intra-block,
   * intra-process inter-block, or inter-process), followed by the face
   * fragment Ids and integrated fragment attributes.
   */
  void ResolveEquivalentFragments();

  /**
   * With the equivalence set (intra-process intra-block, intra-process inter-
   * block, or inter-process) resolved in advance, this function updates each
   * face / polygon stored in the face hash with a resolved fragment Id. In
   * this way, polygons of multiple sub-volumes or macro-volumes that have
   * different (old) fragment Ids are automatically grouped into a single
   * entity (macro-volume or fragment, respectively) by being assigned with the
   * same (new) fragment Id.
   */
  void ResolveFaceFragmentIds();

  /**
   * With the equivalence set (intra-process intra-block, intra-process inter-
   * block, or inter-process) resolved in advance, this function integrates
   * cell data attributes based on the fragment Id. Specifically, multiple
   * entries (of the old array of integrated attributes) with the same fragment
   * Id have their integration results summed together and are then combined
   * into a single entry (of the new shortened array of integrated attributes).
   */
  void ResolveIntegratedFragmentAttributes();

  // ---------------------------------------------------------------------- //
  // ----------- Intra-process intra-block fragments extraction ----------- //
  // ---------------------------------------------------------------------- //

  // Descrption:
  // Give a vtkRectilinearGrid data block (rectGrid, the dual version of an
  // original input block) with point data attributes (including some volume
  // fraction arrays and others like pressure, density, or velocity vector),
  // the name of a selected volume fraction array (fracName, through which to
  // extract fragments), and a specified volume fraction iso-value (isoValue),
  // this function performs the marching-cubes algorithm based on the volume
  // fraction array and the iso-value to produce greater-than-isovalue sub-
  // volumes (or polyhedra) by employing an extended 256-entry lookup table.
  // These resulting polyhedra are stored in the output vtkPolyData (plyHedra).
  // All point data attributes except for non-selected volume fraction arrays
  // are integrated when marching cubes. The integrated attribute arrays are
  // attached to the polyhedra's faces as the cell data.
  void ExtractFragmentPolyhedra(
    vtkRectilinearGrid* rectGrid, const char* fracName, double isoValue, vtkPolyData* plyHedra);

  /**
   * Given a vtkPolyData (plyHedra) storing the polygons of the greater-than-
   * isovalue sub-volumes (or polyhedra) extracted from a data block, this
   * function initializes the size of the face hash (with the number of points
   * of the polyhedra) used to maintain the polygons of the polyhedra.
   */
  void InitializeFaceHash(vtkPolyData* plyHedra);

  /**
   * Given a data block index (blockIdx) and a vtkPolyData (plyHedra) storing
   * the polygons of the greater-than-isovalue sub-volumes (or polyhedra) that
   * are extracted from the block, this function pushes the polygons on a per
   * sub-volume basis to the face hash (the sub-volume Ids are used to group
   * polygons into sub-volumes). For each internal polygon / face shared by
   * two sub-volumes, an entry is added to the (intra-process intra-block)
   * equivalence set to associate the fragment Ids assigned to the two sub-
   * volumes' polygons.
   */
  void AddPolygonsToFaceHash(int blockIdx, vtkPolyData* plyHedra);

  /**
   * Given the index of the data block (blockIdx), the vtkPolyData (plyHedra)
   * storing the polygons / faces of the greater-than-isovalue sub-volumes
   * (or polyhedra) extracted from this block via marching cubes, and a global
   * (intra-process inter-block) point locator, the function extracts fragments
   * from this block by initializing the face hash with the raw polygons
   * extracted from the block (via marching cubes), pushing these polygons to
   * the face hash on a per sub-volume basis, detecting internal faces or
   * polygons, registering faces to the intra-process intra-block equivalence
   * set by the intermediate fragment Id, resolving the equivalence set,
   * updating the fragment Id of each polygon, and integrating cell data
   * attributes based on the fragment Id, grouping / re-arranging the 'valid'
   * faces (as opposed to those removed internal faces) in the face hash by the
   * fragment Id (to construct macro-volumes), accessing the full version of
   * each valid face from the original polyhedra, and attaching the fragment Id
   * as well as other integrated cell data attributes to each cell in the output
   * vtkPolyData data (polygons) where each point is assigned with a global
   * (intra-process inter-block) Id (via a global point locator gPtIdGen) used
   * for subsequent combination of fragments extracted from multiple blocks.
   * Argument maxFSize returns the maximum number of faces a macro-volume
   * (combination of sub-volumes) may contain in a block.
   */
  void ExtractFragmentPolygons(int blockIdx, int& maxFsize, vtkPolyData* plyHedra,
    vtkPolyData* polygons, vtkIncrementalOctreePointLocator* gPtIdGen);

  // ---------------------------------------------------------------------- //
  // ----------- Intra-process inter-block fragments extraction ----------- //
  // ---------------------------------------------------------------------- //

  /**
   * Given a number (numPolys) of vtkPolyData objects (plyDatas) storing the
   * fragments extracted from the multiple data blocks, this function inits
   * the face hash (with the maximum global inter-block point Id: a point data
   * attribute attached to each point of these vtkPolyData objects) that is
   * used to combine these intermediate fragments.
   */
  void InitializeFaceHash(vtkPolyData** plyDatas, int numPolys);

  /**
   * Given a number (numPolys) of vtkPolyData objects (plyDatas) storing the
   * initial fragments (macro-volumes --- combination of greater-than-isovalue
   * sub-volumes) extracted from multiple blocks and an array of values
   * (maxFsize) storing the maximum number of faces that a macro-volume may
   * contain in each block (used to allocate appropriate memory for buffering
   * the polygons of a macro-volume), this function pushes these polygons on a
   * per macro-volume basis to the face hash (fragment Ids are used to group
   * polygons to macro-volumes), detects internal faces / polygons, and then
   * registers polygons to the (intra-process inter-block) equivalence set by
   * the intermediate fragment Id. For each internal polygon / face shared by
   * two macro-volumes, an entry is added to the equivalence set to associate
   * the fragment Ids assigned to the two macro-volumes' polygons.
   */
  void AddPolygonsToFaceHash(vtkPolyData** plyDatas, int* maxFsize, int numPolys);

  /**
   * With the intra-process inter-block equivalence set resolved, intra-process
   * inter-block fragment Ids resolved, and cell data attributes integrated by
   * the fragment Id, this function retrieves the face hash for the 'valid'
   * faces (as opposed to those removed internal faces), accesses their full
   * version from the vtkPolyData objects ('surfaces') that store the polygons
   * of intermediate fragments extracted from multiple ('numSurfs') data blocks,
   * and exports these complete yet 'valid' polygons to the output vtkPolyData
   * ('polyData') while attaching the index of the material (partIndx, i.e. the
   * volume fraction array used to extract fragments) to each polygon as a cell
   * data attribute. In a word, this function exports a combined version of the
   * fragments extracted from multiple blocks assigned to a single process.
   */
  void GenerateOutputFromSingleProcess(
    vtkPolyData** surfaces, int numSurfs, unsigned char partIndx, vtkPolyData* polyData);

  // ---------------------------------------------------------------------- //
  // ----------------- Inter-process fragments extraction ----------------- //
  // ---------------------------------------------------------------------- //

  /**
   * Given the vtkPolyData (fragPoly) storing the fragments extracted from a
   * single process (assigned with either one block or multiple blocks), this
   * function groups / re-arranges the polygons by the (intra-process inter-
   * block) fragment Id, determines the maximum number (maxFsize) of polygons
   * (belonging to a fragment) with the same fragment Id, and exports the re-
   * arranged polygons with the associated fragment Ids and all integrated cell
   * data attributes to the output inter-process vtkPolyData (procPoly) where
   * each point is assigned with a global inter-process point Id (through a
   * global point locator 'gPtIdGen') as the point data attribute used later
   * for combining the fragments across multiple processes.
   */
  void CreateInterProcessPolygons(vtkPolyData* fragPoly, vtkPolyData* procPoly,
    vtkIncrementalOctreePointLocator* gPtIdGen, int& maxFsize);

  /**
   * Given a number (numProcs) of vtkPolyData objects (procPlys) storing the
   * (initial) fragments extracted from multiple processes and an array of
   * values (maxFsize) storing the maximum number of faces that an initial
   * fragment may contain on each process (used to allocate appropriate memory
   * for buffering the polygons of an initial fragment), this function pushes
   * these polygons to the face hash on a per fragment basis (the fragment Ids
   * are used to group polygons into initial fragments), detects internal faces
   * / polygons, and registers polygons to the inter-process equivalence set by
   * the intermediate fragment Id. For each internal polygon / face shared by
   * two initial fragments, an entry is added to the equivalence set in a way to
   * associate the fragment Ids assigned to the two initial fragments' polygons.
   * Note each face in the face hash is assigned with a process Id that is used
   * later to retrieve the source vtkPolyData for the target complete polygon
   * if the face remains in the face hash after removing internal faces.
   */
  void AddInterProcessPolygonsToFaceHash(vtkPolyData** procPlys, int* maxFsize, int numProcs);

  /**
   * With the inter-process equivalence set resolved, inter-process fragment
   * Ids resolved, and cell data attributes integrated by the fragment Id, this
   * function retrieves the face hash for the 'valid' faces (as opposed to the
   * removed internal faces), accesses their complete version (according to the
   * information stored in the face) from the vtkPolyData objects (procPlys)
   * that store the polygons of the fragments extracted from multiple (numProcs)
   * processes, and exports these complete 'valid' polygons to the output
   * vtkPolyData (polyData) while attaching the index of the material (partIndx,
   * corresponding to the volume fraction array used to extract the fragments)
   * to each polygon as a cell data attribute. In a word, this function exports
   * a combined version of the fragments extracted from multiple processes (of
   * which each is assigned with either one block or multiple blocks).
   */
  void GenerateOutputFromMultiProcesses(
    vtkPolyData** procPlys, int numProcs, unsigned char partIndx, vtkPolyData* polyData);

private:
  vtkRectilinearGridConnectivity(const vtkRectilinearGridConnectivity&) = delete;
  void operator=(const vtkRectilinearGridConnectivity&) = delete;
};

#endif
