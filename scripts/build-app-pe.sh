#!/usr/bin/env bash
# Headless C# source -> nanoFramework PE (NFMRK1 v1 format), no Visual Studio / project system.
#
# Pipeline: csc (/nostdlib, referencing the nano CoreLibrary reference assembly + any --ref assemblies) -> a plain
# managed .dll -> the nanoFramework MetadataProcessor MSBuild task, invoked directly via a minimal <UsingTask>
# project, -> .pe. The processor is pinned to 3.0.100 (emits the v1 NFMRK1 format this interpreter reads; 4.0.x
# emits NFMRK2 and is rejected). With --skeleton, also regenerates the native interop skeleton for a host API.
#
# Usage: build-app-pe.sh [--library] [--ref <file.dll>]... [--skeleton <dir> <name> <project>] <out.pe> <Main.cs> [more.cs...]
set -euo pipefail

kind="exe"; refs=(); skel_dir=""; skel_name=""; skel_project=""
while [ $# -gt 0 ]; do
  case "$1" in
    --library) kind="library"; shift ;;
    --ref) refs+=("$2"); shift 2 ;;
    --skeleton) skel_dir="$2"; skel_name="$3"; skel_project="$4"; shift 4 ;;
    *) break ;;
  esac
done
out="${1:?usage: build-app-pe.sh [--library] [--ref X.dll]... <out.pe> <Main.cs> [more.cs...]}"; shift
sources=("$@"); [ ${#sources[@]} -gt 0 ] || { echo "no sources"; exit 2; }

root="$(cd "$(dirname "$0")/.." && pwd)"
tc="${root}/build/toolchain"; core="${tc}/corelib"; mdp="${tc}/mdp"
mkdir -p "${tc}"
if [ ! -f "${core}/mscorlib.dll" ]; then
  curl -fsL https://api.nuget.org/v3-flatcontainer/nanoframework.corelibrary.noreflection/1.17.11/nanoframework.corelibrary.noreflection.1.17.11.nupkg -o "${tc}/core.nupkg"
  mkdir -p "${core}"; unzip -j -o "${tc}/core.nupkg" lib/mscorlib.dll lib/mscorlib.pe -d "${core}" >/dev/null
fi
if [ ! -f "${mdp}/nanoFramework.Tools.MetadataProcessor.MsBuildTask.dll" ]; then
  curl -fsL https://api.nuget.org/v3-flatcontainer/nanoframework.tools.metadataprocessor.msbuildtask/3.0.100/nanoframework.tools.metadataprocessor.msbuildtask.3.0.100.nupkg -o "${tc}/mdp.nupkg"
  mkdir -p "${mdp}"; unzip -j -o "${tc}/mdp.nupkg" "lib/net6.0/*" -d "${mdp}" >/dev/null
fi

csc="$(find /usr/lib/dotnet /usr/share/dotnet -name csc.dll 2>/dev/null | head -1)"
[ -n "${csc}" ] || { echo "csc.dll not found (need a .NET SDK)"; exit 1; }

work="$(mktemp -d)"; trap 'rm -rf "${work}"' EXIT
refargs=(/reference:"${core}/mscorlib.dll")
hints="<Hints Include=\"${core}/mscorlib.dll\" />"
for r in "${refs[@]}"; do refargs+=(/reference:"${r}"); hints="${hints}<Hints Include=\"${r}\" />"; done

# The assembly's INTERNAL name (from the output name) must match how it is loaded + how any native interop table
# registers it (nanoCLR binds InternalCall methods by the PE's assembly name). Derive it from the output PE name.
asm="$(basename "${out}" .pe)"
dotnet exec "${csc}" /nostdlib /noconfig /optimize+ "/target:${kind}" /out:"${work}/${asm}.dll" "${refargs[@]}" "${sources[@]}"

out_abs="$(cd "$(dirname "${out}")" && pwd)/$(basename "${out}")"
skel_attr=""
if [ -n "${skel_dir}" ]; then
  mkdir -p "${skel_dir}"
  skel_attr="GenerateStubs=\"true\" GenerateSkeletonFile=\"${skel_dir}/${skel_name}\" GenerateSkeletonName=\"${skel_name}\" GenerateSkeletonProject=\"${skel_project}\""
fi
cat > "${work}/mdp.proj" <<XML
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <UsingTask TaskName="nanoFramework.Tools.MetadataProcessor.MsBuildTask.MetaDataProcessorTask"
             AssemblyFile="${mdp}/nanoFramework.Tools.MetadataProcessor.MsBuildTask.dll" />
  <Target Name="Build">
    <ItemGroup>${hints}</ItemGroup>
    <MetaDataProcessorTask Parse="${work}/${asm}.dll" LoadHints="@(Hints)" Compile="${out_abs}" ${skel_attr} />
  </Target>
</Project>
XML
dotnet msbuild "${work}/mdp.proj" /t:Build /nologo /v:quiet
# expose the managed .dll next to the PE so it can be a --ref for dependent assemblies.
cp "${work}/${asm}.dll" "${out_abs%.pe}.dll"
echo "wrote ${out} ($(stat -c%s "${out}") bytes, $(head -c6 "${out}"))"
