`axmldec`: Android Binary XML Decoder
=====================================

## 1 Overview

`AndroidManifest.xml` in an APK file can be binary encoded. This tool accepts
either a binary or a text XML file and prints the decoded XML to the standard
output or a file.

![](doc/overview.svg)

Tools such as [Apktool](https://ibotpeaches.github.io/Apktool/) are designed to
process the whole APK file including the resource files for reverse engineering
purpose. They may also need a Java virtual machine to run. As a result, it is
too slow for batch processing many APK files just get the XML information. This
tool ignores the resource files and written in simple modern C++ that runs
nicely within a shell script.

The [parser](include/jitana/util/axml_parser.hpp) is taken from
[Jitana](https://github.com/ytsutano/jitana), a graph-based static-dynamic
hybrid DEX code analysis tool. You can use `read_axml()` to read a binary XML
file into `boost::property_tree::ptree` ([Boost Property
Tree](http://www.boost.org/doc/libs/1_64_0/doc/html/property_tree.html)) in
your C++ program just like a normal XML file.

## 2 Installation

### 2.1 macOS

You can install `axmldec` using [Homebrew](https://brew.sh) (recommended):

    brew tap ytsutano/toolbox
    brew install axmldec

Or, download the binary from
[Releases](https://github.com/ytsutano/axmldec/releases).

### 2.2 Windows

Download the .exe file from
[Releases](https://github.com/ytsutano/axmldec/releases).

### 2.3 Linux

Build the tool from the source code (see below).

## 3 Usage

1. Use `unzip` to extract the manifest file from a APK file:

        unzip -j com.example.app.apk AndroidManifest.xml

2. Pass the manifest file (either binary or text) to decode:

        axmldec -o output.xml AndroidManifest.xml

    This will write the decoded XML to `output.xml`.

    `axmldec` writes to the standard output if the `-o` option is not
    specified. This is useful when additional processing is required. For
    example, you can extract the package name using `xmllint`:

        axmldec AndroidManifest.xml | xmllint --xpath 'string(/manifest/@package)' -

## 4 Building

Install Boost and CMake. Make sure you have a latest C++ compiler. Then compile:

    cmake -DCMAKE_BUILD_TYPE=Release . && make

## 5 Developer

- [Yutaka Tsutano](http://yutaka.tsutano.com) at University of Nebraska-Lincoln.

## 6 License

- See [LICENSE.md](LICENSE.md) for license rights and limitations (ISC).
