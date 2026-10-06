# Copia o SMS Plus (Game Gear e Master System) do projeto LatinhaColor para o firmware.
# Os arquivos ficam em firmware\cydboy_fw\src\smsplus, que o git ignora: o Z80 tem licenca
# "so uso nao comercial" e o SMS Plus e GPL v2, entao eles nao vao para o repositorio publico.
# Sem essa pasta o firmware compila normalmente, so sem Game Gear e Master System (HAS_SMS = 0).
param([string]$Latinha = "$HOME\OneDrive\Documents\arduino\00-Projects\LatinhaColor")

$origem  = Join-Path $Latinha 'src\smsplus'
$destino = Join-Path $PSScriptRoot '..\firmware\cydboy_fw\src\smsplus'

if (-not (Test-Path $origem)) { Write-Error "Nao achei $origem. Use -Latinha com a pasta do LatinhaColor."; exit 1 }

New-Item -ItemType Directory -Force $destino | Out-Null
Copy-Item (Join-Path $origem '*') $destino -Recurse -Force
$n = (Get-ChildItem $destino -File | Measure-Object).Count
Write-Host "$n arquivos copiados para $destino"

# O -O3 do firmware infla o Z80 e o cache de instrucoes da ESP32 (32 KB) nao da conta: o SMS Plus
# rodou a 5 quadros por segundo. Estes arquivos sao compilados com -Os (como no LatinhaColor).
foreach ($f in 'z80.c','sms.c','render.c','vdp.c','system.c','sn76496.c','latinha_state.c') {
    $p = Join-Path $destino $f
    $t = [IO.File]::ReadAllText($p)
    if ($t -notmatch 'GCC optimize') {
        $t = '#pragma GCC optimize ("Os")' + "`r`n" + $t
        [IO.File]::WriteAllText($p, $t)
    }
}
Write-Host 'Compilacao com -Os aplicada ao SMS Plus.'
