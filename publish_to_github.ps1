# ============================================================
#  publish_to_github.ps1 — CardioIA Fase 3
#  Conecta ao repositório remoto e realiza o git push seguro.
#
#  Uso: .\publish_to_github.ps1
# ============================================================

$ErrorActionPreference = "Stop"

Write-Host ""
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host "  CardioIA Fase 3 — Script de Publicação no GitHub" -ForegroundColor Cyan
Write-Host "======================================================" -ForegroundColor Cyan
Write-Host ""

# ── 1. Verificar que estamos dentro do repositório Git ──────────────────────
Write-Host "[1/5] Verificando repositório Git local..." -ForegroundColor Yellow
$gitStatus = git status 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERRO: Diretório atual não é um repositório Git." -ForegroundColor Red
    exit 1
}
Write-Host "      ✅ Repositório Git encontrado." -ForegroundColor Green

# ── 2. Mostrar remotes existentes ───────────────────────────────────────────
Write-Host ""
Write-Host "[2/5] Remotes configurados atualmente:" -ForegroundColor Yellow
git remote -v

# ── 3. Adicionar o remote JV-004 (sem remover o origin existente) ───────────
Write-Host ""
Write-Host "[3/5] Configurando remote 'jv004'..." -ForegroundColor Yellow

$remoteUrl = "https://github.com/JV-004/FIAP-CardioIA-Fase3.git"
$existingRemote = git remote 2>&1 | Where-Object { $_ -eq "jv004" }

if ($existingRemote -eq "jv004") {
    Write-Host "      Remote 'jv004' já existe. Atualizando URL..." -ForegroundColor Yellow
    git remote set-url jv004 $remoteUrl
} else {
    git remote add jv004 $remoteUrl
    Write-Host "      ✅ Remote 'jv004' adicionado: $remoteUrl" -ForegroundColor Green
}

# ── 4. Verificar arquivos sensíveis antes do push ───────────────────────────
Write-Host ""
Write-Host "[4/5] Verificação de segurança..." -ForegroundColor Yellow

$sensitiveFiles = @("wokwi/config.h", ".env", ".env.local", ".env.production")
$found = $false

foreach ($file in $sensitiveFiles) {
    if (Test-Path $file) {
        Write-Host "      ⚠️  ARQUIVO SENSÍVEL detectado: $file" -ForegroundColor Red
        Write-Host "         Este arquivo está no .gitignore e NÃO será enviado." -ForegroundColor Yellow
        $found = $true
    }
}

if (-not $found) {
    Write-Host "      ✅ Nenhum arquivo sensível fora do .gitignore detectado." -ForegroundColor Green
}

# Verificar se config.h está em staging (situação crítica)
$stagedFiles = git diff --cached --name-only 2>&1
if ($stagedFiles -match "config\.h") {
    Write-Host ""
    Write-Host "  🚨 CRÍTICO: config.h está em staging! Abortando push." -ForegroundColor Red
    Write-Host "     Execute: git reset HEAD wokwi/config.h" -ForegroundColor Yellow
    exit 1
}

# ── 5. Realizar o git push ──────────────────────────────────────────────────
Write-Host ""
Write-Host "[5/5] Enviando commits para o GitHub (JV-004)..." -ForegroundColor Yellow
Write-Host "      Branch: master → jv004/master" -ForegroundColor Gray
Write-Host ""

git push jv004 master

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "======================================================" -ForegroundColor Green
    Write-Host "  ✅ Push realizado com sucesso!" -ForegroundColor Green
    Write-Host "  🔗 https://github.com/JV-004/FIAP-CardioIA-Fase3" -ForegroundColor Green
    Write-Host "======================================================" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "======================================================" -ForegroundColor Red
    Write-Host "  ❌ Falha no push. Verifique:" -ForegroundColor Red
    Write-Host "     1. Suas credenciais do GitHub (token PAT)" -ForegroundColor Red
    Write-Host "     2. Se você tem acesso ao repositório JV-004" -ForegroundColor Red
    Write-Host "     3. Se o repositório remoto existe e está vazio" -ForegroundColor Red
    Write-Host "======================================================" -ForegroundColor Red
    exit 1
}
