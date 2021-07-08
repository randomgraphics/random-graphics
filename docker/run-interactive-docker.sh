#!/bin/bash
dir="$(cd $(dirname "${BASH_SOURCE[0]}");pwd)"
sdkroot="$(cd $(dirname "${BASH_SOURCE[0]}")/../..;pwd)"

# parse command line arguments
append_to_env_params=0
run_as="-u $UID:$GID"
for param in "$@"
do
  case "$param" in
    -r | --root)
        # run as root user
        echo "Enter the container as root user."
        run_as=
        ;;
    --)
        # any parameters after this ara forward to env.sh
        append_to_env_params=1
        ;;
    *)
        if [[ ${append_to_env_params} == 1 ]]; then
            env_params="${env_params} $param"
        else
            # unrecognized parameter
            echo "[ERROR] Unrecognized parameter: ${param}"
            exit -1
        fi
  esac
  shift
done

image=randomgraphics/vulkan-android:1.2.162-ndk-22.1.7171670-cuda-11.3.0-ubuntu-20.04

# run docker
docker run --rm \
    -v "/etc/group:/etc/group:ro" \
    -v "/etc/passwd:/etc/passwd:ro" \
    -v "/etc/shadow:/etc/shadow:ro" \
    -v "$HOME:$HOME" \
    ${run_as} \
    -it \
    $image ${dir}/docker-entrypoint.sh ${env_params}
