#ifndef _GRAPEFRUIT_H_
#define _GRAPEFRUIT_H_

#include <3ds.h>
#include <citro3d.h>
#include <stdbool.h>

typedef enum {
	TEXTURE_TARGET_RAM,
	TEXTURE_TARGET_VRAM
} texture_location;

struct ctr_shader_data
{
  DVLB_s* dvlb;
  shaderProgram_s program;
  int proj_loc;
  C3D_AttrInfo attr;
};

bool ctr_load_t3x(C3D_Tex* tex, const char *name, texture_location loc);

void ctr_init_shader(struct ctr_shader_data *shader, const void* data, int size);
void ctr_bind_shader(struct ctr_shader_data *shader);

#endif /* _GRAPEFRUIT_H_ */
