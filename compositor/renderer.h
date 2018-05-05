#ifndef RENDERER_H
#define RENDERER_H

/*
 * The renderer interface assumes that an EGL context is already set up, and
 * provides functions to create textures and render them there.
 *
 * NOTE: a future Vulkan implementation of the renderer would require a
 * modification of this interface. Vulkan doesn't rely on EGL, there are two
 * possible ways to make it work:
 * 1) let it drive KMS itself
 * 2) allocate buffers with GBM and wrap them in VkImages. After rendering
 * extract the dma-fence FD from the rendering's signal VkSemaphore and pass
 * that into KMS with IN_FENCE_FD. This would require buffer handles as inputs
 * and dma-fence FDs as outputs.
 */

struct texture;

struct renderer *renderer_setup();

void renderer_clear();

struct texture *renderer_tex_from_data(const int32_t width, const int32_t
height, const void *data);

void renderer_tex_draw(const struct renderer *renderer, const struct texture
*texture);

void renderer_delete_tex(struct texture *texture);

#endif
