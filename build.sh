#!/bin/bash

set -eu

if [[ $# != 2 ]] || ! [[ $1 =~ ^(fs|fw)$ ]]; then
    >&2 printf 'Usage: %s <type> <model_name>\nWhere type is fw for firmware or fs for filesystem.\n' "$0"
    exit 1
fi

buildType="$1"
modelName="$2"

if [[ ! -f "data-env/$modelName" ]]; then
    >&2 printf 'Unknown model %s.\n' "$modelName"
    exit 1
fi

platformioEnvFile="data-env/$modelName"
platformioEnv=$(cat "$platformioEnvFile")
modelDataDir="data-models/$modelName"

if [[ $buildType == fw ]]; then
    pio run -e "$platformioEnv"
    cp .pio/build/"$platformioEnv"/firmware.bin /tmp/"$modelName".firmware.bin
    printf 'Built firmware to %s\n' "/tmp/${modelName}.firmware.bin"
else
    rm data
    ln -s "$modelDataDir" data
    pio run -e "$platformioEnv" -t buildfs
    cp .pio/build/"$platformioEnv"/littlefs.bin /tmp/"$modelName".littlefs.bin
    printf 'Built filesystem to %s\n' "/tmp/${modelName}.littlefs.bin"
fi

