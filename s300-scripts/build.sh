#!/usr/bin/env bash

build_command="$0 ${@}"
cmake_options=""
interactive="false"
targets=(arm buildroot x86)
targets_str="${targets[@]}"
target=""
boards=(zeus300s gaia300d)
boards_str="${boards[@]}"
default_board="zeus300s"
board=""

parse_args() {
    # read the options
    local args
    args="$(getopt --options t:b:ih --longoptions target,board:,interactive,help --quiet -- $@)"
    if [ "${?}" != 0 ]; then
        echo "Invalid option given!" >&2
        print_help
        return 1
    fi
    eval set -- "${args}"

    # extract options and their arguments into variables
    while true; do
        case "${1}" in
            -t | --target)
                target="${2}"
                shift 2
                ;;
            -b | --board)
                board="${2}"
                shift 2
                ;;
            -i | --interactive)
                interactive="true"
                shift
                ;;
            -h | --help)
                print_help
                exit 0
                ;;
            --)
                shift
                cmake_options="${@}"
                break
                ;;
        esac
    done

    if [ -z "${target}" ]; then
        echo "Error: specify a target with -t" >&2
        print_help
        return 1
    fi

    if [ -z "${board}" ]; then
        echo "Warning: No board specified with -b, falling back to \"${default_board}\"" >&2
        board=${default_board}
    fi

    if [[ ! "${targets[@]}" =~ "${target}" ]]; then
        echo "Target ${target} does not correspond to a valid target. Please select one of the following: ${targets_str// /, }" >&2
        return 1
    fi

    if [[ ! "${boards[@]}" =~ "${board}" ]]; then
        echo "Board ${board} does not correspond to a valid board. Please select one of the following: ${boards_str// /, }" >&2
        return 1
    fi

    return 0
}


print_help() {
    echo "usage: ${0} [-h] [-t TARGET] [-b BOARD] [-i] -- <cmake options>"
    echo
    echo "Build the application."
    echo
    echo "arguments:"
    echo "  -t TARGET, --target TARGET"
    echo "                        target to build, must be one of the following: ${targets_str// /, }"
    echo "  -b BOARD, --board BOARD"
    echo "                        select your target board: ${boards_str// /, }"
    echo
    echo "optional arguments:"
    echo "  -i, --interactive     open an interactive shell in the Docker image"
    echo "  -- cmake options      pass additional options to the cmake command"
    echo "  -h, --help            show this help message and exit"
    return 0
}


docker_cmd() {
    if [ "$(id -un)" == "jenkins" ]; then
        echo "Starting container in Jenkins"
        docker run \
            --interactive \
            --rm \
            --volumes-from $(cat /etc/hostname) \
            --env UID="$(id -u)" \
            --env GID="$(id -g)" \
            --env CONTAINER_NAME=${docker_image} \
            --workdir $(realpath "$(pwd)/${build_dir}") \
            ${docker_image} \
            "${@}"
    else
        echo "Starting container natively"
        docker run \
            --interactive  \
            --tty  \
            --rm \
            --volume $(pwd):/home/dev/app \
            --volume $(pwd)/${build_dir}:/home/dev/build \
            --volume ${BR2_CCACHE_DIR:-buildroot-ccache}:/var/lib/buildroot/.buildroot-ccache \
            --volume ${BR2_DL_DIR:-buildroot-dl}:/var/lib/buildroot/dl \
            --env CONTAINER_NAME=${docker_image} \
            --env UID="$(id -u)" \
            --env GID="$(id -g)" \
            --workdir /home/dev/build \
            ${docker_image} \
            "${@}"
    fi
    return "${?}"
}


start_build() {
    scriptpath="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
    pushd "${scriptpath}/.."

    # initialize directories
    if [ "$(id -un)" == "jenkins" ]; then
        source_dir=$(realpath "$(pwd)")
    else
        source_dir="/home/dev/app"
    fi

    # Initialize Docker images
    if [ -d "docker" ]; then
        . docker/.env
    else
        echo "Cannot source docker env file" >&2
        return 1
    fi

    # Initialize parameters
    case "${target}" in
        "arm")
            build_dir="build-arm"
            cmd="cmake ${source_dir} -DCMAKE_TOOLCHAIN_FILE=${source_dir}/cmake/toolchain/arm-none-eabi.cmake -DBOARD_NAME=${board} ${cmake_options} && make -j $(nproc)"
            docker_image="docker.barco.com/hdis/arm-none-eabi-gcc-sdk:${ARM_NONE_EABI_GCC_SDK_VERSION}"
            ;;
        "buildroot")
            build_dir="build-buildroot"
            cmd="/opt/tools/buildroot-sdk/bin/cmake ${source_dir} -DCMAKE_TOOLCHAIN_FILE=/opt/tools/buildroot-sdk/share/buildroot/toolchainfile.cmake ${cmake_options} && make -j $(nproc)"
            docker_image="docker.barco.com/hdis/buildroot-sdk:${BUILDROOT_SDK_VERSION}"
            ;;
        "x86")
            build_dir="build-x86"
            cmd="cmake ${source_dir} ${cmake_options} && make -j $(nproc)"
            docker_image="docker.barco.com/hdis/x86-sdk:${X86_SDK_VERSION}"
            ;;
    esac

    if [ ! -d "${build_dir}" ]; then
        mkdir -p "${build_dir}"
    fi

    if [ "${interactive}" == "true" ]; then
        docker_cmd "/bin/bash"
    else
        docker_cmd ${cmd}
    fi
    local ret="${?}"
    popd
    return "${ret}"
}


main() {
    parse_args ${@}
    if [ "${?}" != 0 ]; then
        return 1
    fi

    start_build
    return "${?}"
}


main ${@}
exit ${?}

