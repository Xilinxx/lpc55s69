# General

This repository contains helper scripts for the s300, this repo should be added as a submodule to s300-application

# Build

## Checkout

    git clone ssh://git@git.barco.com:7999/hdis/s300-scripts.git
	git submodule add ssh://git@git.barco.com:7999/hdis/s300-scripts.git scripts

# Usage

## Setup and make for embedded platform

    ./build.sh -d target -b <build-output-directory>

## Setup and make for native platform run tests

    ./build.sh -d native -b <build-output-directory>

## Run interactively
It is possible to run the cmake and make commands manually in the build directory of the native buildd container

    ./build.sh -d interactive -b <build-output-directory>
 
## Code Coverage static analysis and push to sonarqube

Code Coverage is done using gcov and lcov tools and it is done for native platform.
By running script codeCoverage.sh, test will be runned and report generated. Report is
place in native-build/codeCoverage/index.html

    ./build.sh -d test -b <build-output-directory>
