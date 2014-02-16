Happynine
===

See README for overview.

Technologies
===

* [Chrome Apps](http://developer.chrome.com/apps/)
* [Native Client](https://developers.google.com/native-client/)
* [Bootstrap](http://getbootstrap.com/)
* [AngularJS](http://angularjs.org/)
* [jQuery](http://jquery.com/)
* C++, JavaScript, HTML, CSS

Installation Requirements for Development
===

I've listed the apt packages I needed for my bare-bones Debian-based Linux box (similar to Ubuntu 13.04). You might have to map them to different package names on other distributions.

* `NACL_SDK_ROOT` pointing to the appropriate `pepper_NN` directory (pepper_31 as of today) in the [Native Client SDK](https://developers.google.com/native-client/sdk/download)
* `GOOGLE_CLOSURE_JAR` pointing to compiler.jar from [Google Closure Compiler](https://developers.google.com/closure/compiler/)
* [naclports](https://code.google.com/p/naclports/) with `./make_all.sh openssl` successfully executed on the local system (for what it's worth, I had to exclude the glibc versions to get it to build with unit tests.)
* [googletest](https://code.google.com/p/googletest/) (a.k.a. gtest)
* `sudo apt-get install openjdk-7-jre libc6-i386 lib32stdc++6 gcc binutils make`

Development Cycle
===

0. Be on Linux (which could be a [crouton](https://github.com/dnschneid/crouton)-ized Chromebook). Unfortunately for OSX/Windows developers, I couldn't build the NaCl version of OpenSSL anywhere but Linux. If you're able to live with just frontend development, you might be able to get the prebuilt NaCl/PNaCl binaries from a neckbearded friend, then just stick them in the right place in the out/ directory. The rest of the development process can happen on any OS that's capable of installing a Java runtime environment.
1. Clone the repository.
2. In the `nacl_module` directory, `make -f TestMakefile` and confirm the unit tests pass.
3. Back in the top level of the source directory, `make`.
4. In Chrome, load the unpacked extension in `out/zip`.
