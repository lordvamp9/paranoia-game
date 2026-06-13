// PARANOIA v2 — GLSL shaders (embedded)
#pragma once

// ---------------------------------------------------------------- world VS
static const char* VS_BODY = R"GLSL(
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
#ifdef INSTANCED
in mat4 instanceTransform;
#endif
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform float uTime;
uniform int uSway;
out vec3 fragPos;
out vec2 fragUV;
out vec3 fragNormal;
out vec4 fragColor;
void main() {
#ifdef INSTANCED
    mat4 model = instanceTransform;
    vec3 wp = (model * vec4(vertexPosition, 1.0)).xyz;
    vec3 wn = normalize((model * vec4(vertexNormal, 0.0)).xyz);
    if (uSway == 1) {
        float k = clamp(vertexPosition.y * 0.35, 0.0, 1.0);
        wp.x += sin(uTime * 1.3 + wp.z * 0.35 + wp.x * 0.2) * 0.12 * k;
        wp.z += cos(uTime * 1.1 + wp.x * 0.3) * 0.08 * k;
    }
    gl_Position = mvp * vec4(wp, 1.0);
#else
    vec3 wp = (matModel * vec4(vertexPosition, 1.0)).xyz;
    vec3 wn = normalize((matNormal * vec4(vertexNormal, 0.0)).xyz);
    gl_Position = mvp * vec4(vertexPosition, 1.0);
#endif
    fragPos = wp;
    fragUV = vertexTexCoord;
    fragNormal = wn;
    fragColor = vertexColor;
}
)GLSL";

// ---------------------------------------------------------------- world FS
static const char* FS_WORLD = R"GLSL(#version 330
in vec3 fragPos;
in vec2 fragUV;
in vec3 fragNormal;
in vec4 fragColor;
uniform sampler2D texture0;
uniform sampler2D texNoise;
uniform sampler2D texShadow;
uniform vec4 colDiffuse;
uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightDir;
uniform float lightI;
uniform vec2 lightCut;     // cos(inner), cos(outer)
uniform float fogD;
uniform vec3 ambientC;
uniform mat4 lightVP;
uniform int shadowOn;
uniform float specK;
uniform float emisK;
uniform float fogMin;
out vec4 finalColor;

void main() {
    vec4 base = texture(texture0, fragUV) * colDiffuse * fragColor;
    // procedural micro-detail: perturb the normal with world-space noise
    vec3 N = normalize(fragNormal);
    vec3 nz = texture(texNoise, fragPos.xz * 0.22 + fragPos.yy * 0.13).rgb - 0.5;
    N = normalize(N + nz * 0.55);
    float dtl = 0.85 + texture(texNoise, fragPos.xz * 0.9).r * 0.3;
    base.rgb *= dtl;

    vec3 Lv = lightPos - fragPos;
    float d = length(Lv);
    vec3 L = Lv / max(d, 0.001);
    float atten = 1.0 / (1.0 + 0.10 * d + 0.020 * d * d);
    float theta = dot(-L, normalize(lightDir));
    float spot = smoothstep(lightCut.y, lightCut.x, theta);
    float diff = max(dot(N, L), 0.0);

    float sh = 1.0;
    if (shadowOn == 1 && spot > 0.01) {
        vec4 lp = lightVP * vec4(fragPos, 1.0);
        vec3 ndc = lp.xyz / lp.w * 0.5 + 0.5;
        if (ndc.x > 0.0 && ndc.x < 1.0 && ndc.y > 0.0 && ndc.y < 1.0 && ndc.z < 1.0) {
            float bias = max(0.0022 * (1.0 - dot(N, L)), 0.0008);
            vec2 ts = vec2(1.0 / 1024.0);
            sh = 0.0;
            for (int i = -1; i <= 1; i += 2)
            for (int j = -1; j <= 1; j += 2) {
                float pd = texture(texShadow, ndc.xy + vec2(i, j) * ts * 0.6).r;
                sh += (ndc.z - bias > pd) ? 0.0 : 0.25;
            }
        }
    }

    vec3 V = normalize(viewPos - fragPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * specK;

    float hemi = 0.65 + 0.35 * max(N.y, 0.0);
    vec3 light = ambientC * hemi + vec3(0.78, 0.84, 0.95) * lightI * atten * spot * sh * (diff * 0.92 + 0.10);
    vec3 col = base.rgb * light + vec3(0.8, 0.85, 1.0) * spec * lightI * atten * spot * sh;
    col += base.rgb * emisK; // unreal glow (the white house ignores the dark)

    float cd = length(viewPos - fragPos);
    float fog = clamp(exp(-fogD * cd), fogMin, 1.0);
    col = mix(vec3(0.004, 0.005, 0.012), col, fog);
    finalColor = vec4(col, base.a);
}
)GLSL";

