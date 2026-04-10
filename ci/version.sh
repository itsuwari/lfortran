#!/usr/bin/env bash

# This script extracts the project's current version from git using
# `git describe`, which determines the version based on the latest tag, such as:
#
#     $ git describe --tags --dirty
#     v0.6.0-37-g3878937f-dirty
#
# Each tag starts with "v", so we strip the "v", and the final version becomes:
#
#     0.6.0-37-g3878937f-dirty
#

set -ex

if version=$(git describe --tags --dirty 2>/dev/null); then
    if [[ "${version}" == v* ]]; then
        version="${version:1}"
    fi
else
    sha=$(git rev-parse --short HEAD)
    version="0.0.0-g${sha}"
    if ! git diff --quiet --ignore-submodules HEAD --; then
        version="${version}-dirty"
    fi
fi

echo $version > version
