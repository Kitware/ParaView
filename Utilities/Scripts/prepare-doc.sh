#!/usr/bin/env bash

# error out if anything fails.
set -xe

PV_SRC=$1
PV_BUILD=$2
WORK_DIR=$3
UPDATE_LATEST="false"
PV_URL="https://www.paraview.org/paraview-docs/"

if [ -z "$PV_SRC" ] || [ -z "$PV_BUILD" ] || [ -z "$WORK_DIR" ]
then
    echo "Usage:"
    echo "  ./prepare-doc.sh /path/to/paraview/src /path/to/paraview/build /path/to/workdir [version]"
    echo ""
    exit 0
fi

# -----------------------------------------------------------------------------
# Source version
# -----------------------------------------------------------------------------
if [ -n "$4" ]
then
    VERSION="$4"
    UPDATE_LATEST="true"
else
    VERSION=$(git -C "$PV_SRC" describe)
fi

echo "$VERSION"

# -----------------------------------------------------------------------------
# Grab Web Content
# -----------------------------------------------------------------------------

mkdir -p "$WORK_DIR/paraview-docs"
cd "$WORK_DIR/paraview-docs"

curl -OL "${PV_URL}/versions"
curl -OL "${PV_URL}/versions.json"

# -----------------------------------------------------------------------------
# Copy Documentation to target
# -----------------------------------------------------------------------------
mkdir -p "${WORK_DIR}/paraview-docs/${VERSION}"
cp -r "${PV_BUILD}/doc/cxx" "${WORK_DIR}/paraview-docs/${VERSION}/cxx"
cp -r "${PV_BUILD}/doc/python" "${WORK_DIR}/paraview-docs/${VERSION}/python"
rm -rf "${WORK_DIR}/paraview-docs/${VERSION}/python/.doctrees"

# -----------------------------------------------------------------------------
# update available `versions` file.
# -----------------------------------------------------------------------------
cd "${WORK_DIR}/paraview-docs/"

if ! grep "^${VERSION}$" versions
then
  echo "${VERSION}" >> versions
fi

LC_ALL=C sort --version-sort --reverse --unique --output versions versions

# -----------------------------------------------------------------------------
# update available `versions.json` file.
# -----------------------------------------------------------------------------
GENERATED_TAGS="$(
  sed '
     # Every line
     {
       s/v\(.*\)/{ "value": "v\1",  "label": "\1"}/
     }
     # Every line but the last one
     $! {
       s/$/,/
     }' versions
     )"

cat << EOF > versions.json
[
{ "value": "nightly", "label": "nightly (development)" },
{ "value": "latest",  "label": "latest release (5.12.*)" },
$GENERATED_TAGS
]
EOF

# -----------------------------------------------------------------------------
# Commit to server
# -----------------------------------------------------------------------------

if [ "$PARAVIEW_DOC_UPLOAD" = "true" ]; then
    cd "${WORK_DIR}/paraview-docs/"

    if [ "$UPDATE_LATEST" = "true" ]; then
        if [ -d latest ]; then
            rm -rf latest
        fi
        cp -av "$VERSION"/ latest/
    fi
    if [ -f "${RSYNC_SSH_PRIVATE_KEY}" ]; then
        rsync -e "ssh -o StrictHostKeyChecking=accept-new -i $RSYNC_SSH_PRIVATE_KEY" -av --progress . kitware@web:
    fi
fi
