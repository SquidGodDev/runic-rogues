shader_type spatial;
render_mode cull_disabled;

#define USE_ALPHA 0
#define USE_ALPHA_CUTOFF 0
#define USE_EMISSION 0
#define USE_REFLECTIONS 0
#define USE_NORMAL_MAP 0
#define USE_OCCLUSION 0
#define USE_ANISOTROPY 0
#define USE_BACKLIGHT 0
#define USE_REFRACTION 0

#include "includes/base-cel-shader.gdshaderinc"

group_uniforms BaseProperties;
uniform vec4 color : source_color = vec4(1.0, 1.0, 1.0, 1.0);
uniform sampler2D base_texture : source_color;
uniform vec4 specular : source_color = vec4(0.3, 0.3, 0.3, 0.5);
uniform sampler2D specular_texture : hint_default_white;
uniform vec4 fresnel : source_color = vec4(0.2, 0.2, 0.2, 0.3);
uniform sampler2D fresnel_texture : hint_default_white;
group_uniforms;

varying vec3 cel_specular_color;
varying float cel_specular_amount;
varying vec3 cel_fresnel_color;
varying float cel_fresnel_amount;

group_uniforms Tiling;
uniform vec2 uv_scale = vec2(1, 1);
uniform vec2 uv_offset = vec2(0, 0);
group_uniforms;

void vertex() {
	UV = UV * uv_scale.xy + uv_offset.xy;
}

void fragment() {
	ALBEDO = color.rgb * texture(base_texture, UV).rgb;

	cel_specular_color = specular.rgb * texture(specular_texture, UV).rgb;
	cel_specular_amount = specular.a * texture(specular_texture, UV).a;
	cel_fresnel_color = fresnel.rgb * texture(fresnel_texture, UV).rgb;
	cel_fresnel_amount = fresnel.a * texture(fresnel_texture, UV).a;
}

void light() {
	DIFFUSE_LIGHT += diffuse_light(
			ALBEDO,
			LIGHT_COLOR,
			LIGHT,
			NORMAL,
			ATTENUATION
	);

	SPECULAR_LIGHT += specular_light(
			LIGHT_COLOR,
			cel_specular_color,
			cel_specular_amount,
			NORMAL,
			VIEW,
			LIGHT,
			ATTENUATION
	);

	SPECULAR_LIGHT += fresnel_light(
			LIGHT_COLOR,
			cel_fresnel_color,
			cel_fresnel_amount,
			NORMAL,
			VIEW,
			LIGHT,
			ATTENUATION
	);
}
