## Catalyst Pipeline Selection

Support for which Catalyst pipelines to execute has
been added. This includes Python script pipelines as
well as precompiled pipelines. The Python script
pipelines are configure with:
```
node['catalyst/scripts/aname'] = afile.py
```
where aname is the name of the pipeline. Precompiled
pipelines are configure with:
```
node["catalyst/pipelines/0/type"] = "io"
node["catalyst/pipelines/0/filename"] = "foo-%04ts.vtpd"
node["catalyst/pipelines/0/channel"] = "input"
```
where 0 is the name of the pipeline.

Pipelines can be selected by listing them under the
catalyst/state/pipelines node such as
```
node['catalyst/state/pipelines/0'] = 'aname'
```
Note that the value of the node (aname) is what matters
and not the name of the node (0).
It is possible to turn off all of the pipelines by
making pipelines a leaf node such as:
```
node['catalyst/state/pipelines'] = 0
```
If catalyst/state/pipelines does not exist, Catalyst
will, by default, execute all pipelines.
