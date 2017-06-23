#!/bin/bash

function generate_ver() {
	MAJOR=`git tag | head -1 | awk '{split($0,a,"."); print a[1]}'`
	MINOR=`git tag | head -1 | awk '{split($0,a,"."); print a[2]}'`
	MICRO=`git rev-list HEAD --count`
	TAG=`git log | head -1 | awk '{printf("%s", substr($2, 0, 7))}'`

	if [ "$MAJOR" == "" ]; then
		MAJOR=0
		MINOR=0
	fi
	echo $MAJOR.$MINOR.$MICRO-$TAG
}

BASE=$(dirname $0)
VERSION=$(generate_ver)
MAJOR=`echo $VERSION | awk -F'[.-]' '{ print $1 }'`
MINOR=`echo $VERSION | awk -F'[.-]' '{ print $2 }'`
MICRO=`echo $VERSION | awk -F'[.-]' '{ print $3 }'`
TAG=`echo $VERSION | awk -F'[.-]' '{ print $4 }'`

echo "#ifndef __VERSION_H__
#define __VERSION_H__

#define VERSION_MAJOR   $MAJOR
#define VERSION_MINOR   $MINOR
#define VERSION_MICRO   $MICRO
#define VERSION_TAG	\"$TAG\"

#define VERSION         ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | (VERSION_MICRO))

#endif /* __VERSION_H__ */"
