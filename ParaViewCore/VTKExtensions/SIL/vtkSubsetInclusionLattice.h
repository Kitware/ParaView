/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSubsetInclusionLattice.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkSubsetInclusionLattice
 * @brief A directed acyclic graph to manage data hierarchy and relationships
 *        between hierarchy nodes.
 *
 * vtkSubsetInclusionLattice is designed to help readers
 * to describe hierarchical relationships in data-blocks in the file as well as
 * offer the user ability to select data-blocks to read (or not read).
 *
 * vtkSubsetInclusionLattice provides API to build a hierarchical description of
 * the data-blocks. It internally uses XML to store a directed-acyclic graph of
 * relations between blocks using nodes the XML. Parent-child links are directly
 * stored in XML as such. However, that is not sufficient for readers with
 * different ways of selecting blocks e.g. CGNS reader supports selecting block
 * using block names or families. To support such cases, the approach used is to
 * have a subtree for each "class" of control and then support cross links
 * between subtrees. The construction API, which includes `AddNode`, and
 * `AddCrossLink` allows building of such subtrees with cross links.
 *
 * The selection API allows selecting nodes. The selection API provided by
 * vtkSubsetInclusionLattice is fairly basic and not most intuitive for the
 * users. Readers are expected to subclass vtkSubsetInclusionLattice to provide
 * selection API relevant to the file format using the terminology natural
 * to the file format users.
 *
 * Typically, an instance of vtkSubsetInclusionLattice is constructed by the
 * reader using information in the file in `Reader::RequestInformation`. Then,
 * users can query the vtkSubsetInclusionLattice (or subclass) for information
 * about various classifications and then select blocks. In an ideal world,
 * `Reader::RequestInformation` will be called before user makes selection
 * requests i.e. the vtkSubsetInclusionLattice will be constructed using
 * construction API before using any of the selection API. However, that may not
 * always be the case. Hence, when `Select` or `Deselect` methods are called
 * with a path string as the argument, they add new nodes to the tree.
 *
 * @par Events
 *
 * vtkSubsetInclusionLattice fires the following events
 * \li `vtkCommand::ModifiedEvent` is fired whenever the structure is modified e.g. new
 * nodes or links are added, or state is restored using `Deserialize`.
 * \li `vtkCommand::StateChangedEvent` is fired whenever the selection state for
 * any node changes. The calldata points to the `int` which is the node's id.
 *
 * @sa vtkCGNSSubsetInclusionLattice
 */

#ifndef vtkSubsetInclusionLattice_h
#define vtkSubsetInclusionLattice_h

#include "vtkObject.h"

#include "vtkNew.h"                      // For vtkNew.
#include "vtkPVVTKExtensionsSILModule.h" // For export macro
#include "vtkSmartPointer.h"             // For vtkSmartPointer
#include <map>                           // for std::map
#include <vector>                        // for std::vector

class vtkInformation;
class vtkInformationObjectBaseKey;
class vtkInformationVector;