// ---------------------------------------------------------------- depth FS
static const char* FS_DEPTH = R"GLSL(#version 330
void main() { }
)GLSL";

// ---------------------------------------------------------------- post FS
static const char* FS_POST = R"GLSL(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec2 uRes;        // internal resolution
uniform float uTime;
uniform float uStress;    // 0..1
uniform float uGlitch;
uniform float uFlashW;
uniform float uFlashB;
uniform float uBattery;   // 0..1
out vec4 finalColor;

float rand(vec2 co) { return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453); }

float bayer8(vec2 p) {
    int x = int(mod(p.x, 8.0));
    int y = int(mod(p.y, 8.0));
    int idx = (x ^ y) * 1; // placeholder, replaced below
    // classic 8x8 Bayer matrix via bit interleave
    int v = 0; int xc = x ^ y; int yc = y;
    v = ((xc & 1) << 5) | ((yc & 1) << 4) | ((xc & 2) << 2) | ((yc & 2) << 1) | ((xc & 4) >> 1) | ((yc & 4) >> 2);
    return float(v) / 64.0;
}

void main() {
    vec2 uv = fragTexCoord;
    float s = uStress;
    float g = uGlitch;

    // glitch: horizontal band displacement
    if (g > 0.01) {
        float band = floor(uv.y * 28.0);
        float r = rand(vec2(band, floor(uTime * 16.0)));
        if (r > 0.78) uv.x += (rand(vec2(band, floor(uTime * 9.0))) - 0.5) * 0.09 * g;
    }

    // chromatic aberration
    float ca = (0.0012 + s * 0.0045 + g * 0.008);
    vec2 dir = uv - 0.5;
    vec3 col;
    col.r = texture(texture0, uv + dir * ca).r;
    col.g = texture(texture0, uv).g;
    col.b = texture(texture0, uv - dir * ca).b;

    // film grain (animated, heavy: part of the look)
    float gr = rand(floor(uv * uRes) + vec2(fract(uTime * 13.7), fract(uTime * 7.3)) * 100.0) - 0.5;
    col += gr * (0.05 + s * 0.13 + g * 0.08);

    // desaturate + cold tint with stress
    float lum = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(col, vec3(lum) * vec3(0.92, 0.98, 1.06), 0.18 + s * 0.30);

    // ordered dithering + color quantization — the PSX-photo look
    vec2 pix = floor(uv * uRes);
    float dth = bayer8(pix) - 0.5;
    float levels = 13.0;
    col = floor(col * levels + dth * 1.05 + 0.5) / levels;

    // vignette
    float vig = smoothstep(1.18 - s * 0.34, 0.28, length(dir) * 1.42);
    col *= mix(0.30, 1.0, vig);

    // low battery flicker
    if (uBattery < 0.2) {
        float fl = step(0.55, rand(vec2(floor(uTime * 9.0), 1.0))) * (0.2 - uBattery) * 2.5;
        col *= 1.0 - fl * 0.6;
    }

    col = mix(col, vec3(1.0), clamp(uFlashW, 0.0, 1.0));
    col = mix(col, vec3(0.0), clamp(uFlashB, 0.0, 1.0));
    finalColor = vec4(col, 1.0);
}
)GLSL";
