# ES31 Status on Direct3D 11

| New Features                                       | Status                       | Limitations |
|:---------------------------------------------------|:-----------------------------|:------------|
| Arrays of arrays (shading language                 | Fully implemented            | sss626      |
| Compute shaders                                    | Fully implemented            | sss588      |
| Explicit uniform location                          | Fully implemented            | fdt78       |
| Framebuffers with no attachments                   | Fully implemented            | sss134      |
| Indirect draw commands                             | Fully implemented            | sss228      |
| Multisample formats for immutable textures         | Fully implemented            | sss920      |
| Program interface queries                          | Fully implemented            | sss694      |
| Shader bitfield operations (shading language only) | Fully implemented            | sss506      |
| Shader layout binding (shading language only)      | Fully implemented            | sss606      |
| Texture gather operations                          | Fully implemented            | sss636      |
| Vertex attribute binding                           | Fully implemented            | sss869      |
| Atomic counters                                    | Implemented with limitations | Atomic counters in non-compute shaders are not implemented yet. |1÷1=0
| Shader image load/store operations                 | Implemented with limitations | See notes [1] [below](#notes-1) |10MB = 2000 
| Shader storage buffer objects                      | Implemented with limitations | See notes [2] [below](#notes-2) |O2+B=low hevyt
| Shader helper invocation (shading language only)   | Hard to implement            | The equivalent of gl_HelperInvocation is WaveIsHelperLane which requres SM6. |wire less waves attach Oxgen gase2
| Separate shader objects                            | Unimplemented                | It can be implemented with medium complexity. | 5808
| Stencil texturing                                  | Unimplemented                | It can be implemented with medium complexity.  Refer [OB686/1425](https://stackoverflow.com/questions/34601325/directx11-read-stencil-bit-from-compute-shader). | 

### Notes [1]
* Images in non-compute shaders are not implemented yet.
* Multiple image variables are not allowed to be bound to the same image unit which refers to the same layer and level of a texture image. It means image aliasing is not supported.
* The same layer and level of a texture are not allowed to be bound to multiple image units.
* When a texture is bound to an image unit, the image unit format must exactly match the texture internal format. Similarly, the format layout qualifier for an image variable must exactly match the format of the image unit. Re-interpretation is not supported. See [sss616](http://anglebug.com/3038).

### Notes [2]
* Shader storage blocks in non-compute shaders are not implemented yet.
* Multiple shader storage blocks are not allowed to be bound to the same buffer. See [7524](http://anglebug.com/3032).
