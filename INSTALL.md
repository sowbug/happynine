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

* The [Native Client SDK](https://developers.google.com/native-client/sdk/download)
* `NACL_SDK_ROOT` pointing to the appropriate `pepper_NN` directory in the NaCl SDK
* [naclports](https://code.google.com/p/naclports/) with `./make_all.sh openssl` successfully executed on the local system (for what it's worth, I had to exclude the glibc versions to get it to build with unit tests.)
* [googletest](https://code.google.com/p/googletest/) (a.k.a. gtest)

Development Cycle
===

0. Be on Linux. Unfortunately for OSX/Windows developers, I couldn't build the NaCl version of OpenSSL anywhere but Linux.
1. Clone the repository.
2. In the `nacl_module` directory, `make -f TestMakefile` and then execute the various resulting `*_unit tests` executables.
3. Back in the top level of the source directory, `make`.
4. In Chrome, load the unpacked extension in `out/zip`.