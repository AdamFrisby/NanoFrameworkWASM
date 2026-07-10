#!/usr/bin/env bash
set -euo pipefail

destination="${1:-build/test-pes}"
mkdir -p "${destination}"

core_package="${destination}/nanoframework.corelibrary.noreflection.1.17.11.nupkg"
runtime_package="${destination}/nanoframework.runtime.native.1.7.11.nupkg"

curl -fL \
  https://api.nuget.org/v3-flatcontainer/nanoframework.corelibrary.noreflection/1.17.11/nanoframework.corelibrary.noreflection.1.17.11.nupkg \
  -o "${core_package}"
curl -fL \
  https://api.nuget.org/v3-flatcontainer/nanoframework.runtime.native/1.7.11/nanoframework.runtime.native.1.7.11.nupkg \
  -o "${runtime_package}"

printf '%s  %s\n' \
  6680d5402ed64fd2f4697a3fe48b578a8f53714e900cb26216cb471d386015f1 "${core_package}" \
  3c467d80ce40fe0000b8ccdc083c56ea83f6b6f06018f21f6a1b062123e7fd99 "${runtime_package}" \
  | sha256sum --check --status

unzip -j -o "${core_package}" lib/mscorlib.pe -d "${destination}"
unzip -j -o "${runtime_package}" lib/nanoFramework.Runtime.Native.pe -d "${destination}"
