# Verification Script for Instance Cleanup Implementation

Write-Host "=== SECRET ENGINE - Instance Cleanup Verification ===" -ForegroundColor Cyan
Write-Host ""

# 1. Check if device is connected
Write-Host "1. Checking device connection..." -ForegroundColor Yellow
$devices = adb devices
if ($devices -match "device$") {
    Write-Host "   ✓ Device connected" -ForegroundColor Green
} else {
    Write-Host "   ✗ No device connected" -ForegroundColor Red
    exit 1
}

# 2. Clear logcat
Write-Host ""
Write-Host "2. Clearing logcat..." -ForegroundColor Yellow
adb logcat -c
Write-Host "   ✓ Logcat cleared" -ForegroundColor Green

# 3. Start the app
Write-Host ""
Write-Host "3. Starting the app..." -ForegroundColor Yellow
adb shell am start -n com.secretengine.game/.MainActivity
Start-Sleep -Seconds 3
Write-Host "   ✓ App started" -ForegroundColor Green

# 4. Capture initial screenshot
Write-Host ""
Write-Host "4. Capturing initial screenshot..." -ForegroundColor Yellow
adb shell screencap -p /sdcard/screenshot_initial.png
adb pull /sdcard/screenshot_initial.png ./screenshot_initial.png
Write-Host "   ✓ Screenshot saved: screenshot_initial.png" -ForegroundColor Green

# 5. Wait and capture logcat for instance cleanup messages
Write-Host ""
Write-Host "5. Capturing logcat (looking for instance cleanup messages)..." -ForegroundColor Yellow
Write-Host "   Waiting 10 seconds for app initialization..." -ForegroundColor Gray
Start-Sleep -Seconds 10

# Capture logcat
adb logcat -d > logcat_output.txt
Write-Host "   ✓ Logcat saved: logcat_output.txt" -ForegroundColor Green

# 6. Search for key messages
Write-Host ""
Write-Host "6. Analyzing logcat for key messages..." -ForegroundColor Yellow

$logContent = Get-Content logcat_output.txt -Raw

# Check for instance cleanup messages
if ($logContent -match "ClearAllInstances|Cleared all instances") {
    Write-Host "   ✓ Found instance cleanup messages" -ForegroundColor Green
    $matches = Select-String -Path logcat_output.txt -Pattern "ClearAllInstances|Cleared all instances"
    foreach ($match in $matches) {
        Write-Host "     - $($match.Line)" -ForegroundColor Cyan
    }
} else {
    Write-Host "   ⚠ No instance cleanup messages found yet" -ForegroundColor Yellow
}

# Check for level loading messages
if ($logContent -match "Loading level|Level loaded") {
    Write-Host "   ✓ Found level loading messages" -ForegroundColor Green
}

# Check for instance count messages
if ($logContent -match "totalInstances|instance.*count") {
    Write-Host "   ✓ Found instance count messages" -ForegroundColor Green
    $matches = Select-String -Path logcat_output.txt -Pattern "totalInstances=\d+|instances removed"
    foreach ($match in $matches | Select-Object -First 5) {
        Write-Host "     - $($match.Line)" -ForegroundColor Cyan
    }
}

# 7. Summary
Write-Host ""
Write-Host "=== Verification Complete ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Files generated:" -ForegroundColor Yellow
Write-Host "  - screenshot_initial.png" -ForegroundColor White
Write-Host "  - logcat_output.txt" -ForegroundColor White
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Review screenshot_initial.png to see the app running" -ForegroundColor White
Write-Host "  2. Review logcat_output.txt for detailed logs" -ForegroundColor White
Write-Host "  3. Try changing levels in the app to trigger cleanup" -ForegroundColor White
Write-Host ""
