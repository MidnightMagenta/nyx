#! /bin/sh
set -euo pipefail

COMMIT_ID=$(git rev-parse HEAD | tr -d '\n')

cat >include/generated/version.h <<EOF
#define NYX_BUILD_ID "$COMMIT_ID"
EOF
