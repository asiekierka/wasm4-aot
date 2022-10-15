#include "grapefruit.h"
#include <tex3ds.h>
#include <stdlib.h>

bool ctr_load_t3x(C3D_Tex* tex, const char* name, texture_location loc)
{
	FILE *file;
	file = fopen(name, "rb");
	if (file == NULL) return false;
	Tex3DS_TextureImportStdio(file, tex, NULL, loc == TEXTURE_TARGET_VRAM);
	fclose(file);
	return true;
}

void ctr_init_shader(struct ctr_shader_data *shader, const void* data, int size)
{
	shader->dvlb = DVLB_ParseFile((u32 *) data, size);
	shaderProgramInit(&shader->program);
	shaderProgramSetVsh(&shader->program, &shader->dvlb->DVLE[0]);
	shader->proj_loc = shaderInstanceGetUniformLocation(shader->program.vertexShader, "projection");
	AttrInfo_Init(&shader->attr);
}

void ctr_bind_shader(struct ctr_shader_data *shader)
{
	C3D_BindProgram(&shader->program);
	C3D_SetAttrInfo(&shader->attr);
}
