# zkanji BETA
This is a temporary name. I will refer to the program simply as zkanji.

zkanji is FREE software. If you payed for it, that's a scam and you should ask
for a refund. You can find the source code of the program on [GitHub](https://github.com/z1dev/zkanji).

You can reach me at:  
z-.one AT+sign free.mail DOT hu (Remove the spaces and . characters, and
replace parts accordingly.)

---

## Description

zkanji is an open-source, cross-platform, feature rich Japanese language study
suite and dictionary software. It is beta, meaning its features are in place
but not fully tested. Use it at your own risk! zkanji has several kanji look-up
methods, example sentences for many Japanese words, vocabulary printing, JLPT
levels indicated for words and kanji, a spaced-repetition system for studying
and more.

Please see the License and Credit sections at the end of this readme.

---

## VERY short history of zkanji

The first version of zkanji was released in the late 2000s (or 00s.) The final
version was released in 2013, which can be downloaded from its project page on
[Sourceforge](https://sourceforge.net/projects/zkanji/). I more or less lost
interest to develop the program further at the time.  

I wanted to revive zkanji, and started working on the next version in 2015.
Since v0.731 was made with a compiler and library I no longer use, I had to
start over the project from scratch. This is when I decided to make it
cross-platform, and chose Qt as the library for the user interface. I finally
reached a point in 2017 to release this new version, and plan to develop it
further.

---

## Compiling from source

zkanji utilizes Qt 5, so an installation of the Qt SDK with its header files
and libraries must be present on the system. I used Qt version 5.5 during
initial development, but switched to 5.9.1 then 5.10.1 for the early release on
GitHub. Older versions of Qt might work, but were not tested. zkanji can be
compiled with C++ compilers that are C++11 compatible and can use Qt.

* On Windows: the minimal Visual Studio version capable of compiling zkanji is
VS2015, as it's required for the C++11 features. I don't plan making zkanji
compatible with older compilers.
It is recommended, and possibly required, to use the Qt add-on for Visual
Studio. GNU C++ or other compilers should have no problem compiling zkanji
either, but I haven't tested those. The .ui and Qt resource files must be
manually processed when not using the Qt add-on for VS or compiling with a
different compiler.
Currently only the Visual Studio and Qt Creator project files are included. If
it is set up correctly, zkanji should compile out of the box.
If you plan to debug zkanji in Visual Studio, see the VS folder for natvis
extension for zkanji data types. Copy 'zkanji.natvis' to
'%VSInstallLocation%\Common7\Packages\Debugger\Visualizers\'.

* When compiling for the Linux platform, use Qt Creator and open the `.pro`
file found in the root folder. If Qt was installed the default way, the program
will compile without further changes, but it won't have an icon.

---

## Deployment on Windows

To use the program after compilation, you should copy the C++ runtime DLLs of
your compiler next to the executable. zkanji also needs the Qt DLLs. See the
page [Qt for Windows - Deployment](http://doc.qt.io/qt-5/windows-deployment.html)
for details.

The easiest way is to run the `windeployqt` executable from the Qt `bin` folder
with the path to the compiled zkanji executable as a command-line parameter.
This will copy some DLLs next to the executable, and also create some extra
folders. These extra folders, except the one called `platforms` (this is
important!), must be copied to a subfolder named `plugins`. The `platforms`
folder should be kept in the root of the executable.

---

## Deployment on Linux

To be completed...

---

## Setting up the data files (unless using pre-installed ones)

Files must be downloaded from the following locations, before the data files
can be generated by zkanji:  
[JMdict](http://www.edrdg.org/jmdict/edict_doc.html) in UTF-8 encoding (The
"JMdict.gz" or the "JMdict_e.gz" link.)  
[kanjidic](http://nihongo.monash.edu/kanjidic.html) in EUC-JP encoding (First
or second link after the "Monash University ftp site" text.)  
[radkfile](http://nihongo.monash.edu/kradinf.html) also in EUC-JP encoding.
The link is titled "radkfile.gz" in the download section.

For the example sentences data, download the complete version of the
[Tanaka Corpus](http://www.edrdg.org/wiki/index.php/Tanaka_Corpus) in UTF-8
encoding.
As an alternative, you can get the smaller subset of the Tanaka Corpus, but
it's only available in EUC-JP encoding on the site, so it must be converted to
UTF-8 first with the name `examples.utf`.

The downloaded files must be unpacked into a single folder (referred to as
`import folder` from here on.) The zkanji package contains a folder called
`dataimports` which holds additional files needed for dictionary import. Copy
everything from there into the `import folder` too.

The command line option for generating the dictionary data files is:
`-i [path]`
Path must point to the `import folder` containing the extracted JMdict,
KANJIDIC and RADKFILE, plus the contents from the `dataimports` folder.

To generate the example sentences data file, use the command line option:  
`-e [path]`
The `examples.utf` file must be on the passed path and the dictionary data must
be present already (or imported with -i at the same time.)

The options can be combined. For example the following line will create every
necessary data file for zkanji after correcting the path:  
`zkanji.exe -ie C:\importfolder` (on Windows)
or
`./zkanji -ie ~/importfolder` (on Linux)

If the data files are generated successfully, zkanji will start. See the next
section.

---

## Creating and using translation files

Since the first BETA version, zkanji can use translations to change the user
interface language. Please write a mail if you plan on starting a new
translation to avoid parallel work on the same language and to ask how to start
it.

---

## Running for the first time

When run for the first time, zkanji might offer to run normally or in portable
mode. For this test version, it's recommended to start the program in portable
mode, to save the data files next to the executable in the `data` folder. (The
user must have full access to the folder.) If normal mode is selected, the user
data files will be saved in a user folder appropriate for the system.

The program will offer to import user data from zkanji v0.731 next. If this step
is skipped, the old files can only be imported after deleting the generated user
files. (Files with the `.zkdict` and `.zkuser` extensions in the `data` folder,
and the `zkanji.ini` file.)

---

## License

Copyright 2007-2013, 2017-2018 S�lyom Zolt�n

zkanji BETA is licensed under the terms of the GNU GPLv3. See the LICENSE file
for details.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

An earlier program called zkanji, last released in 2013 used a different
license, which is NOT applicable to this version.

---

## Credits

The following projects are not in any connection with zkanji. The program is
only using them as their license allows:

* Qt Open Source Edition is used for the cross platform user interface and for
other purposes, like string handling. Qt is licensed under the terms of the GNU
LGPL v3. You can find out more about it on the [Qt website](https://www.qt.io/).

* Some parts of LibQxt, an open source project to extend Qt were used in zkanji
with small modifications. See the [LibQxt project's website](https://bitbucket.org/libqxt/libqxt/)
for the source code. See the `QxtAUTHORS` file for its authors who contributed
to that project, and `QxtCOPYING` for license information.

* zkanji wouldn't be possible without JMdict (a free Japanese dictionary,)
KANJIDIC (holding information about 6355 kanji) and RADKFILE. They are the
property of [The Electronic Dictionary Research and Development Group, Monash University](http://www.edrdg.org/).
The files are made available under a Creative Commons Attribution-ShareAlike
Licence (V3.0).

* The [Tanaka Corpus](http://www.edrdg.org/wiki/index.php/Tanaka_Corpus), a
database of English/Japanese example sentences is included or can be imported in
zkanji. It is released under the terms of [Creative Commons CC-BY](http://creativecommons.org/licenses/by/2.0/fr/deed.en_GB)
license.
