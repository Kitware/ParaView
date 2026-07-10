"""Mini-app that drives a ParaView Catalyst V2 'fides_conduit' channel.

Unlike paraview.demos.wavelet_miniapp (which uses the in-process Catalyst V1
bridge), this mini-app talks to the Catalyst V2 conduit ABI directly via the
catalyst SDK (``import catalyst``), because the 'fides_conduit' channel type is
dispatched inside catalyst_execute() and is not reachable through the bridge.

It builds a small dataset as an in-memory conduit node together with an inline
Fides JSON data model, hands it to ParaView over a 'fides_conduit' channel for
a few timesteps, and lets the attached analysis pipeline verify the result.

Usage:
    pvbatch fides_catalyst_miniapp.py --grid {unstructured,cellgrid} -s <script.py>
"""
import argparse
import numpy as np
import catalyst
import catalyst_conduit as conduit

# ---------------------------------------------------------------------------
# Geometry shared by both grids: two hexahedra sharing a face (12 pts, 2 cells)
# ---------------------------------------------------------------------------
POINTS = np.array([[0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0],
                   [0, 0, 1], [1, 0, 1], [1, 1, 1], [0, 1, 1],
                   [2, 0, 0], [2, 1, 0], [2, 0, 1], [2, 1, 1]], dtype=np.float64)
CONN = np.array([[0, 1, 2, 3, 4, 5, 6, 7], [1, 8, 9, 2, 5, 10, 11, 6]], dtype=np.int64)


# ---------------------------------------------------------------------------
# 'unstructured': a Fides data model with a single_type (hexahedron) cell set
# and explicit point coordinates, all sourced from the conduit node.
# ---------------------------------------------------------------------------
UNSTRUCTURED_SCHEMA = """{
  "unstructured_grid": {
    "data_sources": [ { "name": "source", "type": "conduit" } ],
    "coordinate_system": {
      "array": { "array_type": "basic", "data_source": "source", "variable": "points" }
    },
    "cell_set": {
      "cell_set_type": "single_type",
      "cell_type": "hexahedron",
      "data_source": "source",
      "variable": "connectivity"
    },
    "fields": [
      { "name": "temperature", "association": "points",
        "array": { "array_type": "basic", "data_source": "source", "variable": "temperature" } }
    ],
    "variables": {
      "points":       { "shape_path": "variables/points/shape",       "data_path": "variables/points/data" },
      "connectivity": { "shape_path": "variables/connectivity/shape", "data_path": "variables/connectivity/data" },
      "temperature":  { "shape_path": "variables/temperature/shape",  "data_path": "variables/temperature/data" }
    }
  }
}"""


def fill_unstructured(data, step):
    data['variables/points/shape'] = np.array([12, 3], dtype=np.int64)
    data['variables/points/data'] = POINTS.reshape(-1).copy()
    data['variables/points/dtype'] = 'double'
    data['variables/connectivity/shape'] = np.array([16], dtype=np.int64)
    data['variables/connectivity/data'] = CONN.reshape(-1).copy()
    data['variables/connectivity/dtype'] = 'int64'
    temp = (np.arange(12, dtype=np.float64) + step) * 10.0
    data['variables/temperature/shape'] = np.array([12], dtype=np.int64)
    data['variables/temperature/data'] = temp
    data['variables/temperature/dtype'] = 'double'


# ---------------------------------------------------------------------------
# 'cellgrid': a Fides cell_grid data model (DG hexahedra -> vtkCellGrid). The
# conduit node carries the data-source "attributes" (metadata strings/ints, at
# top-level paths) plus the value/connectivity variables. This layout mirrors
# what vtkFidesWriter emits for a vtkCellGrid, with the data source switched to
# type "conduit".
# ---------------------------------------------------------------------------
CELLGRID_SCHEMA = """{
  "cell_grid": {
    "data_sources": [ { "name": "source", "type": "conduit" } ],
    "cell_attributes": [
      { "name": "temperature", "data_source": "source" },
      { "name": "shape", "data_source": "source" }
    ]
  }
}"""


def fill_cellgrid(data, step):
    # data-source "attributes" (metadata)
    data['cell_types'] = 'vtkDGHex'
    data['vtkDGHex/shape'] = 'hexahedron'
    # geometry / shape attribute
    data['shape/space'] = 'R3'
    data['shape/components'] = 3
    data['shape/is_shape'] = 1
    data['shape/vtkDGHex/function_space'] = 'HGRAD'
    data['shape/vtkDGHex/basis'] = 'C'
    data['shape/vtkDGHex/order'] = 1
    data['shape/vtkDGHex/dof_sharing'] = 'CG'
    data['shape/vtkDGHex/values'] = 'coordinates/Points'
    data['shape/vtkDGHex/connectivity'] = 'vtkDGHex/conn'
    # temperature attribute
    data['temperature/space'] = 'R'
    data['temperature/components'] = 1
    data['temperature/is_shape'] = 0
    data['temperature/vtkDGHex/function_space'] = 'HGRAD'
    data['temperature/vtkDGHex/basis'] = 'C'
    data['temperature/vtkDGHex/order'] = 1
    data['temperature/vtkDGHex/dof_sharing'] = 'CG'
    data['temperature/vtkDGHex/values'] = 'vtkDGHex/temperature'
    data['temperature/vtkDGHex/connectivity'] = 'vtkDGHex/conn'
    # variables (data arrays) at default conduit paths variables/<name>/...
    data['variables/coordinates/Points/shape'] = np.array([12, 3], dtype=np.int64)
    data['variables/coordinates/Points/data'] = POINTS.reshape(-1).copy()
    data['variables/coordinates/Points/dtype'] = 'double'
    data['variables/vtkDGHex/conn/shape'] = np.array([2, 8], dtype=np.int64)
    data['variables/vtkDGHex/conn/data'] = CONN.reshape(-1).copy()
    data['variables/vtkDGHex/conn/dtype'] = 'int64'
    temp = (np.arange(12, dtype=np.float64) + step) * 10.0
    data['variables/vtkDGHex/temperature/shape'] = np.array([12], dtype=np.int64)
    data['variables/vtkDGHex/temperature/data'] = temp
    data['variables/vtkDGHex/temperature/dtype'] = 'double'


GRIDS = {
    'unstructured': (UNSTRUCTURED_SCHEMA, fill_unstructured),
    'cellgrid': (CELLGRID_SCHEMA, fill_cellgrid),
}


def initialize(scripts):
    node = conduit.Node()
    for i, s in enumerate(scripts):
        node['catalyst/scripts/script%d' % i] = s
    node['catalyst_load/implementation'] = 'paraview'
    catalyst.initialize(node)


def execute(schema, fill, step, time):
    node = conduit.Node()
    node['catalyst/state/timestep'] = step
    node['catalyst/state/time'] = time
    ch = node['catalyst/channels/grid']
    ch['type'] = 'fides_conduit'
    ch['schema'] = schema
    fill(ch['data'], step)
    catalyst.execute(node)


def finalize():
    catalyst.finalize(conduit.Node())


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--grid', required=True, choices=list(GRIDS))
    parser.add_argument('-s', '--script', action='append', default=[],
                        help='Catalyst analysis pipeline script(s).')
    parser.add_argument('-t', '--timesteps', type=int, default=3)
    args = parser.parse_args()

    schema, fill = GRIDS[args.grid]
    initialize(args.script)
    for step in range(args.timesteps):
        execute(schema, fill, step, float(step) * 0.5)
    finalize()


if __name__ == '__main__':
    main()
