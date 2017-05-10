axmldec: Android Binary XML Decoder
===================================

## 1 Overview

[`AndroidManifest.xml`][Android App Manifest] in an [APK file][APK] is binary
encoded. This tool accepts either a binary or a text XML file and prints the
decoded XML to the standard output or a file. It also allows you to extract the
decoded `AndroidManifest.xml` directly from an APK file.

![](doc/overview.png)

Tools such as [Apktool] are designed to process the whole APK file including
the resource files for reverse engineering purpose. They may also need a Java
virtual machine to run. As a result, they are too slow for batch processing
many APK files just to get the XML information. In contrast, axmldec is
specialized for binary XML decoding and written in simple modern C++, so it
runs nicely within a shell script.

The [parser](include/jitana/util/axml_parser.hpp) is taken from [Jitana], a
graph-based static-dynamic hybrid DEX code analysis tool. You can use
`jitana::read_axml()` instead of the standard
`boost::property_tree::read_xml()` to read a binary XML file into
`boost::property_tree::ptree` ([Boost Property Tree][ptree]) in your C++
program.

## 2 Installation

### 2.1 macOS

You can install axmldec using [Homebrew]:
```sh
brew tap ytsutano/toolbox
brew install axmldec
```

Or, download the binary from [Releases].

### 2.2 Windows

Download the .exe file from [Releases].

### 2.3 Linux

Build the tool from the source code (see below).

## 3 Usage

### 3.1 Decoding `AndroidManifest.xml`

Pass the manifest file (either binary or text) to decodedecode:
```sh
axmldec -o output.xml AndroidManifest.xml
```

This will write the decoded XML to `output.xml`. You can specify the same
filename for input and output to decode the file in-place.

### 3.2 Decoding `AndroidManifest.xml` in an APK File

If an APK file is specified, axmldec automatically extracts and decodes
`AndroidManifest.xml`:
```sh
axmldec -o output.xml com.example.app.apk
```

### 3.3 Using the Standard Output

axmldec writes to the standard output if the `-o` option is not specified. This
is useful when additional processing is required. For example, you can extract
the package name from an APK file using xmllint:
```sh
axmldec com.example.app.apk | xmllint --xpath 'string(/manifest/@package)' -
```

## 4 Building

1. Install Boost, zlib, and CMake. Make sure you have a latest C++ compiler.

2. Clone axmldec and its submodule from GitHub:
    ```sh
    git clone --recursive https://github.com/ytsutano/axmldec.git
    ```

3. Compile axmldec:
    ```sh
    cmake -DCMAKE_BUILD_TYPE=Release . && make
    ```

## 5 Developer

- [Yutaka Tsutano] at University of Nebraska-Lincoln.

## 6 License

- See [LICENSE.md](LICENSE.md) for license rights and limitations (ISC).

[Yutaka Tsutano]: http://yutaka.tsutano.com
[Releases]: https://github.com/ytsutano/axmldec/releases
[Jitana]: https://github.com/ytsutano/jitana
[ptree]: http://www.boost.org/doc/libs/1_64_0/doc/html/property_tree.html
[Homebrew]: https://brew.sh
[APK]: https://en.wikipedia.org/wiki/Android_application_package
[Android App Manifest]: https://developer.android.com/guide/topics/manifest/manifest-intro.html
[Apktool]: https://ibotpeaches.github.io/Apktool/
