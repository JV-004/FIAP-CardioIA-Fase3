# ============================================================
#  publish_to_github.ps1 - CardioIA Fase 3
#  Conecta ao repositorio remoto e realiza o git push seguro.
#
#  Uso: .\publish_to_github.ps1
# ============================================================

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host "  CardioIA Fase 3 - Script de Publicacao no GitHub" -ForegroundColor Cyan
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host ""

# ── 1. Verificar repositorio Git ────────────────────────────
Write-Host "[1/5] Verificando repositorio Git local..." -ForegroundColor Yellow
$gitStatus = git status 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERRO: Diretorio atual nao e um repositorio Git." -ForegroundColor Red
    exit 1
}
Write-Host "      [OK] Repositorio Git encontrado." -ForegroundColor Green

# ── 2. Mostrar remotes existentes ───────────────────────────
Write-Host ""
Write-Host "[2/5] Remotes configurados atualmente:" -ForegroundColor Yellow
git remote -v

# ── 3. Adicionar o remote JV-004 ────────────────────────────
Write-Host ""
Write-Host "[3/5] Configurando remote 'jv004'..." -ForegroundColor Yellow

$remoteUrl = "https://github.com/JV-004/FIAP-CardioIA-Fase3.git"
$allRemotes = git remote 2>&1
$existingRemote = $allRemotes | Where-Object { $_ -eq "jv004" }

if ($existingRemote -eq "jv004") {
    Write-Host "      Remote 'jv004' ja existe. Atualizando URL..." -ForegroundColor Yellow
    git remote set-url jv004 $remoteUrl
} else {
    git remote add jv004 $remoteUrl
    Write-Host "      [OK] Remote 'jv004' adicionado: $remoteUrl" -ForegroundColor Green
}

# ── 4. Verificar arquivos sensiveis ─────────────────────────
Write-Host ""
Write-Host "[4/5] Verificacao de seguranca..." -ForegroundColor Yellow

$sensitiveFiles = @("wokwi/config.h", ".env", ".env.local", ".env.production")
$found = $false

foreach ($file in $sensitiveFiles) {
    if (Test-Path $file) {
        Write-Host "      [AVISO] Arquivo sensivel detectado: $file" -ForegroundColor Red
        Write-Host "              Este arquivo esta no .gitignore e NAO sera enviado." -ForegroundColor Yellow
        $found = $true
    }
}

if (-not $found) {
    Write-Host "      [OK] Nenhum arquivo sensivel fora do .gitignore detectado." -ForegroundColor Green
}

# Verificar se config.h esta em staging
$stagedFiles = git diff --cached --name-only 2>&1
if ($stagedFiles -match "config\.h") {
    Write-Host ""
    Write-Host "  [CRITICO] config.h esta em staging! Abortando push." -ForegroundColor Red
    Write-Host "            Execute: git reset HEAD wokwi/config.h" -ForegroundColor Yellow
    exit 1
}

# ── 5. Realizar o git push ──────────────────────────────────
Write-Host ""
Write-Host "[5/5] Enviando commits para o GitHub (JV-004)..." -ForegroundColor Yellow
Write-Host "      Branch: master -> jv004/master" -ForegroundColor Gray
Write-Host ""

git push jv004 master

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "======================================================" -ForegroundColor Green
    Write-Host "  [SUCESSO] Push realizado com sucesso!" -ForegroundColor Green
    Write-Host "  Link: https://github.com/JV-004/FIAP-CardioIA-Fase3" -ForegroundColor Green
    Write-Host "======================================================" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "======================================================" -ForegroundColor Red
    Write-Host "  [FALHA] Erro no push. Verifique:" -ForegroundColor Red
    Write-Host "     1. Suas credenciais do GitHub (token PAT)" -ForegroundColor Red
    Write-Host "     2. Se voce tem acesso ao repositorio JV-004" -ForegroundColor Red
    Write-Host "     3. Se o repositorio remoto existe" -ForegroundColor Red
    Write-Host "======================================================" -ForegroundColor Red
    exit 1
}
