#!/bin/bash

BASE=$(dirname $0)
VERSION=`$BASE/../bin/ver.sh`
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
