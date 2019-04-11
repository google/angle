# ANGLE: Vulkan Back-end

ANGLE's Vulkan back-end implementation lives in this folder.

[Vulkan](https://www.khronos.org/vulkan/) is an explicit graphics API. It has a lot in common with
other explicit APIs such as Microsoft's
[D3D12](https://docs.microsoft.com/en-us/windows/desktop/direct3d12/directx-12-programming-guide)
and Apple's [Metal](https://developer.apple.com/metal/). Compared to APIs like OpenGL or D3D11
explicit APIs can offer a number of significant benefits:

 * Lower API call CPU overhead.
 * A smaller API surface with more direct hardware control.
 * Better support for multi-core programming.
 * Vulkan in particular has open-source tooling and tests.

## Back-end Design

The [RendererVk](RendererVk.cpp) is a singleton. RendererVk owns shared global resources like the
[VkDevice](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkDevice.html),
[VkQueue](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkQueue.html), the
[Vulkan format tables](vk_format_utils.h) and [internal Vulkan shaders](shaders). The back-end
creates a new [ContextVk](ContextVk.cpp) instance to manage each allocated OpenGL Context. ContextVk
processes state changes and handles action commands like `glDrawArrays` and `glDrawElements`.

### Fast OpenGL State Transitions

Typical OpenGL programs issue a few small state change commands between draw call commands. We want
the typical app's use case to be as fast as possible so this leads to unique performance challenges.

Vulkan in quite different from OpenGL because it requires a separate compiled
[VkPipeline](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipeline.html)
for each state vector. Compiling VkPipelines is multiple orders of magnitude slower than enabling or
disabling an OpenGL render state. To speed this up we use three levels of caching when transitioning
states in the Vulkan back-end.

The first level is the driver's
[VkPipelineCache](https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkPipelineCache.html). The driver cache reduces pipeline recompilation time
significantly. But even cached pipeline recompilations are orders of manitude slower than OpenGL
state changes.

The second level cache is an ANGLE-owned hash map from OpenGL state vectors to compiled pipelines.
See
[GraphicsPipelineCache](https://chromium.googlesource.com/angle/angle/+/225f08bf85a368f905362cdd1366e4795680452c/src/libANGLE/renderer/vulkan/vk_cache_utils.h#498)
in [vk_cache_utils.h](vk_cache_utils.h). ANGLE's
[GraphicsPipelineDesc](https://chromium.googlesource.com/angle/angle/+/225f08bf85a368f905362cdd1366e4795680452c/src/libANGLE/renderer/vulkan/vk_cache_utils.h#244)
class is a tightly packed 256-byte description of the current OpenGL rendering state. We
also use a [xxHash](https://github.com/Cyan4973/xxHash) for the fastest possible hash computation.
The hash map speeds up state changes considerably. But it is still significantly slower than OpenGL
implementations.

To get best performance we use a transition table from each OpenGL state vector to neighbouring
state vectors. The transition table points from GraphicsPipelineCache entries directly to
neighbouring VkPipeline objects. When the application changes state the state change bits are
recorded into a compact bit mask that covers the GraphicsPipelineDesc state vector. Then on the next
draw call we scan the transition bit mask and compare the GraphicsPipelineDesc of the current state
vector and the state vector of the cached transition. With the hash map we compute a hash over the
entire state vector and then do a 256-byte `memcmp` to guard against hash collisions. With the
transition table we will only compare as many bytes as were changed in the transition bit mask. By
skipping the expensive hashing and `memcmp` we can get as good or faster performance than native
OpenGL drivers.

Note that the current design of the transition table stores transitions in an unsorted list. If
applications map from one state to many this will slow down the transition time. This could be
improved in the future using a faster look up. For instance we could keep a sorted transition table
or use a small hash map for transitions.

### Shader Module Compilation

ANGLE converts application shaders into Vulkan [VkShaderModules][VkShaderModule] through a series
of steps:

1. **ANGLE Internal Translation**: The initial calls to `glCompileShader` are passed to the [ANGLE
shader translator][translator]. The translator compiles application shaders into Vulkan-compatible
GLSL. Vulkan-compatible GLSL matches the [GL_KHR_vulkan_glsl][GL_KHR_vulkan_glsl] extension spec
with some additional workarounds and emulation. We emulate OpenGL's different depth range, viewport
y flipping, default uniforms, and OpenGL line segment rasterization. For more info see
[TranslatorVulkan.cpp][TranslatorVulkan.cpp]. After initial compilation the shaders are not
complete. They are templated with markers that are filled in later at link time.

1. **Link-Time Translation**: During a call to `glLinkProgram` the Vulkan back-end can know the
necessary locations and properties to write to connect the shader stage interfaces. We get the
completed shader source using ANGLE's [GlslangWrapper][GlslangWrapper.cpp] helper class. We still
cannot generate `VkShaderModules` since some ANGLE features like OpenGL line rasterization emulation
depend on draw-time information.

1. **Draw-time SPIR-V Generation**: Once the application records a draw call we use Khronos'
[glslang][glslang] to convert the Vulkan-compatible GLSL into SPIR-V with the correct draw-time
defines. The SPIR-V is then compiled into `VkShaderModules`. For details please see
[GlslangWrapper.cpp][GlslangWrapper.cpp]. The `VkShaderModules` are then used by `VkPipelines`. Note
that we currently don't use [SPIRV-Tools][SPIRV-Tools] to perform any SPIR-V optimization. This
could be something to improve on in the future.

See the below diagram for a high-level view of the shader translation flow:

<!-- Generated from https://bramp.github.io/js-sequence-diagrams/
participant App
participant "ANGLE Front-end"
participant "Vulkan Back-end"
participant "ANGLE Translator"
participant "GlslangWrapper"
participant "Glslang"

App->"ANGLE Front-end": glCompileShader (VS)
"ANGLE Front-end"->"Vulkan Back-end": ShaderVk::compile
"Vulkan Back-end"->"ANGLE Translator": sh::Compile
"ANGLE Translator"- ->"ANGLE Front-end": return Vulkan-compatible GLSL

Note right of "ANGLE Front-end": Source is templated\nwith markers to be\nfilled at link time.

Note right of App: Same for FS, GS, etc...

App->"ANGLE Front-end": glCreateProgram (...)
App->"ANGLE Front-end": glAttachShader (...)
App->"ANGLE Front-end": glLinkProgram
"ANGLE Front-end"->"Vulkan Back-end": ProgramVk::link

Note right of "Vulkan Back-end": ProgramVk inits uniforms,\nlayouts, and descriptors.

"Vulkan Back-end"->GlslangWrapper: GlslangWrapper::GetShaderSource
GlslangWrapper- ->"Vulkan Back-end": return filled-in sources

Note right of "Vulkan Back-end": Source is templated with\ndefines to be resolved at\ndraw time.

"Vulkan Back-end"- ->"ANGLE Front-end": return success

Note right of App: App execution continues...

App->"ANGLE Front-end": glDrawArrays (any draw)
"ANGLE Front-end"->"Vulkan Back-end": ContextVk::drawArrays

"Vulkan Back-end"->GlslangWrapper: GlslangWrapper::GetShaderCode (with defines)
GlslangWrapper->Glslang: GlslangToSpv
Glslang- ->"Vulkan Back-end": Return SPIR-V

Note right of "Vulkan Back-end": We init VkShaderModules\nand VkPipeline then\nrecord the draw.

"Vulkan Back-end"- ->"ANGLE Front-end": return success
-->

![Vulkan Shader Translation Flow](https://raw.githubusercontent.com/google/angle/master/src/libANGLE/renderer/vulkan/doc/img/VulkanShaderTranslation.svg?sanitize=true)

[VkShaderModule]: https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkShaderModule.html
[translator]: https://chromium.googlesource.com/angle/angle/+/refs/heads/master/src/compiler/translator/
[GL_KHR_vulkan_glsl]: https://github.com/KhronosGroup/GLSL/blob/master/extensions/khr/GL_KHR_vulkan_glsl.txt
[TranslatorVulkan.cpp]: https://chromium.googlesource.com/angle/angle/+/refs/heads/master/src/compiler/translator/TranslatorVulkan.cpp
[glslang]: https://github.com/KhronosGroup/glslang
[GlslangWrapper.cpp]: https://chromium.googlesource.com/angle/angle/+/refs/heads/master/src/libANGLE/renderer/vulkan/GlslangWrapper.cpp
[SPIRV-Tools]: https://github.com/KhronosGroup/SPIRV-Tools