class VTKPVVTKEXTENSIONSSIL_EXPORT vtkSubsetInclusionLattice : public vtkObject
{
public:
  static vtkSubsetInclusionLattice* New();
  vtkTypeMacro(vtkSubsetInclusionLattice, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Initializes the SIL.
   */
  void Initialize();

  /**
   * Saves the SIL state to a string.
   */
  std::string Serialize() const;

  //@{
  /**
   * Deserializes the SIL state from a string.
   */
  bool Deserialize(const std::string& data);
  bool Deserialize(const char* data);
  //@}

  /**
   * Copies the contents from `other`.
   */
  void DeepCopy(const vtkSubsetInclusionLattice* other);

  /**
   * Merges with state from another instance.
   */
  void Merge(const std::string& state);

  /**
   * Merges with another instance of vtkSubsetInclusionLattice.
   */
  void Merge(const vtkSubsetInclusionLattice* other);

  /**
   * Add a node to the SIL.
   *
   * @param[in] name name for the node. There is no requirement that names are
   *                 unique.
   * @param[in] parent the id for the parent node. Default is the root of the
   *            SIL.
   * @return the id for the newly added node on success, or -1 on failure.
   */
  int AddNode(const char* name, int parent = 0);

  /**
   * Add a node to the SIL at the given path. Unlike path for `FindNode` which
   * can be incomplete, the path for here must be fully qualified i.e. start
   * with `/` and cannot have a `//`.
   */
  int AddNodeAtPath(const char* path);

  /**
   * A cross link is directed link between nodes in two sub-trees. Care must be
   * taken to avoid cycles. The current implementation does not check for cycles
   * although such a capability can be added in the future.
   *
   * @param[in] src id for the source node.
   * @param[out] dst id for the destination node.
   * @returns true on success, else false. May fail if either of the ids
   * specified are invalid.
   */
  bool AddCrossLink(int src, int dst);

  /**
   * Find the id for a node given a path expression to locate it.
   * Path can be specified as a fully or partially qualified, '/'-separated
   * location of the node.
   *
   * e.g.
   * \li `/Block0`
   * \li `/Assembly/Component0/Block0`
   * \li `/Assembly//Block0`
   * \li `//Block0`
   *
   * @note a valid patch must begin with `/` or a `//`.
   *
   * @param[in] path path for the node to find.
   * @return index of the node or -1 if not found.
   */
  int FindNode(const char* path) const;

  enum SelectionStates
  {
    NotSelected = 0,
    Selected,
    PartiallySelected,
  };

  /**
   * Get the current state for a specific node.
   *
   * @param[in] node the id for the node of interest.
   * @returns the status of the node. Will return `SelectionStates::NotSelected`
   * if the node is invalid.
   */
  SelectionStates GetSelectionState(int node) const;

  /**
   * Get the current state for a node given its path.
   *
   * @param[in] path the path for the node of interest (see `FindNode`).
   * @returns the status of the node. Will return `SelectionStates::NotSelected`
   * if the node is invalid.
   */
  SelectionStates GetSelectionState(const char* path) const
  {
    return this->GetSelectionState(this->FindNode(path));
  }

  /**
   * Select a node. A new node will be added at the path indicated if none
   * exists.
   */
  bool Select(const char* path);

  /**
   * Deselect a node. A new node will be added at the path indicated if none
   * exists.
   */
  bool Deselect(const char* path);

  //@{
  /**
   * Select/deselect nodes using their node id's. Returns true if the selection
   * state for the node was changed.
   */
  bool Select(int node);
  bool Deselect(int node);
  //@}

  /**
   * Clears all selection statuses.
   */
  void ClearSelections();

  //@{
  /**
   * Select/Deselect all nodes that match the path. This is useful to bulk
   * select/deselect nodes. Note, however, that this cannot be used to add new
   * nodes to the structure, unlike `Select` and `Deselect` calls.
   *
   * @returns true if a matching node was found and its state was changed.
   */
  bool SelectAll(const char* path);
  bool DeselectAll(const char* path);

  /**
   * Returns the time stamp for the most recent selection state change.
   */
  vtkGetMacro(SelectionChangeTime, vtkMTimeType);

  /**
   * Returns a vector of node ids for child nodes of a node. Note that
   * cross-links are not treated same as parent child relationships and hence
   * won't be returned here.
   */
  std::vector<int> GetChildren(int node) const;

  /**
   * Returns the id for the parent of a node.
   * -1 is returned for the parent of the root node.
   */
  int GetParent(int node, int* childIndex = nullptr) const;

  /**
   * Returns a node's name. nullptr if not valid.
   */
  const char* GetNodeName(int node) const;

  /**
   * This defines the type for selection states for nodes
   * exposed by API. The key is the path to a node and value is the status
   * (true if selected, false if not).
   */
  using SelectionType = std::map<std::string, bool>;

  /**
   * Get the paths for currently selected/deselected nodes. The list comprises of a subset
   * of paths for nodes on which `Select` or `Deselect` was called since the
   * last `Initialize`.
   */
  SelectionType GetSelection() const;

  /**
   * Set the paths for nodes to select. It will create current selection state
   */
  void SetSelection(const SelectionType& selection);

  /**
   * Overridden to modify SelectionChangeTime, since any time the SIL structure
   * is modified, it's akin to selection states being modified.
   */
  void Modified() override;

  /**
   * Key used to provide an instance of vtkSubsetInclusionLattice in the output
   * information during `vtkAlgorithm`'s `RequestInformation` pass.
   *
   * @ingroup InformationKeys
   */
  static vtkInformationObjectBaseKey* SUBSET_INCLUSION_LATTICE();

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkSubsetInclusionLattice* GetSIL(vtkInformation* info);
  static vtkSubsetInclusionLattice* GetSIL(vtkInformationVector* v, int i = 0);
  //@}

  /**
   * Creates a new clone of `other`.
   */
  static vtkSmartPointer<vtkSubsetInclusionLattice> Clone(const vtkSubsetInclusionLattice* other);

protected:
  vtkSubsetInclusionLattice();
  ~vtkSubsetInclusionLattice();

private:
  vtkSubsetInclusionLattice(const vtkSubsetInclusionLattice&) = delete;
  void operator=(const vtkSubsetInclusionLattice&) = delete;

  class vtkInternals;
  vtkInternals* Internals;
  friend class vtkInternals;

  void TriggerSelectionChanged(int node);

  vtkTimeStamp SelectionChangeTime;
};

#endif
