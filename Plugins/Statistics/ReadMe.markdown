# ParaView Statistics Plugin

This plugin provides a way to use vtkStatisticsAlgorithm subclasses from within ParaView.
It has been developed against the ParaView trunk but should also work with ParaView 3.6.

## Building the plugin

To create the plugin, run

    mkdir build
    cd build
    cmake -DParaView_DIR:PATH=/path/to/ParaViewBuildDir ..
    make
    env PV_PLUGIN_PATH=`pwd` /path/to/ParaViewBuildDir/bin/paraview

You should then see two new entries in the Filters menu bar:

+ Contingency Statistics
+ Descriptive Statistics
+ Multicorrelative Statistics
+ Principal Component Analysis
+ K-Means

In the near future, there will also be

+ Correlative Statistics

## Using the plugin

In the simplest use case, just
select a dataset in ParaView's pipeline browser,
create a statistics filter from the _Filter_ menu,
select the arrays you are interested in,
and click _Apply_.

![The object inspector.][fig:object inspector]

The default task for all of the filters (labeled "_Model and assess the same data_")
is to use a _small, random_ portion of your dataset to create a statistical model and
then use that model to evaluate _all_ of the data.
There are 4 different tasks that filters can perform:

1. "_Statistics of all the data_," which creates an output table (or tables) summarizing the entire input dataset;
1. "_Model a subset of the data_," which creates an output table (or tables) summarizing
   a randomly-chosen subset of the input dataset;
2. "_Assess the data with a model_," which adds attributes to the first input dataset using
   a model provided on the second input port; and
3. "_Model and assess the same data_," which is really just the 2 operations above applied to the same input dataset.
   The model is first trained using a fraction of the input data and then the entire dataset is assessed using that model.

When the task includes creating a model (i.e., tasks 2, and 4), you may adjust the fraction of the input dataset used for training.
You should avoid using a large fraction of the input data for training as you will then not be able to detect [overfitting][].
The _Training fraction_ setting will be ignored for tasks 1 and 3.

The first output of statistics filters is always the model table(s).
The model may be newly-created (tasks 1, 2, or 4) or a copy of the input model (task 3).
The second output will either be empty (tasks 1 and 2) or
a copy of the input dataset with additional attribute arrays (tasks 3 and 4).

## Filter-specific options

### Contingency Statistics

This filter computes contingency tables between pairs of attributes (a process known as _marginalization_).
This result is a tabular bivariate probability distribution which serves as a Bayesian-style prior model.
Data is assessed by computing
* the probability of observing both variables simultaneously;
* the probability of each variable conditioned on the other (the two values need not be identical); and
* the [pointwise mutual information (PMI)][pmi].
Finally, the summary statistics include the [information entropy][] of the observations.

### Descriptive Statistics

This filter computes the min, max, mean, raw moments M2--M4, standard deviation, skewness, and kurtosis
for each array you select.
The model is simply a univariate Gaussian distribution with the mean and standard deviation provided.
Data is assessed using this model by detrending the data (i.e., subtracting the mean) and
then dividing by the standard deviation.
Thus the assessment is an array whose entries are the number of standard deviations from the mean that each input point lies.
The _Signed Deviations_ option allows you to control whether the reported number of deviations will always be positive or
whether the sign encodes if the input point was above or below the mean.

![Descriptive stats in action!][fig:descriptive stats example]

### K-Means

This filter iteratively computes the center of k clusters in a space whose coordinates are specified by the arrays you select.
The clusters are chosen as local minima of the sum of square Euclidean distances from each point to its nearest cluster center.
The model is then a set of cluster centers.
Data is assessed by assigning a cluster center and distance to the cluster to each point in the input data set.

The _K_ option lets you specify the number of clusters.
The _Max Iterations_ option lets you specify the maximum number of iterations before the search for cluster centers terminates.
The _Tolerance_ option lets you specify the relative tolerance on cluster center coordinate changes between iterations before
the search for cluster centers terminates.

## Multicorrelative Statistics

This filter computes the covariance matrix for all the arrays you select plus the mean of each array.
The model is thus a multivariate Gaussian distribution with the mean vector and variances provided.
Data is assessed using this model by computing the [Mahalanobis distance][] for each input point.
This distance will always be positive.

## Principal Component Analysis

This filter performs additional analysis above and beyond the multicorrelative filter.
It computes the eigenvalues and eigenvectors of the covariance matrix from the multicorrelative filter.
Data is then assessed by projecting the original tuples into a possibly lower-dimensional space.
For more information see the Wikipedia entry on [principal component analysis][] (PCA).

The _Normalization Scheme_ option allows you to choose between no normalization &mdash; in which case
each variable of interest is assumed to be interchangeable (i.e., of the same dimension and units) &mdash;
or diagonal covariance normalization &mdash; in which case each <math>(i,j)</math>-entry of the covariance matrix
is normalized by <math>\sqrt{\mathrm{cov}(i,i)\mathrm{cov}(j,j)}</math> before the eigenvector decomposition is performed.
This is useful when variables of interest are not comparable but their variances are expected to be
useful indications of their full range, and their full ranges are expected to be useful normalization factors.

As PCA is frequently used for projecting tuples into a lower-dimensional space that preserves as much information as possible,
several settings are available to control the assessment output.
The _Basis Scheme_ allows you to control how projection to a lower dimension is performed.
Either no projection is performed (i.e., the output assessment has the same dimension as the number of variables of interest), or
projection is performed using the first N entries of each eigenvector, or
projection is performed using the first several entries of each eigenvector such that the "information energy" of the
projection will be above some specified amount E.

The _Basis Size_ setting specifies N, the dimension of the projected space when the _Basis Scheme_ is set to "_Fixed-size basis_".
The _Basis Energy_ setting specifies E, the minimum "information energy" when the _Basis Scheme_ is set to "_Fixed-energy basis_".

[information entropy]: http://en.wikipedia.org/wiki/Information_entropy
[Mahalanobis distance]: http://en.wikipedia.org/wiki/Mahalanobis_distance
[overfitting]: http://en.wikipedia.org/wiki/Overfitting
[pmi]: http://en.wikipedia.org/wiki/Pointwise_mutual_information
[principal component analysis]: http://en.wikipedia.org/wiki/Principal_component_analysis

[fig:object inspector]: images/objectInspector.png "The descriptive statistics object inspector."
[fig:descriptive stats example]: images/descriptiveStatisticsExample.png "An example of statistics in action!"

