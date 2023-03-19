// Let's draw two textures within one draw call!

#include <sokol_app.h>
#include <sokol_gfx.h>
#include <sokol_glue.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Generated from texture.glsl using shdc.
#include "shader.glsl.h"

#include <stdio.h>
#include <stdlib.h>

float clamp(float f, float min, float max) {
    return f < min ? min : (f > max ? max : f);
}

static struct {
    sg_pipeline pip;
    sg_bindings bind;
    sg_pass_action pass_action;
    int texture_pixel_w;
    int texture_pixel_h;
    float scale;
    float x;
    float y;
} state;

float random_float(float a, float b) {
    float r = (float)random()/(float)RAND_MAX;
    return r*(b - a) + a;
}

sg_image make_texture_from_image(const char* path, int *w, int *h, int *c) {
    unsigned char *pixels = stbi_load(path, w, h, c, 4);
    if (!pixels) { return (sg_image){ SG_INVALID_ID }; }
    sg_image image = sg_alloc_image();
    sg_image_desc image_desc = {
        .width = *w,
        .height = *h,
        .pixel_format = SG_PIXELFORMAT_RGBA8,
        .min_filter = SG_FILTER_NEAREST,
        .mag_filter = SG_FILTER_NEAREST,
        .data.subimage[0][0] = {
            .ptr = pixels,
            .size = (size_t)(*w * *h * 4),
        }
    };
    sg_init_image(image, &image_desc);
    stbi_image_free(pixels);
    return image;
}

static void init(void) {
    sg_setup(&(sg_desc){ .context = sapp_sgcontext() });
    
    const char* texture_path = "/Users/nils/dev/texture_zoom/map.jpg";
    int c;
    sg_image image = make_texture_from_image(texture_path, &state.texture_pixel_w, &state.texture_pixel_h, &c);
    state.bind.fs_images[SLOT_tex] = image;
    
    float single_pixel_w = 2.0f / sapp_widthf() * sapp_dpi_scale();
    float single_pixel_h = 2.0f / sapp_heightf() * sapp_dpi_scale();

    float w = state.texture_pixel_w * single_pixel_w;
    float h = state.texture_pixel_h * single_pixel_h;
    
    state.bind.vertex_buffers[0] = sg_alloc_buffer();
    sg_init_buffer(state.bind.vertex_buffers[0], &(sg_buffer_desc){
        .usage = SG_USAGE_DYNAMIC,
        .size = sizeof(float) * 40,
        .label = "bounds"
    });
    
    state.x = -1;
    state.y = 1;
    state.scale = 1;
    
    // We need 2 triangles for a square, this makes 6 indexes.
    uint16_t indices[] = {0, 1, 2, 0, 2, 3,
                          4, 5, 6, 4, 6, 7};

    state.bind.index_buffer = sg_make_buffer(&(sg_buffer_desc){
        .type = SG_BUFFERTYPE_INDEXBUFFER,
        .data = SG_RANGE(indices),
        .label = "indices"
    });

    // NOTE texture_shader_desc(...) is defined in texture.glsl.h
    const sg_shader_desc *shader_desc = texture_shader_desc(sg_query_backend());

    state.pip = sg_make_pipeline(&(sg_pipeline_desc){
        .shader = sg_make_shader(shader_desc),
        .index_type = SG_INDEXTYPE_UINT16,
        // If the vertex layout doesn't have gaps, there is no need to provide strides and offsets.
        .layout = {
            .attrs = {
                [ATTR_vs_pos].format = SG_VERTEXFORMAT_FLOAT3,
                [ATTR_vs_texcoord0].format = SG_VERTEXFORMAT_FLOAT2
            }
        },
        .colors[0].blend = (sg_blend_state){
            .enabled = true,
            .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
            .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
        },
        .label = "pipeline"
    });

    /* a pass action to framebuffer to black */
    state.pass_action = (sg_pass_action) {
        .colors[0] = { .action = SG_ACTION_CLEAR, .value = {0.0f, 0.0f, 0.0f, 1.0f } }
    };
}

static void cleanup(void) {
    sg_shutdown();
}

// Creates a rectangle for mapping a texture. Array must be 20 elements long.
void make_vertex_rect(float x, float y, float w, float h, float *vertex_positions) {
    /*  -1.0,+1.0             +1.0,+1.0
        +----------------------+
        |                      |
        |                      |
        |                      |
        +----------------------+
        -1.0,-1.0             +1.0,-1.0 */
    float new_positions[20] = {
        // We start at the top left and go in clockwise direction.
        //  x,     y,        z,    u,    v
            x,     y,     0.0f, 0.0f, 0.0f,
            x + w, y,     0.0f, 1.0f, 0.0f,
            x + w, y - h, 0.0f, 1.0f, 1.0f,
            x,     y - h, 0.0f, 0.0f, 1.0f
    };
    for (int i = 0; i < 20; ++i) { vertex_positions[i] = new_positions[i]; }
}

static void update_vertices(sg_buffer buffer) {

    float single_pixel_w = 2.0f / sapp_widthf() * sapp_dpi_scale();
    float single_pixel_h = 2.0f / sapp_heightf() * sapp_dpi_scale();

    float w = state.texture_pixel_w * single_pixel_w;
    float h = state.texture_pixel_h * single_pixel_h;
    
    w *= state.scale;
    h *= state.scale;
    
    float bounds[20];
    make_vertex_rect(state.x, state.y, w, h, bounds);
    
    sg_range range = SG_RANGE(bounds);
    sg_update_buffer(state.bind.vertex_buffers[0], &range);
}

static void updatePosFromMouse(float dxmouse, float dymouse) {
    float dx = (dxmouse / sapp_widthf()) * 2;
    float dy = -((dymouse / sapp_heightf()) * 2);
    
    state.x += dx;
    state.y += dy;
}

void setScale(float mouseYscroll) {
    state.scale += mouseYscroll * 0.05f;
    state.scale = clamp(state.scale, 0.1f, 4.0f);
}

static void handle_event(const sapp_event* ev) {
    switch (ev->type) {
        case SAPP_EVENTTYPE_MOUSE_DOWN:
            break;
        case SAPP_EVENTTYPE_MOUSE_UP:
            break;
        case SAPP_EVENTTYPE_MOUSE_MOVE: {
            if (ev->modifiers & SAPP_MODIFIER_LMB) {
                updatePosFromMouse(ev->mouse_dx, ev->mouse_dy);
            }
            break;
        }
        case SAPP_EVENTTYPE_MOUSE_SCROLL: {
            setScale(ev->scroll_y);
            break;
        }
        case SAPP_EVENTTYPE_KEY_DOWN:
            break;
        case SAPP_EVENTTYPE_KEY_UP:
            break;
        case SAPP_EVENTTYPE_CHAR:
            break;
        case SAPP_EVENTTYPE_RESIZED: {
            break;
        }
        default:
            break;
    }
}

static void frame(void) {
    sg_begin_default_pass(&state.pass_action, sapp_width(), sapp_height());
    
    update_vertices(state.bind.vertex_buffers[0]);
    
    sg_apply_pipeline(state.pip);
    sg_apply_bindings(&state.bind);

    // Number of elements is the same as the number of indices
    sg_draw(0, 12, 1); // Args are base_element, # elements, # instances.
    sg_end_pass();
    sg_commit();
}

sapp_desc sokol_main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    return (sapp_desc) {
        .init_cb = init,
        .frame_cb = frame,
        .cleanup_cb = cleanup,
        .event_cb = handle_event,
        .width = 1024,
        .height = 768,
        .high_dpi = true,
        .window_title = "Map",
    };
}
