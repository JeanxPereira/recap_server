export LDFLAGS="-Wl,--copy-dt-needed-entries"
cmake . -B build -DCMAKE_INSTALL_PREFIX=./AppDir/usr -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations -Wno-narrowing" || exit 1
cmake --build build || exit 1
cmake -DCOMPONENT=recap_server -Pbuild/cmake_install.cmake || exit 1

#rm -r build/data || true
#rm -r build/storage || true

mkdir -p build/storage/www
cp -r res/data build/
cp -r res/template_png build/storage/
cp -r res/static build/storage/www/


