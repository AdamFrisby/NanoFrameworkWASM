#!/usr/bin/env bash
# Headless C# source -> nanoFramework PE (NFMRK1 v1 format), no Visual Studio / project system.
#
# Pipeline: csc (/nostdlib, referencing the nano CoreLibrary reference assembly) -> a plain managed .dll ->
# the nanoFramework MetadataProcessor MSBuild task, invoked directly via a minimal <UsingTask> project, -> .pe.
#
# The MetadataProcessor version is pinned to 3.0.100 because it emits the v1 (NFMRK1) PE format the pinned
# interpreter + CoreLibrary 1.17.11 read; 4.0.x emits NFMRK2 and is rejected by this interpreter.
#
# Usage: scripts/build-app-pe.sh <Main.cs> <out.pe> [extra.cs ...]
set -euo pipefail

src="${1:?usage: build-app-pe.sh <Main.cs> <out.pe> [extra.cs ...]}"
out="${2:?missing <out.pe>}"
shift 2
extra=("$@")

root="$(cd "$(dirname "$0")/.." && pwd)"
tc="${root}/build/toolchain"
core="${tc}/corelib"
mdp="${tc}/mdp"
mkdir -p "${tc}"

if [ ! -f "${core}/mscorlib.dll" ]; then
  echo "fetching CoreLibrary.NoReflection 1.17.11 (reference assembly + PE)..."
  curl -fsL https://api.nuget.org/v3-flatcontainer/nanoframework.corelibrary.noreflection/1.17.11/nanoframework.corelibrary.noreflection.1.17.11.nupkg -o "${tc}/core.nupkg"
  mkdir -p "${core}"; unzip -j -o "${tc}/core.nupkg" lib/mscorlib.dll lib/mscorlib.pe -d "${core}" >/dev/null
fi
if [ ! -f "${mdp}/nanoFramework.Tools.MetadataProcessor.MsBuildTask.dll" ]; then
  echo "fetching MetadataProcessor 3.0.100 (NFMRK1 / net6.0)..."
  curl -fsL https://api.nuget.org/v3-flatcontainer/nanoframework.tools.metadataprocessor.msbuildtask/3.0.100/nanoframework.tools.metadataprocessor.msbuildtask.3.0.100.nupkg -o "${tc}/mdp.nupkg"
  mkdir -p "${mdp}"; unzip -j -o "${tc}/mdp.nupkg" "lib/net6.0/*" -d "${mdp}" >/dev/null
fi

csc="$(find /usr/lib/dotnet /usr/share/dotnet -name csc.dll 2>/dev/null | head -1)"
[ -n "${csc}" ] || { echo "csc.dll not found (need a .NET SDK)"; exit 1; }

work="$(mktemp -d)"
trap 'rm -rf "${work}"' EXIT

# 1. compile C# against the nano reference assembly only (no host BCL).
dotnet exec "${csc}" /nostdlib /noconfig /optimize+ /target:exe \
  /out:"${work}/app.dll" /reference:"${core}/mscorlib.dll" "${src}" "${extra[@]}"

# 2. MetadataProcessor: parse the managed assembly, resolve refs against the CoreLibrary, emit the PE.
out_abs="$(cd "$(dirname "${out}")" && pwd)/$(basename "${out}")"
cat > "${work}/mdp.proj" <<XML
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <UsingTask TaskName="nanoFramework.Tools.MetadataProcessor.MsBuildTask.MetaDataProcessorTask"
             AssemblyFile="${mdp}/nanoFramework.Tools.MetadataProcessor.MsBuildTask.dll" />
  <Target Name="Build">
    <ItemGroup><Hints Include="${core}/mscorlib.dll" /></ItemGroup>
    <MetaDataProcessorTask Parse="${work}/app.dll" LoadHints="@(Hints)" Compile="${out_abs}" />
  </Target>
</Project>
XML
dotnet msbuild "${work}/mdp.proj" /t:Build /nologo /v:quiet

echo "wrote ${out} ($(stat -c%s "${out}") bytes, $(head -c6 "${out}"))"
