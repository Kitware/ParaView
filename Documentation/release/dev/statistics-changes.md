## Statistical analysis refactor

In order to work around several shortcomings of the existing statistics filters
in VTK and ParaView, we have refactored the filters. The new capabilities are
not identical to the old; in some ways there is less functionality because rare
use cases are not supported, but in many ways the functionality is more useful.
These are discussed below in detail.

### Filter inputs and outputs

Statistics filters previously used multiblock datasets to handle composite
input data and produced composite output models.
This was problematic when the newer partitioned dataset collection was introduced:
since statistical models were each themselves multiblock datasets of tables,
it became difficult to understand which portions of a hierarchy corresponded
to the input data and which were portions of the output model-table hierarchy.

Now, statistics filters:

+ Use partitioned dataset collections to handle input composite data.

+ Produce a new data type as output: a statistical model (which internally
  holds tables but does not behave as composite data).

+ Hold information about the algorithm which produced the model tables
  so that the underlying statistical algorithm can be recreated in order
  to further assess data or aggregate models.

These changes make organization of models much simpler to process.

### Models of composite data

Statistics filters previously operated on composite data by producing a
statistical model for each block. This caused issues when some ranks did not
have any data present and when input data was split into multiple pieces for
parallel processing (since pieces do not usually correspond to a relevant
partition of the data for analysis purposes). For example, computing descriptive
statistics on the `can.e.4.X` test data with 8 ranks would hang because the
reader does not produce 8 pieces. This same filter run on 4 ranks would succeed
but not be useful because each piece had its own model for fields such as
acceleration or displacement rather than a single model for each material.

Now, you may select whether composite data yields a single model for the
entire data object or a model per data-assembly node. Pieces inside a partitioned dataset
always have their statistical models merged into a single model for the
entire partition.

### Simplified operation

Previously, each statistical analysis filter provided multiple operational
capabilities that had to be selected (learn and derive a model (i.e., train),
train on a subset, assess data with an existing model, or train and assess
the same data).

Now, we aim to provide a separate filter for creating models,
a separate filter for assessing data, and a separate filter for
merging models of the same type.
This release of ParaView does not provide all of the above, but does
include simplified model generation.

The statistics filters that produce models no longer accept an input model;
they only accept data from which to produce samples for constructing a model.
To our knowledge, this feature (merging new data into an existing model from
an upstream filter) was not used and complicated construction of pipelines
for simpler use cases.

In the future, we intend to add support for merging statistical models as
a filter unto itself.

Assessing data in the face of models is not currently supported.
This will be addressed in the future by providing a separate filter to
perform assessment of data. (The filter will fail if the input data does
not have fields required by the statistical model to perform the assessment.)
