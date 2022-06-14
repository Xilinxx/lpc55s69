#!/usr/bin/env bash 

build_command="$0 ${@}"
dry_run="false"
interactive="false"
package=""
tests=(clangtidy cmocka cppcheck cppclean rats unittests)
tests_str="${tests[@]}"
test_type=""
xml="false"


parse_args() {
    # read the options
    local args
    args="$(getopt --options t:inxh --longoptions test:,interactive,dry-run,xml,help --quiet -- $@)"
    if [ "${?}" != 0 ]; then
        echo "Invalid option given!" >&2
        print_help
        return 1
    fi
    eval set -- "${args}"

    # extract options and their arguments into variables
    while true; do
        case "${1}" in
            -t | --test)
                test_type="${2}"
                shift 2
                ;;
            -i | --interactive)
                interactive="true"
                shift
                ;;
            -n | --dry-run)
                dry_run="true"
                shift
                ;;
            -x | --xml)
                xml="true"
                shift
                ;;
            -h | --help)
                print_help
                exit 0
                ;;
            --)
                shift
                if [ "${#}" != 0 ]; then
                    echo "Invalid option given!" >&2
                    print_help
                    return 1
                fi
                break
                ;;
        esac
    done


    if [ -z "${test_type}" ]; then
        echo "Error: specify a test with -t" >&2
        print_help
        return 1
    fi
    if [[ ! "${tests[@]}" =~ "${test_type}" ]]; then
        echo "Test ${test_type} does not correspond to a valid test. Please select one of the following: ${tests_str// /, }" >&2
        return 1
    fi

    return 0
}


print_help() {
    echo "usage: ${0} [-h] [-t TARGET] [-i] [--xml]"
    echo
    echo "Test the application."
    echo
    echo "optional arguments:"
    echo "  -t TEST, --test TEST  test to perform, must be one of the following: ${tests_str// /, }"
    echo "  -x, --xml             generate output of the tests in xml junit format for jenkins"
    echo "  -i, --interactive     open an interactive shell in the Docker image"
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
            --workdir $(realpath "$(pwd)/${test_dir}") \
            ${docker_image} \
            "${@}"
    else
        echo "Starting container natively"
        docker run \
            --interactive  \
            --tty  \
            --rm \
            --volume $(pwd):/home/dev/app \
            --volume $(pwd)/${test_dir}:/home/dev/test \
            --env UID="$(id -u)" \
            --env GID="$(id -g)" \
            --workdir /home/dev/test \
            ${docker_image} \
            "${@}"
    fi
}


start_build() {
    scriptpath="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
    pushd "${scriptpath}/.."

    # initialize directories
    if [ "$(id -un)" == "jenkins" ]; then
        source_dir=$(realpath "$(pwd)")
        cmake_dir="${source_dir}"
    else
        source_dir="/home/dev/app"
        cmake_dir="${source_dir}"
    fi

    # seabert specific
    if [ -d "$(pwd)/seabert" ]; then
        echo "Seabert specific source dir"
        cmake_dir="${source_dir}"
        source_dir="${source_dir}/seabert"
        seabert="true"
    fi

    # Initialize Docker images
    if [ -d "docker" ]; then
        . docker/.env
    else
        echo "Cannot source docker env file" >&2
        return 1
    fi

    # Initialize parameters
    case "${test_type}" in
        "clangtidy")
            cmd="cmake ${cmake_dir} && make -j $(nproc) && find ${source_dir}/source/ -iname \*.cpp -o -iname \*.hpp | xargs -I {} clang-tidy -p bin"
            if [ "${dry_run}" == "false" ]; then
                cmd+=" -fix"
            fi
            cmd+=" {} | tee clangtidy_results.log"
            test_dir="test-clangtidy"
            docker_image="docker.barco.com/hdis/x86-sdk:${X86_SDK_VERSION}"
            ;;
        "cmocka")
            cmd="cmake ${cmake_dir} -DCMAKE_TOOLCHAIN_FILE=${cmake_dir}/cmake/toolchain/arm-none-eabi.cmake -DBOARD_NAME=\"zeus300s\" && make"
            if [ "${xml}" == "true" ]; then
                cmd+=" && CMOCKA_MESSAGE_OUTPUT=xml CMOCKA_XML_FILE=testcase_%g.xml make test"
            else
                cmd+=" && make test"
            fi
            test_dir="test-cmocka"
            docker_image="docker.barco.com/hdis/arm-none-eabi-gcc-sdk:${ARM_NONE_EABI_GCC_SDK_VERSION}"
            ;;
        "cppcheck")
            if [ "${xml}" == "true" ]; then
                cmd="mkdir -p analysis_results && cppcheck --enable=all -i ${source_dir}/ModEmbCpp -i ${source_dir}/plmod -j 12 --xml ${source_dir}/source 2> analysis_results/cppcheck_report.xml"
            else
                cmd="cppcheck --enable=all -i ${source_dir}/build -i \$(find ${source_dir} -type d -name ModEmbCpp | sed ':a; N; $!ba; s/\n/ -i /g') -i ${source_dir}/plmod -j 12 ${source_dir}/source"
            fi
            test_dir="test-cppcheck"
            docker_image="docker.barco.com/hdis/x86-sdk:${X86_SDK_VERSION}"
            ;;
        "cppclean")
            cmd="cppclean --include-path=${source_dir} ${source_dir}/source | tee cppclean.log"
            test_dir="test-cppclean"
            docker_image="docker.barco.com/hdis/x86-sdk:${X86_SDK_VERSION}"
            ;;
        "rats")
            cmd="mkdir -p analysis_results"
            if [ "${xml}" == "true" ]; then
                cmd+=" && rats --context --warning 3 --xml ${source_dir}/source > analysis_results/rats_report.xml"
            else
                cmd+=" && rats --context --warning 3 ${source_dir}/source"
            fi
            test_dir="test-rats"
            docker_image="docker.barco.com/hdis/x86-sdk:${X86_SDK_VERSION}"
            ;;
        "unittests")
            cmd="cmake ${cmake_dir} -DCMAKE_BUILD_TYPE=unittests ${source_dir} && make unittests -j $(nproc)"
            if [ "${seabert}" == "false" ]; then
                if [ "${xml}" == "true" ]; then
                    cmd+=" && mkdir analysis_results && ./bin/unittests --reporter junit --durations yes --success --nothrow --out analysis_results/test_report.xml"
                else
                    cmd+=" && ./bin/unittests"
                fi
            fi
            test_dir="test-unittests"
            docker_image="docker.barco.com/hdis/x86-sdk:${X86_SDK_VERSION}"
            ;;
    esac

    if [ ! -d "${test_dir}" ]; then
        mkdir -p "${test_dir}"
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
    if [ "${?}" != 0 ]; then
        return 1
    fi
}


main ${@}
exit ${?}

