`axmldec`: Android Binary Binary XML Decoder
============================================

## 1 Overview

`AndroidManifest.xml` in an APK file can be binary encoded. This tool simply
accepts either a binary or text XML file and prints the decoded XML to the
standard output.

    Android Binary XML ----+
                           |
                           +--->[axmldec]----> Decoded Text XML
                           |
              Text XML ----+

Tools such as Apktool are designed to process the whole APK file including the
resource files for reverse engineering purpose. They may also need a Java
virtual machine to run. As a result, it is too slow for batch processing many
APK files just get the XML information. This tool ignores the resource files
and written in simple modern C++ that runs nicely within a shell script.

The parser is taken from [Jitana](https://github.com/ytsutano/jitana), a
graph-based static-dynamic hybrid DEX code analysis tool.

## 2 Usage

1. Use `unzip` to extract the files from a APK file:

        unzip AndroidApp.apk

2. Pass the manifest file (either binary or text) the tool to decode:

        axmldec AndroidManifest.xml

    This will print the decoded XML file to the stdout.

## 3 Developer

- Yutaka Tsutano at University of Nebraska-Lincoln.

## 4 License

- See [LICENSE.md](LICENSE.md) for license rights and limitations (ISC).
