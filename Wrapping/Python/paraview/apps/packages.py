import importlib.util
from pathlib import Path
import sys
from typing import Dict

import paraview
from paraview import simple


def get_dependency_versions() -> Dict[str, str]:
    """Get a dictionary of dependency names and version strings

    This functions by iterating over all `site-packages` directories
    that contain `paraview`, and attempting to retrieve a version for
    each package in that directory.

    There is a blacklist that skips some packages, like `catalyst`,
    `vtk`, etc. This blacklist may need to be updated in future versions.

    There is also a remapping of some names to their PyPI names. Not
    every import name matches the name of the package on PyPI. For
    example, `import PIL` comes from `pillow` on PyPI. We remap these
    names where needed.

    It would be more preferable to use the `dist-utils` for all of these
    packages than the current method. However, it seems ParaView doesn't
    create or keep the `dist-utils` for its Python packages.
    """
    # Use the site path based upon the `paraview` module location
    site_path = Path(paraview.__file__).parent.parent

    blacklist = [
        '__pycache__',
        'catalyst',
        'catalyst_conduit',
        'mpl_toolkits',  # This gets installed automatically with matplotlib
        'paraview',
        'parflow',
        'pkg_resources',  # This doesn't provide a version number (ironically)
        'typing_extensions',  # No version number
        'versioneer',
        'vtk',
        'vtkmodules',
    ]

    # These have a different name on PyPI than their import name
    pypi_remappings = {
        'PIL': 'pillow',
        'dateutil': 'python-dateutil',
    }

    versions = {}
    for path in site_path.iterdir():
        if path.name.startswith('.'):
            continue

        if not path.is_dir() and not path.name.endswith('.py'):
            continue

        module_name = path.stem
        if module_name in blacklist:
            continue

        pypi_name = pypi_remappings.get(module_name, module_name)

        # Try to import it
        try:
            module = importlib.import_module(module_name)
            versions[pypi_name] = getattr(module, '__version__', 'Unknown')
        except ImportError:
            print(
                f'Failed to import "{module_name}". Skipping.',
                file=sys.stderr,
            )

    # Sort the modules by module name
    return {name: versions[name] for name in sorted(versions)}


def print_dependency_versions(versions: Dict[str, str]):
    """Print a list of dependencies similar to the format of `pip list`

    For example, this may look like the following:

        Package                    Version
        -------------------------- --------------------------
        cftime                     1.6.3
        contourpy                  1.1.0
        cycler                     0.10.0
        fontTools                  4.42.1
        h5py                       3.9.0
        kiwisolver                 1.4.5
        ...


    The `versions` input is obtained from `get_dependency_versions()`.

    """
    column_width = 26
    headers = ['Package', 'Version']
    header_str = ' '.join(f'{name:{column_width}}' for name in headers)
    header_str += '\n' + ' '.join('-' * 26 for _ in headers)

    print(header_str)
    for name, version in versions.items():
        print(f'{name:{column_width}} {version:{column_width}}')


def print_requirements_file(versions: Dict[str, str]):
    """Print a requirements.txt file from a versions dictionary.

    It is intended that this file will be installable like so:
    `pip -r ./requirements.txt`. The output appears like the following:

        # Python dependencies for paraview version 5.13.3
        cftime==1.6.3
        contourpy==1.1.0
        cycler==0.10.0
        fontTools==4.42.1
        h5py==3.9.0
        kiwisolver==1.4.5
        ...

    Some dependencies might have to be automatically built from source,
    since there may not be wheels for this particular Python version
    on PyPI. That will be handled automatically as long as all build
    dependencies are present.
    """
    version_string = simple.GetParaViewSourceVersion()
    header = f'# Python dependencies for {version_string}'

    print(header)
    for name, version in versions.items():
        print(f'{name}=={version}')


if __name__ == '__main__':
    # Get the dependency versions
    versions = get_dependency_versions()

    print_format = 'info'
    if '--format' in sys.argv:
        idx = sys.argv.index('--format')
        if len(sys.argv) > idx + 1 and sys.argv[idx + 1] == 'requirements':
            print_format = 'requirements'

    if print_format == 'info':
        # Print the dependency versions
        print_dependency_versions(versions)
    else:
        # Print the dependencies in a requirements.txt format
        print_requirements_file(versions)
