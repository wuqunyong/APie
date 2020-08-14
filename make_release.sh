#! /bin/bash

set -e   # Exit immediately if a simple command exits with a non-zero status.
set -x   # activate debugging from here

trap 'echo ">>> Something went wrong. Unable to generate the release package <<<"' ERR

echo "$0"

BASE_DIR="$(cd "$(dirname -- "$0")" ; pwd)"
echo "CurDir: $BASE_DIR"

SPEC=`ls *.spec`
Name=`awk '/__name/{printf $3; exit;}' "$SPEC"`
Version=`awk '/__ver/{printf $3; exit;}' "$SPEC"`
Release=`awk '/__rel/{printf $3; exit;}' "$SPEC"`

ZipName="${Name}-${Version}-${Release}"
PackageName="${ZipName}.x86_64.rpm"

echo "Name: $Name"
echo "Version: $Version"
echo "Release: $Release"
echo "PackageName: $PackageName"

if [ ! -e ./build/x86_64/${PackageName} ]
then
	echo "./build/x86_64/${PackageName} not exist"
	exit 1
fi

if [ -e "$ZipName.x86_64.zip" ]
then
	/bin/rm -f "$ZipName.x86_64.zip"
	echo "rm -f $ZipName.x86_64.zip"
	exit 1
fi

TMP_DIR="`mktemp -d`"
echo "TMP_DIR=$TMP_DIR"

# make sure we won't remove the wrong files
echo $TMP_DIR | grep "^/tmp/" >/dev/null 2>&1 || exit 1
#trap "rm -rf $TMP_DIR" EXIT

mkdir -p "$TMP_DIR/$ZipName"
/bin/cp -rf ./releaseTemplate/* "$TMP_DIR/$ZipName"
/bin/cp -f ./build/x86_64/${PackageName} "$TMP_DIR/$ZipName/res"
/bin/cp -rf ./SQL/updates/* "$TMP_DIR/$ZipName/sql/updates"

if [ -n "$1" ]; then
    echo "first params：$1" 
    /bin/cp -rf ./SQL/base/* "$TMP_DIR/$ZipName/sql/base"
else
    echo "no params"
fi


pushd "$TMP_DIR"
sed -i "s/@@version@@/${Version}/g" ./$ZipName/config/install.conf
sed -i "s/@@release@@/${Release}/g" ./$ZipName/config/install.conf
sed -i "s/@@package_name@@/${PackageName}/g" ./$ZipName/shell/install.sh
sed -i "s/@@package_name@@/${PackageName}/g" ./$ZipName/shell/upgrade.sh

/usr/bin/zip -r "$ZipName.x86_64.zip" "$ZipName/"
/bin/mv "$ZipName.x86_64.zip" "${BASE_DIR}/"
popd


echo ">>> Generate successfully! <<<"

