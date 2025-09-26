$erroractionpreference = "stop"

$version = "1.1.42"
$full_version = "$version-20250904.1"
$sha256sum = "ED72208B340BB16FF34127281E5D37AAB4218E4E04A4E15B773475B3D2EBD045"
$filename = "xsltproc-$version-Windows-AMD64"
$tarball = "$filename.zip"

$outdir = $pwd.Path
$outdir = "$outdir\.gitlab"
$ProgressPreference = "SilentlyContinue"
Invoke-WebRequest -Uri "https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/xsltproc/v$full_version/$tarball" -OutFile "$outdir\$tarball"
$hash = Get-FileHash "$outdir\$tarball" -Algorithm SHA256
if ($hash.Hash -ne $sha256sum) {
    exit 1
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory("$outdir\$tarball", "$outdir")
Move-Item -Path "$outdir\$filename" -Destination "$outdir\xsltproc"
