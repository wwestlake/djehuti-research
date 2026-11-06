# Reuse record

The root MIT `LICENSE` covers Djehuti Research-owned material. It does not replace any external license. Upstream component implementations remain external; no JUCE, LLVM, CreationDock or FRust source tree is vendored in this repository.

Application lifecycle/settings/docking patterns adapted from DjehutiMath foundation `ff2ee952abe2e697db71d9342618fb6cc359f870` (MIT, copyright 2026 WWestlake).

CreationDock and FRust compiler/plugin host are consumed from the external FrustLang checkout at `008afac7e5bea9569e0631795871334b2336a022` (MIT, copyright 2026 William Westlake). They remain separate read-only sources; none is vendored or modified here.

MIT permission notice for those patterns/components:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## External dependencies and redistribution boundaries

| Component actually used | Terms verified | Publication/distribution boundary |
|---|---|---|
| DjehutiMath foundation patterns, `ff2ee952abe2e697db71d9342618fb6cc359f870` | [MIT](https://github.com/wwestlake/DjehutiMath/blob/ff2ee952abe2e697db71d9342618fb6cc359f870/LICENSE), attribution above | Retain the copyright and permission notice with copied/adapted material. |
| FrustLang compiler/runtime, plugin-host interfaces, CreationDock and Frate, `008afac7e5bea9569e0631795871334b2336a022` | [Root MIT license](https://github.com/wwestlake/FrustLang/blob/008afac7e5bea9569e0631795871334b2336a022/LICENSE); no separate override license found in the referenced components | External sources compiled locally; preserve their notices if subsequently redistributed. |
| JUCE 7.0.9 `juce_gui_basics`, `juce_gui_extra`, `juce_graphics`, `juce_data_structures` | [GPLv3/commercial](https://github.com/juce-framework/JUCE/blob/7.0.9/modules/juce_gui_basics/juce_gui_basics.h), [JUCE 7 license file](https://github.com/juce-framework/JUCE/blob/7.0.9/LICENSE.md) | Not MIT. Combined-application distribution must follow the applicable JUCE commercial terms or GPLv3 route. No commercial entitlement is asserted by this repository. |
| JUCE 7.0.9 `juce_core` and `juce_events` | ISC, as stated in their module headers | Retain their module copyright and permission notices when redistributed. Bundled JUCE codecs/other third-party portions retain the notices in their upstream files. |
| LLVM 18.1.6 component libraries | [Apache-2.0 with LLVM exceptions and the component notices in that file](https://github.com/llvm/llvm-project/blob/llvmorg-18.1.6/llvm/LICENSE.TXT) | Existing installation is consumed read-only. Any later redistribution must preserve the applicable upstream license/attribution and account for its exceptions. |
| WinFlexBison, CMake, Git, Visual Studio, Windows/DIA SDK and PowerShell | External build/test prerequisites, governed by their own distributions | Their tool binaries, SDKs and runtime files are not included in the source publication. |
| Generated C++ parser and lexer support | Bison 3.8.2 GPLv3-or-later parser skeleton with its larger-work exception; Flex 2.6.4 generated scanner exception and the BSD notice in `FlexLexer.h` | The Windows package retains the Bison parser notice/exception and full FlexLexer header notice alongside GPLv3 text. No generator executable is shipped. |

The Windows distribution uses JUCE 7's GPL version 3 option for the combined executable. Project-owned sources retain MIT terms. The release supplies matching Research, JUCE, FRust and LLVM source archives, GPLv3 text, component notices and rebuild instructions alongside the binary, without a source access fee. The separate Microsoft VC143 Release runtime DLLs use Microsoft's redistributable terms. See [docs/RELEASING.md](docs/RELEASING.md); an MIT license for project-owned sources does not make the combined binary MIT-only.
