// PARANOIA — stress-driven post-processing. Port of the FRAG shader in
// src/postfx.js, adapted to a Godot canvas_item fullscreen pass that samples
// the rendered frame via hint_screen_texture (PostFX.gd drives the uniforms).
// NOTE: the JS vignette bug is fixed here — min brightness 0.55, not 0.35.
shader_type canvas_item;
render_mode unshaded;

uniform sampler2D screen_tex : hint_screen_texture, filter_linear;
uniform float u_time = 0.0;
uniform float u_stress = 0.0;        // 0..1
uniform float u_glitch = 0.0;        // 0..1 momentary spikes
uniform float u_flash_white = 0.0;
uniform float u_flash_black = 0.0;
uniform float u_battery = 1.0;       // 0..1

float rand(vec2 co) {
	return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void fragment() {
	vec2 uv = SCREEN_UV;
	float s = u_stress;
	float g = u_glitch;

	// 1. glitch: horizontal block displacement (24 rows, 18 Hz)
	if (g > 0.01) {
		float block = floor(uv.y * 24.0);
		float shift = (rand(vec2(block, floor(u_time * 18.0))) - 0.5) * 0.12 * g;
		if (rand(vec2(block, floor(u_time * 7.0))) > 0.72) {
			uv.x += shift;
		}
	}

	// 2. chromatic aberration grows with stress + glitch
	float ca = 0.0015 + s * 0.006 + g * 0.01;
	vec2 dir = uv - 0.5;
	vec3 col;
	col.r = texture(screen_tex, uv + dir * ca).r;
	col.g = texture(screen_tex, uv).g;
	col.b = texture(screen_tex, uv - dir * ca).b;

	// 3. film grain
	vec2 res = 1.0 / SCREEN_PIXEL_SIZE;
	float grain = rand(uv * res + fract(u_time) * 100.0) - 0.5;
	col += grain * (0.045 + s * 0.16 + g * 0.1);

	// 4. desaturate with stress
	float lum = dot(col, vec3(0.299, 0.587, 0.114));
	col = mix(col, vec3(lum), s * 0.35);

	// 5. scanlines at high stress
	col -= sin(uv.y * 900.0 + u_time * 12.0) * 0.5 * (s * s * 0.08 + g * 0.06);

	// 6. vignette (FIXED: min brightness 0.55, not the JS 0.35)
	float vig = smoothstep(1.15 - s * 0.35, 0.25, length(dir) * 1.4);
	col *= mix(0.55, 1.0, vig);

	// 7. low-battery dimming + flicker (9 Hz)
	float bat = clamp(u_battery, 0.0, 1.0);
	if (bat < 0.2) {
		float fl = step(0.5, rand(vec2(floor(u_time * 9.0), 1.0))) * (0.2 - bat) * 2.5;
		col *= 1.0 - fl * 0.6;
	}

	// 8. flashes
	col = mix(col, vec3(1.0), clamp(u_flash_white, 0.0, 1.0));
	col = mix(col, vec3(0.0), clamp(u_flash_black, 0.0, 1.0));

	COLOR = vec4(col, 1.0);
}
