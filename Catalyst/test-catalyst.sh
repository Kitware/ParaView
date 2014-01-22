#!/bin/sh

usage="$0 <srcdir> <builddir> [--install] [--no-configure] [--no-build] [--no-test] [--remove-dirs] --input <edition> [--input <edition>]... [<cmake arguments>...]"

# A handy function for handling errors.
die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -lt 3 ] && \
    die "$usage"

curscript="$0"
scriptdir="$( dirname "$curscript" )"
scriptdir="$( readlink -f "$scriptdir" )"

# Where the edition directories live.
editiondir="$scriptdir/Editions"

# The output directory to catalyze into.
src_output="$1"
shift

# The output directory to build into.
bin_output="$1"
shift

# Parse out --input parameters.
editions=
args=
remove=
no_configure=
no_build=
no_test=
install=
while [ "$#" -gt 0 ]; do
    case "$1" in
    --input)
        shift
        edition="$1"

        # Fail on non-existent editions.
        [ -d "$editiondir/$edition" ] || \
            die "The $edition edition does not exist"

        editions="$editions $edition"
        args="$args -i $editiondir/$edition"
        ;;
    --remove-dirs)
        remove=y
        ;;
    --no-configure)
        no_configure=y
        ;;
    --no-build)
        no_build=y
        ;;
    --no-test)
        no_test=y
        ;;
    --install)
        install=y
        ;;
    *)
        break
        ;;
    esac
    shift
done

# We need at least one edition to catalyze.
[ -z "$editions" ] && \
    die "Need at least one input"

# Remove directories if requested
[ -n "$remove" ] && \
    rm -rf "$src_output" "$bin_output"

# Catalyze.
mkdir -p "$src_output"
python "$scriptdir/catalyze.py" -r "$scriptdir/.." $args -o "$src_output" || \
    die "Failed to catalyze"

if [ -z "$no_test" ]; then
    # Build the testing directory.
    mkdir "$src_output/Testing"
    for edition in $editions; do
        [ -d "$editiondir/$edition/Testing" ] || continue

        cp -rv "$editiondir/$edition/Testing/"* "$src_output/Testing" || \
            die "Failed to copy tests for edition $edition"
    done
fi

[ -n "$no_configure" ] && exit 0

# Configure the catalyzed tree.
mkdir -p "$bin_output"
cd "$bin_output"
"$src_output/cmake.sh" "$@" "$src_output" || \
    die "Failed to configure the tree"

if [ -z "$no_test" ]; then
    # Configure the testing tree.
    mkdir -p "$bin_output/Testing"
    cd "$bin_output/Testing"
    cmake "$@" \
        "-DPVPYTHON_EXE=$bin_output/bin/pvpython" \
        "-DParaView_DIR=$bin_output" \
        "$src_output/Testing" || \
        die "Failed to configure tests"
fi

# Exit if a build is not wanted.
[ -n "$no_build" ] && exit 0

cd "$bin_output"
cmake --build . || \
    die "Failed to build"

# Test if wanted.
if [ -z "$no_test" ]; then
    cd "$bin_output/Testing"
    cmake --build . || \
        die "Failed to build tests"
    ctest -VV || \
        die "Tests failed"
fi

if [ -n "$install" ]; then
    cd "$bin_output"
    cmake --build . --target install || \
        die "Failed to install"
fi
