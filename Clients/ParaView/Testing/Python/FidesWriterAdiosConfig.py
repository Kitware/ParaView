# Verify that the AdiosConfigFile property on the FidesWriter proxy is propagated
# through to the underlying vtkFidesWriter

from paraview.simple import *
from paraview.vtk.util.misc import vtkGetTempDir
from os.path import join, exists
import os
import shutil
import sys

tmp_dir = vtkGetTempDir()


def remove_path(path):
    if os.path.isdir(path):
        shutil.rmtree(path, ignore_errors=True)
    elif os.path.exists(path):
        os.remove(path)


def write_config(config_path, engine):
    config_file_contents = f"""
<?xml version="1.0"?>
<adios-config>
    <io name="fides-write-io">
        <engine type="{engine}"/>
    </io>
</adios-config>
"""
    with open(config_path, "w") as f:
        f.write(config_file_contents)


def write_bp(bp_path, config_path, engine):
    remove_path(bp_path)
    remove_path(config_path)

    write_config(config_path, engine)

    wavelet = Wavelet()
    wavelet.UpdatePipeline()

    # SaveData should pick the FidesWriter (it's the only writer registered
    # for ".bp" files) and forward the AdiosConfigFile on to the writer proxy
    SaveData(bp_path, proxy=wavelet, AdiosConfigFile=config_path)


# When writing a BP5 file, the directory must contain the file "mmd.0"
bp5_path = join(tmp_dir, "testPVFidesBP5.bp")
bp5_config = join(tmp_dir, "pv_adios2_bp5.xml")

write_bp(bp5_path, bp5_config, "BP5")

assert exists(join(bp5_path, "data.0")), "BP5 write failed completely: data.0 is missing."

assert exists(join(bp5_path, "mmd.0")), "BP5 config was ignored: mmd.0 is missing."

# When writing a BP4 file, the directory *must not* contain the file "mmd.0"
bp4_path = join(tmp_dir, "testPVFidesBP4.bp")
bp4_config = join(tmp_dir, "pv_adios2_bp4.xml")

write_bp(bp4_path, bp4_config, "BP4")

assert exists(join(bp4_path, "data.0")), "BP4 write failed completely: data.0 is missing."
assert not exists(join(bp4_path, "mmd.0")), "BP4 config was ignored: mmd.0 was generated."

# Clean up
for artifact in (bp5_path, bp5_config, bp4_path, bp4_config):
    remove_path(artifact)
