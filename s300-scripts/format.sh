#!/usr/bin/env bash

dry_run="false"
file_list=""
interactive="false"
location=""
current_dir="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

parse_args() {
    # read the options
    local args
    args="$(getopt --options inh --longoptions interactive,dry-run,help --quiet -- $@)"
    if [ "${?}" != 0 ]; then
        echo "Invalid option given!" >&2
        print_help
        exit 1
    fi
    eval set -- "${args}"

    # extract options and their arguments into variables
    while true; do
        case "${1}" in
            -i | --interactive)
                interactive="true"
                shift
                ;;
            -n | --dry-run)
                dry_run="true"
                shift
                ;;
            -h | --help)
                print_help
                exit 0
                ;;
            --)
                shift
                location="${1}"
                shift
                if [ "${#}" != 0 ]; then
                    echo "Invalid option given!" >&2
                    print_help
                    exit 1
                fi
                break
                ;;
        esac
    done

    if [ -z "${location}" ] && [ "${interactive}" != true ]; then
        echo "Error: specify a location to format, or start an interactive shell using -i" >&2
        print_help
        exit 1
    fi

    if [ "${interactive}" == false ] && [ ! -e "${location}" ]; then
        echo "Error: location to format does not exist!" >&2
        exit 1
    fi

    if [ "${interactive}" == true ] && [ ! -z "${location}" ]; then
        echo "Error: both interactive shell and location provided!" >&2
        exit 1
    fi

    return 0
}


print_help() {
    echo "usage: ${0} [-h] [-i] [-n] location"
    echo
    echo "Build the firmware for the Ares/Hera platform"
    echo
    echo "positional arguments:"
    echo "  location              location to lint files. This can either be a directory, or a file"
    echo
    echo "optional arguments:"
    echo "  -i, --interactive     start an interactive shell in the Docker container"
    echo "  -n, --dry-run         dry run"
    echo "  -h, --help            show this help message and exit"
}


docker_cmd() {
    # Read version of Docker image to use
    if [ -d "docker" ]; then
        . docker/.env
    else
        echo "Cannot source docker env file" >&2
        exit 1
    fi
    local docker_image="docker.barco.com/hdis/x86-sdk:${X86_SDK_VERSION}"

    if [ "$(id -un)" == "jenkins" ]; then
        echo "Starting container in Jenkins"
        docker run \
            --env UID="$(id -u)" \
            --env GID="$(id -g)" \
            --interactive \
            --rm \
            --volumes-from $(cat /etc/hostname) \
            --workdir $(realpath "$(pwd)") \
            ${docker_image} \
            "${@}"
    else
        echo "Starting container natively"
        docker run \
            --env UID="$(id -u)" \
            --env GID="$(id -g)" \
            --interactive \
            --rm \
            --tty  \
            --volume $(pwd):/home/dev/app \
            --workdir /home/dev/app \
            ${docker_image} \
            "${@}"
    fi
}


format() {
    if [ "${interactive}" == "true" ]; then
        echo "Starting interactive shell"
        docker_cmd "/bin/bash"
        exit "${?}"
    else
        # create file list
        clang_format_options=""
        if [ "${dry_run}" == "true" ]; then
            clang_format_options="${clang_format_options} --dry-run --Werror"
        else
            clang_format_options=" -i"
        fi
        echo "Formatting files with ${current_dir}/.clang-format"
        docker_cmd "find ${location} -iname \*.cpp -o -iname \*.hpp | xargs clang-format-12 -style=\"$(cat ${current_dir}/.clang-format)\" ${clang_format_options} --verbose"
        exit "${?}"
    fi
}


main() {
    parse_args ${@}
    if [ "${?}" != 0 ]; then
        exit 1
    fi

    format
    if [ "${?}" != 0 ]; then
        exit 1
    fi
}


main ${@}

