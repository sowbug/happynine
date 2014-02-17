Happynine
===

See README for overview.

Technologies
===

* [Chrome Apps](http://developer.chrome.com/apps/)
* [Native Client](https://developers.google.com/native-client/) including [PNaCl](http://www.chromium.org/nativeclient/pnacl)
* [Bootstrap](http://getbootstrap.com/)
* [AngularJS](http://angularjs.org/)
* [jQuery](http://jquery.com/)
* [Google Closure Compiler](https://developers.google.com/closure/compiler/)
* [OpenSSL](https://www.openssl.org/)
* [JsonCpp](http://jsoncpp.sourceforge.net/)
* C++, JavaScript, HTML, CSS

Development Requirements
===

* `NACL_SDK_ROOT` pointing to the appropriate `pepper_NN` directory (pepper_32 as of today) in the [Native Client SDK](https://developers.google.com/native-client/sdk/download)
* `GOOGLE_CLOSURE_JAR` pointing to compiler.jar from Google Closure Compiler
* `GTEST_DIR` pointing to the root directory of [googletest](https://code.google.com/p/googletest/) (a.k.a. gtest)
* [naclports](https://code.google.com/p/naclports/) with `./make_all.sh openssl` and `./make_all jsoncpp` successfully executed on the local system. For what it's worth, I had to exclude the glibc versions to get openssl to build with unit tests. And now that we're using PNaCl exclusively, you can probably get away with just `NACL_ARCH=pnacl ; make openssl`. In fact, if you really want to live dangerously, disable the unit tests in the make script and save a whole bunch of PNaCl translation time.

I've listed the apt packages I needed for my bare-bones Debian-based Linux box (similar to Ubuntu 13.04). You might need to map them to different package names on other distributions.

* Core development: `sudo apt-get install git libc6-i386 lib32stdc++6 gcc binutils make unzip zip`
* Closure Compiler: `sudo apt-get install openjdk-7-jre`
* Unit tests: `sudo apt-get install libssl-dev libjsoncpp-dev` Note that the unit tests currently build a pure Linux version of the code, rather than a NaCl nexe. This is why we need the dependent libraries installed both for the host system and from naclports. This isn't great for a lot of skew-related reasons, but so far it hasn't caused trouble.

Development Cycle
===

0. Be on Linux (which could be a [crouton](https://github.com/dnschneid/crouton)-ized Chromebook). Unfortunately for OSX/Windows developers, I couldn't build the NaCl version of OpenSSL anywhere but Linux. If you're able to live with just frontend development, you might be able to get the prebuilt NaCl/PNaCl binaries from a neckbearded friend, then just stick them in the right place in `out/`. The rest of the development process can happen on any OS that's capable of installing a Java runtime environment.
1. Clone the repository.
2. `pushd nacl_module; make -f TestMakefile; popd` and confirm the unit tests pass.
3. Back in the top level of the source directory, `make`.
4. In Chrome, load the unpacked extension in `out/zip`.
5. If you want to see how an official build is created for upload to the Chrome Web Store, `./build_official`, which does a bunch of tagging, version-bumping, and extra-clean building.

Crouton Development
===

Yes, it's quite possible to develop an app like Happynine on a $199 Chromebook. Overview:

* Switch to [developer mode](http://www.chromium.org/chromium-os/poking-around-your-chrome-os-device).
* Use Crouton to install a basic system (I used `sudo sh -e ~/Downloads/crouton -r raring -t core -e` and then installed all the requirements listed above).
* To have the build system write directly to the Downloads directory on ChromeOS, build with `OUTBASE=~/Downloads/h9 make`.
* Control-alt-shift &rarr; to flip over to ChromeOS.
* Load Unpacked Extension.
* Pick `h9/zip/` as the directory to load.
* You should see the app appear at the top of the extensions list.
