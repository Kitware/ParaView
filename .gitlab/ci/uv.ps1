$erroractionpreference = "stop"

$version = "0.12.1"
$sha256sum = "8FCB0CB46E1229065E344758980924E569BEF5882EF45F46FADA8FB24E06B74A"
$filename = "uv-x86_64-pc-windows-msvc"
$tarball = "$filename.zip"

$outdir = $pwd.Path
$outdir = "$outdir\.gitlab"
$ProgressPreference = "SilentlyContinue"
Invoke-WebRequest -Uri "https://github.com/astral-sh/uv/releases/download/$version/$tarball" -OutFile "$outdir\$tarball"
$hash = Get-FileHash "$outdir\$tarball" -Algorithm SHA256
if ($hash.Hash -ne $sha256sum) {
    exit 1
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory("$outdir\$tarball", "$outdir")
