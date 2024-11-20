#!/usr/bin/env sh
#
# Modified version of: https://gist.github.com/stefanbuck/ce788fee19ab6eb0b4447a85fc99f447
#
#
# This script accepts the following parameters:
#
# * owner
# * repo
# * tag
# * filename
# * github_api_token
#
# Script to upload a release asset using the GitHub API v3.
#
# Example:
#
# upload-github-release-asset.sh github_api_token=TOKEN owner=stefanbuck repo=playground tag=v0.1.0 filename=./build.zip
#

set -e

# Validate settings.
[ "$TRACE" ] && set -x

for line in "$@"; do
  eval "$line"
done

# Define variables.
GH_API="https://api.github.com"
GH_REPO="$GH_API/repos/$owner/$repo"
GH_RELEASES="$GH_REPO/releases"
GH_TAGS="$GH_RELEASES/tags/$tag"
AUTH="Authorization: token $github_api_token"

if test "$tag" == "LATEST"; then
  GH_TAGS="$GH_REPO/releases/latest"
fi

# Validate token.
curl -o /dev/null -sH "$AUTH" $GH_REPO || { echo "Error: Invalid repo, token or network issue!";  exit 1; }

# Create a release from tag if it does not exist.
curl -s -d "{\"tag_name\":\"$tag\"}" -H "$AUTH" "$GH_RELEASES" > /dev/null

# Read asset tags.
response=$(curl -sH "$AUTH" $GH_TAGS)

# Get ID of the asset based on given filename.
eval "$(echo "$response" | grep -m 1 "id.:" | grep -w id | tr : = | tr -cd '[[:alnum:]]=')"
[ -n "$id" ] || { echo "Error: Failed to get release id for tag: $tag"; echo "$response" | awk 'length($0)<100' >&2; exit 1; }

# Upload asset
echo "Uploading asset... "

# Construct url
GH_ASSET="https://uploads.github.com/repos/$owner/$repo/releases/$id/assets?name=$(basename $filename)"

curl "$GITHUB_OAUTH_BASIC" --data-binary @"$filename" -H "$AUTH" -H "Content-Type: application/octet-stream" $GH_ASSET
