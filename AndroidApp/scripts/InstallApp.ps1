# =========================================
# 自动编译 & 安装 APK 到 Android 设备
# （NativeActivity / 普通 Activity 通用）
# =========================================

# 获取脚本所在目录
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

# 项目根目录（脚本在 scripts/ 下）
$ProjectRoot = Join-Path $ScriptDir ".."
$Gradlew = Join-Path $ProjectRoot "gradlew.bat"
$ApkPath = Join-Path $ProjectRoot "app\build\outputs\apk\debug\app-debug.apk"

# 包名（必须与 applicationId 一致）
$PackageName = "com.beisent.oxygencrate"

# -------------------------------
# 检查 gradlew
# -------------------------------
if (-not (Test-Path $Gradlew)) {
    Write-Error "❌ 未找到 gradlew.bat：$Gradlew"
    exit 1
}

# -------------------------------
# 检查 adb
# -------------------------------
if (-not (Get-Command adb -ErrorAction SilentlyContinue)) {
    Write-Error "❌ 未找到 adb，请确保 Android SDK platform-tools 已加入 PATH"
    exit 1
}

# -------------------------------
# Step 1: 编译 APK
# -------------------------------
Write-Host "🚧 正在编译 APK..."
Push-Location $ProjectRoot
& $Gradlew assembleDebug
Pop-Location

if ($LASTEXITCODE -ne 0) {
    Write-Error "❌ APK 编译失败"
    exit 1
}

if (-not (Test-Path $ApkPath)) {
    Write-Error "❌ 未找到 APK 文件：$ApkPath"
    exit 1
}

Write-Host "✅ APK 编译成功"

# -------------------------------
# Step 2: 检查设备（稳健版）
# -------------------------------

$devices = @()

adb devices | ForEach-Object {
    if ($_ -match "^([a-zA-Z0-9\.\:\-]+)\s+device$") {
        $devices += $matches[1]
    }
}

if ($devices.Count -eq 0) {
    Write-Error "❌ 没有检测到可用设备，请确认：USB 调试已开启 / adb 已授权"
    exit 1
}

$device = $devices[0]
Write-Host "📱 检测到设备：$device"


# -------------------------------
# Step 3: 安装 APK
# -------------------------------
Write-Host "📦 正在安装 APK..."
adb -s $device install -r $ApkPath

if ($LASTEXITCODE -ne 0) {
    Write-Error "❌ APK 安装失败"
    exit 1
}

Write-Host "✅ APK 安装成功"

# -------------------------------
# Step 4: 启动应用（LAUNCHER）
# -------------------------------
Write-Host "🚀 正在启动应用..."
adb -s $device shell monkey -p $PackageName -c android.intent.category.LAUNCHER 1

if ($LASTEXITCODE -ne 0) {
    Write-Error "❌ 应用启动失败"
    exit 1
}

Write-Host "🎉 应用启动成功！"
