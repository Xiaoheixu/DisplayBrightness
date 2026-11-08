#!/bin/sh
set -eu
cd "$(dirname "$0")"
sdk=$(xcrun --sdk macosx --show-sdk-path)
headers="$PWD/vendor/MacKernelSDK-master/Headers"
lilu="$PWD/vendor/Lilu-master/Lilu"
configuration=${1:-Release}
case "$configuration" in
 Release) set -- -O2 ;;
 Debug) set -- -O0 -g -DDEBUG=1 -fdebug-prefix-map="$PWD"=. -fdebug-prefix-map="$(cd vendor && pwd -P)"=vendor ;;
 *) printf 'Usage: %s [Release|Debug]\n' "$0" >&2; exit 2 ;;
esac
build_dir="build/$configuration"
mkdir -p "$build_dir"
compile(){
 source=$1
 object=$2
 shift 2
 xcrun clang++ -arch x86_64 -std=c++14 -mkernel -fapple-kext -fno-exceptions -fno-rtti \
 -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -nostdinc -nostdinc++ -Wall -Wextra -Wno-unused-parameter -fno-stack-protector \
 -isysroot "$sdk" -I "$headers" -I "$headers/bsd" -I "$lilu" -I src \
 -DKERNEL -DKERNEL_PRIVATE -DPRODUCT_NAME=DisplayBrightness -DMODULE_VERSION=0.0.38 \
 "$@" -c "$source" -o "$object"
}
compile src/Plugin.cpp "$build_dir/Plugin.o" "$@" >"$build_dir/Plugin.log" 2>&1
compile src/Kmod.cpp "$build_dir/Kmod.o" "$@" >"$build_dir/Kmod.log" 2>&1
compile vendor/Lilu-master/Lilu/Library/plugin_start.cpp "$build_dir/plugin_start.o" "$@" >"$build_dir/plugin_start.log" 2>&1
xcrun clang++ -std=c++14 -Wall -Wextra -Werror -I src tests/identity-test.cpp -o "$build_dir/identity-test"
"$build_dir/identity-test"
printf 'All compilation units built; bundle linking is a separate check.\n'
bundle="$build_dir/DisplayBrightness.kext/Contents"
mkdir -p "$bundle/MacOS"
cp Info.plist "$bundle/Info.plist"
# Kext references are resolved by the kernel linker; this is packaging, not dependency acceptance.
xcrun clang++ -arch x86_64 -nostdlib -Wl,-kext -Wl,-undefined,dynamic_lookup \
 -Wl,-no_compact_unwind -Wl,-no_fixup_chains \
 "$build_dir/Plugin.o" "$build_dir/Kmod.o" "$build_dir/plugin_start.o" "$sdk/usr/lib/libkmod.a" \
 -o "$bundle/MacOS/DisplayBrightness" >"$build_dir/link.log" 2>&1
nm -u "$bundle/MacOS/DisplayBrightness" >"$build_dir/unresolved-symbols.txt"
plutil -lint "$bundle/Info.plist"
file "$bundle/MacOS/DisplayBrightness"
if [ "$configuration" = Debug ]; then
 xcrun dsymutil "$bundle/MacOS/DisplayBrightness" -o "$build_dir/DisplayBrightness.kext.dSYM"
fi
xcrun clang++ -std=c++14 -Wall -Wextra -Werror -I src tests/image-gate-test.cpp -o "$build_dir/image-gate-test"
"$build_dir/image-gate-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -I src tests/request-policy-test.cpp -o "$build_dir/request-policy-test"
"$build_dir/request-policy-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/protocol-test.cpp -o "$build_dir/protocol-test"
"$build_dir/protocol-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/hardware-policy-test.cpp -o "$build_dir/hardware-policy-test"
"$build_dir/hardware-policy-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/latest-request-test.cpp -o "$build_dir/latest-request-test"
"$build_dir/latest-request-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/retire-policy-test.cpp -o "$build_dir/retire-policy-test"
"$build_dir/retire-policy-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/read-recovery-test.cpp -o "$build_dir/read-recovery-test"
"$build_dir/read-recovery-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/state-sync-test.cpp -o "$build_dir/state-sync-test"
"$build_dir/state-sync-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/bootstrap-test.cpp -o "$build_dir/bootstrap-test"
"$build_dir/bootstrap-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/bootstrap-pending-test.cpp -o "$build_dir/bootstrap-pending-test"
"$build_dir/bootstrap-pending-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/refresh-target-test.cpp -o "$build_dir/refresh-target-test"
"$build_dir/refresh-target-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/responsive-read-test.cpp -o "$build_dir/responsive-read-test"
"$build_dir/responsive-read-test"

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src tests/verify-target-test.cpp -o "$build_dir/verify-target-test"
"$build_dir/verify-target-test"



for test in generic-identity compatibility topology vendor-invariance bootstrap-journal bootstrap-window bootstrap-reply endpoint-refresh bus-admission provider-policy boot-settings; do
 xcrun clang++ -std=c++14 -Wall -Wextra -Werror -fsanitize=address,undefined -I src "tests/$test-test.cpp" -o "$build_dir/$test-test"
 "$build_dir/$test-test"
done

python3 tests/check-brightness-only.py

xcrun clang++ -std=c++14 -Wall -Wextra -Werror -I src tests/reply-mode-test.cpp -o "$build_dir/reply-mode-test"
"$build_dir/reply-mode-test"
