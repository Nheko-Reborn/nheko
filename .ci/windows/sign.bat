@echo off

call "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" x64

@C:\smartcardtools\x64\scsigntool -pin %WINDOWS_SIGNING_KEY_PIN% sign /fd SHA256 /t http://timestamp.digicert.com /a /sha1 %WINDOWS_SIGNING_KEY_THUMBPRINT% nheko.msix >nul 2>&1
