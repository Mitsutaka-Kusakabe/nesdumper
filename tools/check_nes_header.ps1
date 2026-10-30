<#
.SYNOPSIS
    iNES Header Inspector and Modifier for NES ROM dumps.
.DESCRIPTION
    Inspects 16-byte iNES headers across files in a directory or a specific file.
    Displays Mapper, PRG size, CHR size, Mirroring, Battery, Format, and Size validation.
    Allows in-place patching of Mirroring and Mapper ID with automatic backup.
.EXAMPLE
    .\tools\check_nes_header.ps1 -Dir "roms"
    .\tools\check_nes_header.ps1 -Path "roms\twinbee.nes" -FixMirroring "Horizontal"
    .\tools\check_nes_header.ps1 -Path "roms\famista88.nes" -FixMapper 206
#>

[CmdletBinding()]
param (
    [Parameter(Position = 0)]
    [string]$Path,

    [Parameter()]
    [string]$Dir = "roms",

    [Parameter()]
    [ValidateSet("Horizontal", "Vertical")]
    [string]$FixMirroring,

    [Parameter()]
    [ValidateRange(0, 255)]
    [int]$FixMapper = -1
)

function Analyze-NesFile {
    param ([string]$FilePath)

    if (-not (Test-Path $FilePath)) {
        Write-Error "File not found: $FilePath"
        return $null
    }

    $fileInfo = Get-Item $FilePath
    $bytes = [System.IO.File]::ReadAllBytes($FilePath)

    if ($bytes.Length -lt 16) {
        Write-Warning "File too small (< 16 bytes): $FilePath"
        return $null
    }

    # iNES magic is "NES\x1A" -> 0x4E, 0x45, 0x53, 0x1A
    if ($bytes[0] -ne 0x4E -or $bytes[1] -ne 0x45 -or $bytes[2] -ne 0x53 -or $bytes[3] -ne 0x1A) {
        Write-Warning "Invalid iNES Magic mismatch: $FilePath"
        return $null
    }

    $prg16k = $bytes[4]
    $chr8k  = $bytes[5]
    $b6     = $bytes[6]
    $b7     = $bytes[7]

    $mirrorBit = $b6 -band 1
    $fourScreen = ($b6 -band 8) -ne 0
    $mirrorStr = if ($fourScreen) { "4-Screen" } elseif ($mirrorBit -eq 1) { "Vertical" } else { "Horizontal" }

    $hasBattery = ($b6 -band 2) -ne 0
    $hasTrainer = ($b6 -band 4) -ne 0

    $mapperLow = $b6 -shr 4
    $mapperHigh = $b7 -band 0xF0
    $mapper = $mapperHigh -bor $mapperLow

    $isNES2 = (($b7 -band 0x0C) -eq 0x08)
    $formatStr = if ($isNES2) { "NES 2.0" } else { "iNES 1.0" }

    $prgKB = $prg16k * 16
    $chrKB = $chr8k * 8

    $trainerBytes = 0
    if ($hasTrainer) { $trainerBytes = 512 }
    $expectedSize = 16 + ($prgKB * 1024) + ($chrKB * 1024) + $trainerBytes
    $sizeMatch = ($bytes.Length -eq $expectedSize)

    return [PSCustomObject]@{
        FileName  = $fileInfo.Name
        Mapper    = $mapper
        PRG_KB    = $prgKB
        CHR_KB    = $chrKB
        Mirroring = $mirrorStr
        Battery   = $hasBattery
        Format    = $formatStr
        SizeOK    = $sizeMatch
        RawHeader = [System.BitConverter]::ToString($bytes, 0, 16)
        Byte6     = ("0x{0:X2}" -f $b6)
        Byte7     = ("0x{0:X2}" -f $b7)
        FullPath  = $FilePath
    }
}

$targetFiles = @()
if ($Path) {
    $targetFiles += $Path
} elseif ($Dir) {
    if (Test-Path $Dir) {
        $targetFiles = (Get-ChildItem -Path $Dir -Filter "*.nes" | Select-Object -ExpandProperty FullName)
    } else {
        Write-Error "Directory not found: $Dir"
        return
    }
}

if (-not $targetFiles -or $targetFiles.Count -eq 0) {
    Write-Host "No .nes files found to inspect."
    return
}

# 修正モード (単一ファイル指定時)
if ($Path -and ($FixMirroring -or $FixMapper -ge 0)) {
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    $backupPath = [System.IO.Path]::ChangeExtension($Path, ".bak.nes")
    Copy-Item -Path $Path -Destination $backupPath -Force
    Write-Host ("Backup created: " + $backupPath)

    if ($FixMirroring) {
        if ($FixMirroring -eq "Vertical") {
            $bytes[6] = ($bytes[6] -bor 1) -band (-bnot 8)
        } else {
            $bytes[6] = ($bytes[6] -band (-bnot 1)) -band (-bnot 8)
        }
        Write-Host ("Mirroring fixed to " + $FixMirroring)
    }

    if ($FixMapper -ge 0) {
        $lowNibble = $FixMapper -band 0x0F
        $highNibble = $FixMapper -band 0xF0
        $bytes[6] = ($bytes[6] -band 0x0F) -bor ($lowNibble -shl 4)
        $bytes[7] = ($bytes[7] -band 0x0F) -bor $highNibble
        Write-Host ("Mapper ID fixed to " + $FixMapper)
    }

    [System.IO.File]::WriteAllBytes($Path, $bytes)
}

$results = @()
foreach ($f in $targetFiles) {
    $res = Analyze-NesFile -FilePath $f
    if ($res) {
        $results += $res
    }
}

Write-Host "=========================================================================================="
Write-Host "                           iNES Header Inspection Results                                 "
Write-Host "=========================================================================================="

$results | Format-Table -Property FileName, Mapper, PRG_KB, CHR_KB, Mirroring, Battery, Format, SizeOK -AutoSize

Write-Host "------------------------------------------------------------------------------------------"
Write-Host "Raw Headers:"
foreach ($r in $results) {
    $line = "[{0,-28}] Map:{1,-3} PRG:{2,3}KB CHR:{3,3}KB Mir:{4,-10} Raw:{5}" -f $r.FileName, $r.Mapper, $r.PRG_KB, $r.CHR_KB, $r.Mirroring, $r.RawHeader
    Write-Host $line
}
Write-Host "=========================================================================================="
