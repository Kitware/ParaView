Subset Inclusion Lattice (SIL)                {#SubsetInclusionLattice}
================

Data is complex! As simulations have grown in complexity and capabilities, the
datasets they produce have also become complex. It is not uncommon to encounter
datasets comprising of multiple chunks or blocks e.g. Exodus, CGNS. The blocks may also
have relationships between them e.g. to describe an assembly, or block properties like
materials. When such relationships are available in the dataset, it is indeed
useful to present this information to the users so that they can choose to load
blocks in multiple ways. Subset inclusion lattice or SIL is the mechanism
available to ParaView readers to describe this information.

Currently, SIL is only used by readers to describe relations among blocks to
enable selecting of blocks to reader. The blocks need not corresponding to
actual data blocks in the file but in practice, they often do. If future, the
plan is to provide access to the SIL along with composite dataset produced by
the reader.


Generating a SIL
----------------

A reader or a data source that wants to expose an ability to select blocks to
read should generate a SIL. While a SIL can include information of data arrays
such as point data arrays or cell data arrays available in the file, currently
we keep that separate due to existing conventions.

In `vtkAlgorithm::RequestInformation`, every time the filename changes, you read
metadata from the file to build a SIL (vtkSubsetInclusionLattice).
vtkSubsetInclusionLattice provides API to initialize the SIL and then add nodes
with names in a tree structure with parent-child relationships. Directed links
between subtrees can also be added -- these are called cross links
(vtkSubsetInclusionLattice::AddCrossLink).

The top-level nodes i.e. nodes with parent 0 are treated as classifications or
categories e.g. **Assembly**, **Material** etc. There are no other special requirements
on what the nodes should be or how the relationships are defined. Typically, the
first category subtree is native structure for the file format. For example, in
CGNS, data is laid out in a tree-like structure comprising of bases, zones, and
patches. Hence the CGNS reader (vtkCGNSReader) chose to put this structure under
the first **Hierarchy** subtree. The structure reflects the file format layout.


    /Hierarchy/
               Base1/
                     Zone1/
                           Grid
                           Patch0
                           Patch1
                     Zone2/
                           Grid
                           Patch2
               Base2/
                ...


The zones and patches have an attribute called *family name*. The family is
often used to describe additional characteristics of the zone or patch e.g
*boundary-condition-1*, *side-wall*, etc. To allow the ability to select using
family names, vtkCGNSReader added a second category subtree for **Families**.

    /Families/
              Family1
              Family2
             ...

To link family names with nodes under the Hierarchy sub-tree, we use cross
links. Thus there's a cross link from each family name to the zone, grid or patch nodes
under the Hierarchy sub-tree.

An example CGNS SIL is as follows:

    /Hierarchy/
               base/
                    blk-1/
                          Grid
                          bc-1-wall
                          bc-2-inflow
                          bc-3-outflow
                          bc-4-symm-y
                          bc-5-symm-z1
                          bc-6-symm-z2
    /Families/
              bc-1-wall
                (cross-link: Hierarchy/base/blk-1/bc-1-wall)
              bc-2-inflow
                (cross-link: Hierarchy/base/blk-1/bc-2-inflow)
              bc-3-outflow
                (cross-link: Hierarchy/base/blk-1/bc-3-outflow)
              bc-4-symm-y
                (cross-link: Hierarchy/base/blk-1/bc-4-symm-y)
              bc-5-symm-z1
                (cross-link: Hierarchy/base/blk-1/bc-5-symm-z1)
              bc-6-symm-z2
                (cross-link: Hierarchy/base/blk-1/bc-6-symm-z2)
    /Grids/
           base/
                blk-1
                  (cross-link: Hierarchy/base/blk-1/Grid)
    /Patches/
             base/
                  blk-1
                    (cross-link: Hierarchy/base/blk-1/bc-1-wall)
                    (cross-link: Hierarchy/base/blk-1/bc-2-inflow)
                    (cross-link: Hierarchy/base/blk-1/bc-3-outflow)
                    (cross-link: Hierarchy/base/blk-1/bc-4-symm-y)
                    (cross-link: Hierarchy/base/blk-1/bc-5-symm-z1)
                    (cross-link: Hierarchy/base/blk-1/bc-6-symm-z2)

Now, when user selects a specific family, all nodes linked to it under the
Hierarchy subtree also need to be selected. vtkSubsetInclusionLattice manages
this internally.

While vtkSubsetInclusionLattice has API to select/deselect nodes by node ids as
well as user-friendly paths, it's inconvenient for a CGNS user to know how a
base or family is to be named. Hence vtkCGNSReader defines a subclass of
vtkSubsetInclusionLattice called vtkCGNSSubsetInclusionLattice and uses it to
store the SIL. vtkCGNSSubsetInclusionLattice has CGNS-friendly selection API
e.g. `vtkCGNSSubsetInclusionLattice::SelectBase(basename)`,
`vtkCGNSSubsetInclusionLattice::SelectFamily(familyname)` etc. Readers should
provide such subclasses as needed to make it easier on the users. At the same
time, the reader can expose API itself that simply forwards to the internal SIL
e.g. `vtkCGNSReader::SetBaseArrayStatus` simply calls
`vtkCGNSSubsetInclusionLattice::SelectBase` or
`vtkCGNSSubsetInclusionLattice::DeselectBase` on the SIL.

Once the SIL has been built, it should be placed in the output information
using `vtkSubsetInclusionLattice::SUBSET_INCLUSION_LATTICE()` key.

A typical `RequestInformation` implementation will have the following form:

@code{cpp}
int vtkReader::RequestInformation(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override
{

  if (/*filename changed*/)
  {
    /* read file meta-data*/

    // preserve current selection state on SIL, if any.
    auto sel = this->SIL->GetSelection();
    this->SIL->Initialize();

    /* build SIL */

    if (sel.empty())
    {
      /* setup some default selection to avoid reading nothing by default */
      this->SIL->SelectAll("//Grid");
    }
    else
    {
      this->SIL->SetSelection(sel);
    }
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkSubsetInclusionLattice::SUBSET_INCLUSION_LATTICE(), this->SIL);
  return 1;
}
@endcode

Using a SIL to define selections
--------------------------------

Readers that support a SIL for block selection need to expose the SIL so that
users can use it to make selections. Typically, you'd expose the SIL by a simple
`Get` method.

@code{cpp}
class vtkReader : public vtkAlgorithm
{
  ...
public:

  /**
  * Provide access to the SIL.
  */
  vtkSubsetInclusionLattice* GetSIL() const
  { return this->SIL; }

};
@endcode

For readers that define a subclass, you would want to change the `GetSIL`
signature to return an appropriate type rather than the generic
vtkSubsetInclusionLattice.

Additionally, readers should expose selection API for clearing selections and
selecting/deselecting nodes by path. This is useful not only for the users to
make easy selections, but also when defining ParaView Server-Manager
configuration XML.

@code{cpp}
class vtkReader : public vtkAlgorithm
{
public:
  ...

  void ClearBlockStatus()
  { this->SIL->ClearSelections(); }

  void SetBlockStatus(const char* path, bool enabled)
  {
    if (enabled)
    {
      this->SIL->Select(path);
    }
    else
    {
      this->SIL->Deselect(path);
    }
  }

  /**
  * This is presently need for ParaView to decide when
  * the SIL needs to be refetched. This is admittedly a cluncky mechanism
  * and is expected to be cleaned up in the future.
  */
  vtkIdType GetSILTimeStamp()
  {
    return static_cast<vtkIdType>(this->SIL->GetMTime());
  }
};
@endcode

The names of these methods need not be the same as shown in these snippets. You
are free to choose whatever naming scheme that makes most sense for your reader.


In a typical scenario, the user uses the reader as follows:

@code{cpp}

  vtkNew<vtkReader> reader;
  reader->SetFileName(...);

  // call UpdateInformation to read metadata
  reader->UpdateInformation();

  // now specify blocks to read.
  reader->Select(...);

@endcode

In practice, however, the user may call the `Select`/`Deselect` API before
calling `vtkAlgorithm::UpdateInformation`. To support this use-case, the SIL
needs to preserve selection state for nodes even before the structure has been
defined. This is currently supported by making Select/Deselect calls
automatically add nodes at the specified path if they don't exist. That combined
with the `RequestInformation` ensuring that selection state is preserved even
after the SIL has been reconstructed address this use-case.

Using SIL when reading blocks
-----------------------------

An important step to making SIL useful in a reader is for the reader to use the
SIL to decide which blocks are selected. This simply means that in `RequestData`
methond for the reader where it reads blocks, it should check with the SIL,
using `vtkSubsetInclusionLattice::GetSelectionState` method or a convenient variant
provided by the subclass (e.g.
`vtkCGNSSubsetInclusionLattice::ReadGridForZone`).


ServerManager configuration
---------------------------

Now that the reader supports SIL-based block selection, we need to expose it in
the ParaView UI. This is done by adding the following set of properties.

@code{xml}
  <SourceProxy name="Reader" class="vtkReader">

    ...

    <IdTypeVectorProperty name="SILTimeStamp"
      number_of_elements="1"
      default_values="0"
      information_only="1"
      command="GetSILTimeStamp">
        <Documentation>
          Indicates the timestamp when the SIL structured was modified.
          Useful to determine which to rebuild the SIL on the client.
        </Documentation>
    </IdTypeVectorProperty>

    <StringVectorProperty
       name="Blocks"
       command="SetBlockStatus"
       clean_command="ClearBlockStatus"
       repeat_command="1"
       number_of_elements_per_command="2"
       element_types="2 0">
         <SubsetInclusionLatticeDomain name="array_list" default_path="//Grid">
           <RequiredProperties>
             <Property function="TimeStamp" name="SILTimeStamp" />
           </RequiredProperties>
         </SubsetInclusionLatticeDomain>
    </StringVectorProperty>

    ...

  </SourceProxy>
@endcode

The key is to use the vtkSMSubsetInclusionLatticeDomain for the block selection
property. The **Properties** panel will use pqSubsetInclusionLatticeWidget and
pqSubsetInclusionLatticeTreeModel to render the vtkSubsetInclusionLattice in a
Qt widget.

Acknowledgements
----------------

The concept of subset inclusion lattice or SIL comes from
[VisIt](https://www.visitusers.org/index.php?title=Subset_Inclusion_Lattice).
The implementation, however, is independent of VisIt and there is no
expectation of compatibility, explicit or implied.
