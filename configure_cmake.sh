cmake -DCMAKE_TOOLCHAIN_FILE=android.toolchain.cmake -DANDROID_NDK=$ANDROID_NDK_ROOT -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI="arm64-v8a" -DANDROID_NATIVE_API_LEVEL=android-24 -DANDROID_TOOLCHAIN_NAME=aarch64-linux-android-4.9 .
if [ -z "$ANDROID_NDK_ROOT" ]; then
    echo "Need to set ANDROID_NDK_ROOT environment variable to your NDK root directory. If its already set, export it."
    exit 1
fi 

