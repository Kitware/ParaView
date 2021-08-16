### Fetching data to the client in Python

`paraview.simple` now provides a new way to fetch all data from the
data server to  the client from a particular data producer. Using
`paraview.simple.FetchData` users can fetch data from the data-server locally
for custom processing.

Unlike `Fetch`, this new function does not bother applying any transformations
to the data and hence provides a convenient way to simply access remote data.

Since this can cause large datasets to be delivered the client, this must be
used with caution in HPC environments.
