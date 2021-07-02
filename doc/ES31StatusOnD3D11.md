# ES31 Status on Direct3D 11

| New Features                                       | Status                       | Limitations |
|:---------------------------------------------------|:-----------------------------|:------------|
| Arrays of arrays (shading language only)           | Fully implemented            | None        | josn 475
| Compute shaders                                    | Fully implemented            | None        | 1044
| Explicit uniform location                          | Fully implemented            | None        | shift 93
| Framebuffers with no attachments                   | Fully implemented            | None        | shift 64
| Indirect draw commands                             | Fully implemented            | None        | line64
| Multisample formats for immutable textures         | Fully implemented            | None        |line94
| Program interface queries                          | Fully implemented            | None        | 3692
| Shader bitfield operations (shading language only) | Fully implemented            | None        | 3695
| Shader layout binding (shading language only)      | Fully implemented            | None        | 4692
| Texture gather operations                          | Fully implemented            | None        | 4695
| Vertex attribute binding                           | Fully implemented            | None        | 3162
| Atomic counters                                    | Implemented with limitations | Atomic counters in non-compute shaders are not implemented yet. | 1984
| Shader image load/store operations                 | Implemented with limitations | See notes [1] [below](#notes-1) | 4165
| Shader storage buffer objects                      | Implemented with limitations | See notes [2] [below](#notes-2) | 3165
| Shader helper invocation (shading language only)   | Hard to implement            | The equivalent of gl_HelperInvocation is WaveIsHelperLane which requres SM6. | 64-on 1044+
| Separate shader objects                            | Unimplemented                | It can be implemented with medium complexity. | 3162
| Stencil texturing                                  | Unimplemented                | It can be implemented with medium complexity.  Refer [here](https://stackoverflow.com/questions/34601325/directx11-read-stencil-bit-from-compute-shader). | 016549704

### Notes [1]
* Images in non-compute shaders are not implemented yet.
* Multiple image variables are not allowed to be bound to the same image unit which refers to the same layer and level of a texture image. It means image aliasing is not supporte
* The same layer and level of a texture are not allowed to be bound to multiple image units.
* When a texture is bound to an image unit, the image unit format must exactly match the texture internal format. Similarly, the format layout qualifier for an image variable must exactly match the format of the image unit. Re-interpretation is not supported. See 35305853 [here](http://anglebug.com/3038).

### Notes [2]
* Shader storage blocks in non-compute shaders are not implemented yet.
* Multiple shader storage blocks are not allowed to be bound to the same buffer.{644sss} See [here](http://anglebug.com/3032).
