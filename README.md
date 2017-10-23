#README

## What is this?
This is the native C android version of the Multi Path Tunnel (MPT) application. With this tool, you can run multipath VPN on android. For detailed information about the configuration and the software, see the PC version's guide. That is on the `master` branch of this github repo. 

## Important
This version is not maintained. Its require lots of effort because of the fast changes on the Android NDK and libssl. But, the source code is tweaked in order to get this compile and link with Bionic C (android C implementation) library.

## Compile steps:
* Make sure, Android NDK installed, and `ANDROID_NDK_ROOT` environment variable set to the proper NDK path
* Run `./configure_cmake` first
* Then run `cmake .`
* Run `make`

## Installation
Copy the whole folder into the `/data/local/tmp` directory on your device. In this folder you should have execute permisson for native binary applications. Also, you have to properly edited configuration files. See the PC version for config infos, works similar on android.

## Possible errors:
* May or may not compile with LLVM/Clang, not tested
* Possible linkage errors because of the precompiled `crypto` library compiled with GCC 4.9 NDK cross-compiler. Solution: download and compile openssl with your own cross-compiler than link it (or overwrite the existing `libcrypto.a` with your recently compiled version)
* On some device, stopping the program delete all the ip rules. This kill the networking on android. You have to reboot the device, which rebuild the ip rules and get the networking work again
* Before Android 6.0 the device stop the LTE interface if Wi-Fi available. Search for the HIPRIKeeper application for details. If you want to try this I recommend device with Android version >= 6.0

## Contact
For questions about the program or errors please contact me on my site [https://spyff.github.io](https://spyff.github.io) or drop me with a github issue here.

