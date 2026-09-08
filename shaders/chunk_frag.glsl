#version 460 core
out vec4 FragColor;

uniform sampler2D heightMap;

uniform sampler2D u_terrainNormal;
uniform sampler2D u_waterNormal;

uniform samplerCube u_skybox;
uniform sampler2D u_waterNormal2;
uniform float u_time;

uniform float u_amplitude;
uniform float u_chunkWidth;
uniform vec3 u_viewPos;
uniform float u_texScale;

uniform vec3 u_terrainColor;
uniform vec3 u_waterColor;
uniform vec3 u_lightColor;
uniform vec3 u_ambientColor;
uniform vec3 u_snowColor;

uniform float u_snowSlopeMax;
uniform float u_snowSlopeMin;

in VS_OUT {
  vec2 TexCoords;
  float Height;
  vec3 Normal;
  vec3 TangentFragPos;
  vec3 TangetLightDir;
  vec3 TangetViewPos;
  vec3 WorldPos;
  vec3 WorldNormal;
} fs_in;

void main() {
  vec2 tiledUV = fs_in.TexCoords * (u_chunkWidth / u_texScale);

  vec3 tangentNormal;
  if (fs_in.Height > 0) {
    tangentNormal = texture(u_terrainNormal, tiledUV).rgb;
  } else {
    // two UV sets scrolling in different directions at different speeds
    vec2 uv1 = tiledUV + vec2(0.03, 0.01) * u_time;
    vec2 uv2 = tiledUV + vec2(-0.01, 0.02) * u_time;

    vec3 n1 = texture(u_waterNormal, uv1).rgb;
    vec3 n2 = texture(u_waterNormal2, uv2).rgb;

    float blend = sin(u_time * 0.4) * 0.5 + 0.5;
    tangentNormal = mix(n1, n2, blend);
  }
  tangentNormal = normalize(tangentNormal * 2.0 - 1.0);

  vec3 lightDir = normalize(fs_in.TangetLightDir);
  vec3 viewDir = normalize(fs_in.TangetViewPos - fs_in.TangentFragPos);
  vec3 halfDir = normalize(lightDir + viewDir);

  vec3 ambient = 0.15 * u_ambientColor;
  float NdotL = dot(tangentNormal, lightDir);
  float diffuse = NdotL * 0.5 + 0.5;
  diffuse *= diffuse;

  float shininess = 4.0;
  float specularStrength = 0.3;
  float fresnel = 0.0;
  vec3 color = u_waterColor;

  if (fs_in.Height > 0) {
    // --- land ---
    vec2 gradient = texture(heightMap, fs_in.TexCoords).gb;
    float steepness = length(gradient);
    float snowSlope = smoothstep(u_snowSlopeMax, u_snowSlopeMin, steepness);
    float heightThreshold = -u_amplitude * 0.2;
    float heightFade = smoothstep(
        heightThreshold - u_amplitude * 0.15,
        heightThreshold + u_amplitude * 0.15,
        fs_in.Height);
    color = mix(u_terrainColor, u_snowColor, snowSlope * heightFade);

    shininess = 4.0;
    specularStrength = 0.3;
  } else {
    // --- water ---
    shininess = 1024.0;

    // Fresnel in tangent space (viewDir already tangent-space)
    fresnel = pow(1.0 - max(dot(tangentNormal, viewDir), 0.0), 4.0);
    specularStrength = mix(0.8, 3.5, fresnel);

    // Skybox reflection in world space
    vec3 worldViewDir = normalize(fs_in.WorldPos - u_viewPos);
    // perturb world normal with water ripple (tangentNormal.xy drives distortion)
    vec3 perturbedWorldNormal = normalize(fs_in.WorldNormal + vec3(tangentNormal.x, 0.0, tangentNormal.y) * 0.15);
    vec3 reflectDir = reflect(worldViewDir, perturbedWorldNormal);
    vec3 skyReflect = texture(u_skybox, reflectDir).rgb;

    // blend flat water color with sky reflection based on fresnel
    color = mix(u_waterColor, skyReflect, fresnel * 1.0);
  }

  float spec = pow(max(dot(tangentNormal, halfDir), 0.0), shininess);
  float specular = spec * specularStrength;

  vec3 result;
  if (fs_in.Height <= 0) {
    // water gets dimmer diffuse so the reflection reads clearly
    float waterDiffuse = NdotL * 0.3 + 0.3;
    result = color * (ambient + waterDiffuse * u_lightColor)
        + u_lightColor * specular;
  } else {
    result = color * (ambient + diffuse * u_lightColor)
        + u_lightColor * specular;
  }

  FragColor = vec4(result, 1.0);
}
