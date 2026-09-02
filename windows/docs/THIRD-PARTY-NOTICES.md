# Third-party notices for the Windows build

## Qt 5.15.2

The Windows build dynamically links the Qt Core, GUI, Widgets, Windows platform,
and Windows Vista style libraries from Qt 5.15.2. Qt is available under the GNU
Lesser General Public License version 3. The unmodified DLLs are kept separate
from `EinkAssistant.exe`, so users can replace them with compatible builds.

- Project: <https://www.qt.io/>
- Source: <https://download.qt.io/archive/qt/5.15/5.15.2/single/>
- License: `LGPL-3.0.txt`

Qt is Copyright (C) The Qt Company Ltd. and other contributors.

## MinGW-w64 and GCC runtime libraries

The package includes the MinGW-w64/GCC runtime libraries required by the Qt
build: `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, and `libwinpthread-1.dll`.
They are distributed under their respective GNU licenses and runtime-library
exceptions. Corresponding source is available from:

- <https://gcc.gnu.org/>
- <https://www.mingw-w64.org/>

## win-nightlight-cli format mapping

The Night Light CloudStore/Bond binary-format mapping is derived from
`win-nightlight-cli` by Kevin Xiao:

- Project: <https://github.com/kvnxiao/win-nightlight-cli>
- License: MIT

MIT License

Copyright (c) 2025 Kevin Xiao

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
