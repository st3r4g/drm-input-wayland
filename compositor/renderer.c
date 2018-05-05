#include <stdio.h>
#include <stdlib.h>

#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include "algebra.h"

struct renderer {
	GLuint program;
};

struct texture {
	GLuint vao;
	GLuint tex;
};

GLuint CreateProgram(const char *name);
char *GetShaderSource(const char *src_file);

struct renderer *renderer_setup() {
	struct renderer *renderer = calloc(1, sizeof(struct renderer));
	renderer->program = CreateProgram("texture");
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	return renderer;
}

void renderer_clear() {
	glClear(GL_COLOR_BUFFER_BIT);
}

struct texture *renderer_tex_from_data(const int32_t width, const int32_t
height, const void *data) {
	struct texture *texture = calloc(1, sizeof(struct texture));
	GLuint *vao = &texture->vao;
	GLuint *tex = &texture->tex;

	glGenVertexArrays(1, vao);
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindVertexArray(*vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	GLfloat rect[] = {
		-0.08f, -0.08f, 0.0f, 0.0f, // left bottom
		+0.08f, -0.08f, 1.0f, 0.0f, // right bottom
		-0.08f, +0.08f, 0.0f, 1.0f, // left top
		+0.08f, +0.08f, 1.0f, 1.0f, // right top
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(rect), rect, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat),
	(void*)(2*sizeof(GLfloat)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_2D, *tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA_EXT,
	GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);
	return texture;
}

void renderer_tex_draw(const struct renderer *renderer, const struct texture
*texture) {
	if (texture) {
		GLuint program = renderer->program;
		GLuint vao = texture->vao;
		GLuint tex = texture->tex;

		glUseProgram(program);

		GLfloat matrix[16];
		algebra_matrix_rotation_x(matrix, 0);
		GLint loc = glGetUniformLocation(program, "matrix");
		glUniformMatrix4fv(loc, 1, GL_TRUE, matrix);
		glBindTexture(GL_TEXTURE_2D, tex);
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void renderer_delete_tex(struct texture *texture) {
	if (texture) {
		GLuint *tex = &texture->tex;
		glDeleteTextures(1, tex);
		free(texture);
		texture = 0;
	}
}

GLuint CreateProgram(const char *name)
{
	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	char vert_path[64], frag_path[64];
	sprintf(vert_path, "shaders/%s.vert", name);
	sprintf(frag_path, "shaders/%s.frag", name);
	char *vert_src_handle = GetShaderSource(vert_path);
	char *frag_src_handle = GetShaderSource(frag_path);
	const char *vert_src = vert_src_handle;
	const char *frag_src = frag_src_handle;
	glShaderSource(vert, 1, &vert_src, NULL);
	glShaderSource(frag, 1, &frag_src, NULL);
	glCompileShader(vert);
	GLint success = 0;
	glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		GLint logSize = 0;
		glGetShaderiv(vert, GL_INFO_LOG_LENGTH, &logSize);
		char *errorLog = malloc(logSize*sizeof(char));
		glGetShaderInfoLog(vert, logSize, &logSize, errorLog);
		printf("%s\n", errorLog);
		free(errorLog);
	}
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		GLint logSize = 0;
		glGetShaderiv(frag, GL_INFO_LOG_LENGTH, &logSize);
		char *errorLog = malloc(logSize*sizeof(char));
		glGetShaderInfoLog(frag, logSize, &logSize, errorLog);
		printf("%s\n", errorLog);
		free(errorLog);
	}
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glLinkProgram(prog);
	glDeleteShader(vert);
	glDeleteShader(frag);
	free(vert_src_handle);
	free(frag_src_handle);
	return prog;
}

char *GetShaderSource(const char *src_file)
{
	char *buffer = 0;
	long lenght;
	FILE *f = fopen(src_file, "rb");
	if (f == NULL) {
		fprintf(stderr, "fopen on %s failed\n", src_file);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	lenght = ftell(f);
	fseek(f, 0, SEEK_SET);
	buffer = malloc(lenght);
	if (buffer == NULL) {
		fprintf(stderr, "malloc failed\n");
		fclose(f);
		return NULL;
	}

	fread(buffer, 1, lenght, f);
	fclose(f);
	buffer[lenght-1] = '\0';
	return buffer;
}
