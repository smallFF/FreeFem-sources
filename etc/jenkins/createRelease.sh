#/bin/sh

TOKEN=$1
ORGANIZATION="FreeFem"
REPOSITORY="FreeFem-sources"

VERSION=`grep AC_INIT configure.ac | cut -d"," -f2 | tr - .`
RELEASE_TAG_NAME="$VERSION"
RELEASE_TARGET_COMMITISH="master"
RELEASE_NAME="FreeFEM v$VERSION"
RELEASE_BODY=""
RELEASE_DRAFT="false"
RELEASE_PRERELEASE="false"

RELEASE_PARAMETERS=$(printf '{"tag_name": "%s", "target_commitish": "%s", "name": "%s", "body": "%s.", "draft": %s, "prerelease": %s}' $RELEASE_TAG_NAME $RELEASE_TARGET_COMMITISH $RELEASE_NAME $RELEASE_BODY $RELEASE_DRAFT $RELEASE_PRERELEASE)
RELEASE=`curl -H "Authorization: token $TOKEN" --data "$RELEASE_PARAMETERS" 'https://api.github.com/repos/'$ORGANIZATION'/'$REPOSITORY'/releases'`
