#!/bin/bash
# Requires macOS and Xcode with the iPhoneOS SDK. No third-party engine.
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$(uname -s)" != Darwin ]]; then echo 'The IPA build requires macOS + Xcode.' >&2; exit 1; fi
python3 tools/make_demo.py
python3 tools/check-compiled.py
sdk="$(xcrun --sdk iphoneos --show-sdk-path)"
app='build/ios/Payload/ForgeWin.app'
mkdir -p "$app"
xcrun --sdk iphoneos clang++ -std=c++17 -O2 -arch arm64 \
  -isysroot "$sdk" -miphoneos-version-min=16.0 -fobjc-arc \
  -Wall -Wextra -Wno-unused-parameter \
  ios/main.mm -framework UIKit -framework Foundation -framework UniformTypeIdentifiers \
  -o "$app/ForgeWin"
cp ios/Info.plist "$app/Info.plist"
cp tests/sum55.exe "$app/sum55.exe"
cp tests/winapi71.exe "$app/winapi71.exe"
cp tests/compiled251.exe "$app/compiled251.exe"
plutil -lint "$app/Info.plist"
codesign --force --sign - "$app"
codesign --verify --strict "$app"
# Ad-hoc signature for TrollStore/jailbreak installation, not App Store/distribution signing.
rm -f build/ForgeWin_P2_iOS16.ipa
(cd build/ios && zip -qr ../ForgeWin_P2_iOS16.ipa Payload)
echo 'Created build/ForgeWin_P2_iOS16.ipa (ad-hoc signed; device validation required)'
