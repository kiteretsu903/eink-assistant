This project includes work derived from other open source projects.

================================================================
Stillcolor
https://github.com/aiaf/Stillcolor
Copyright (c) 2024 Abdullah Arif
================================================================

The technique used by the "Reduce Shaking" feature — disabling display
dithering by setting the enableDither property on IOMobileFramebufferAP
I/O Registry entries — is taken from Stillcolor. macos/Sources/Shared/Dither.swift
is a reimplementation of that idea, narrowed from all displays to a single
display. Thanks to Abdullah Arif for working it out and publishing it.

Stillcolor is distributed under the MIT License, reproduced in full below.

MIT License

Copyright (c) 2024 Abdullah Arif

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
