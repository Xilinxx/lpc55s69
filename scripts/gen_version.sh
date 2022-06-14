#!/bin/bash

version_string=$(git describe --tags --long --dirty --always)
time_date_string=$(date "+%H:%M_%y-%m-%d")

echo "#ifndef _VERSION_H_" > include/version.h
echo "#define _VERSION_H_" >> include/version.h
echo "#define BOOTLOADER_VERSION_GIT \"${time_date_string}_${version_string}\"" >> include/version.h
echo "#define APPLICATION_VERSION_GIT \"APP-${time_date_string}_${version_string}\"" >> include/version.h
echo "#endif /* _VERSION_H_ */" >> include/version.h
