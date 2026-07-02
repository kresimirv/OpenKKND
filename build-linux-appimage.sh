#!/bin/bash
set -e

APP="OpenKKND"
ARCH="x86_64"
VERSION=1.0.0
APPDIR="AppDir"
OUTPUT="${OUTPUT:-${APP}_Linux_${ARCH}_v${VERSION//./_}.AppImage}"

# Build the game
echo "==> Building $APP..."
mkdir -p build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"

# Create AppDir structure
echo "==> Creating AppDir..."
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/lib"

# Copy binary
cp bin/$APP "$APPDIR/usr/bin/"

# Copy assets
cp -r bin/assets "$APPDIR/usr/bin/"

# Desktop file
cat > "$APPDIR/usr/share/applications/$APP.desktop" << DESKEOF
[Desktop Entry]
Type=Application
Name=Open Krush Kill 'n' Destroy
Comment=Remake of Krush Kill 'N' Destroy Xtreme
Exec=OpenKKND
Icon=$APP
Categories=Game;StrategyGame;
Terminal=false
DESKEOF

ln -sf "usr/share/applications/$APP.desktop" "$APPDIR/"

# Icon
if command -v convert &>/dev/null; then
    convert bin/assets/iconmute.bmp "$APPDIR/usr/share/icons/hicolor/256x256/apps/$APP.png"
else
    printf '\211PNG\r\n\032\n\0\0\0\rIHDR\0\0\1\0\0\0\1\0\10\2\0\0\0\x7f\x6b\x9a\xca\0\0\0\x01sRGB\0\xae\xce\x1c\xe9\0\0\0\x15IDAT\10\xd7\x63\xf8\x0f\x00\x01\x00\x01\xfe\x03\x18\0\0\0\0IEND\xae\x42\x60\x82' > "$APPDIR/usr/share/icons/hicolor/256x256/apps/$APP.png"
fi
ln -sf "usr/share/icons/hicolor/256x256/apps/$APP.png" "$APPDIR/"
ln -sf "usr/share/icons/hicolor/256x256/apps/$APP.png" "$APPDIR/.DirIcon"

# Write AppRun script
cat > "$APPDIR/AppRun" << 'APPRUN'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
exec "$HERE/usr/bin/OpenKKND" "$@"
APPRUN
chmod +x "$APPDIR/AppRun"

# Bundle shared libraries
echo "==> Bundling libraries..."
copy_libs() {
    local BIN="$1"
    for lib in $(ldd "$BIN" 2>/dev/null | grep "=> /" | awk '{print $3}'); do
        base=$(basename "$lib")
        if [ ! -f "$APPDIR/usr/lib/$base" ]; then
            cp -L "$lib" "$APPDIR/usr/lib/"
            copy_libs "$lib"
        fi
    done
}
copy_libs "$APPDIR/usr/bin/$APP"

# Remove libs that cause conflicts (libc, libm, libstdc++ etc.)
for lib in libc.so libm.so libdl.so libpthread.so librt.so libutil.so \
           libstdc++.so libgcc_s.so; do
    rm -f "$APPDIR/usr/lib/$lib"*
done

# Set rpath so libs are found
patchelf --set-rpath '$ORIGIN/../lib' "$APPDIR/usr/bin/$APP"

# Build AppImage
echo "==> Creating squashfs..."
mksquashfs "$APPDIR" "$APP.squashfs" -root-owned -noappend -quiet

echo "==> Downloading AppImage runtime..."
RUNTIME="runtime-$ARCH"
wget -qnc "https://github.com/AppImage/AppImageKit/releases/download/continuous/$RUNTIME"
chmod +x "$RUNTIME"

echo "==> Assembling AppImage..."
cat "$RUNTIME" "$APP.squashfs" > "$OUTPUT"
chmod +x "$OUTPUT"

# Clean up
rm -f "$APP.squashfs" "$RUNTIME"
rm -rf "$APPDIR"

echo "==> Done!"
ls -lh "$OUTPUT"
