uniform vec3 uBaseColor;        // Base color of the spheres
uniform vec3 uLightDirection;  // Direction of the light
uniform vec2 uResolution;      // Screen resolution
uniform vec3 uParticlePositions[100]; // Array of particle positions
uniform float uSphereRadius;   // Radius of each sphere
uniform float uBlendFactor;    // Blending factor for smooth union

// Smooth union for blending spheres
float opSmoothUnion(float d1, float d2, float k) {
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h);
}

// Signed Distance Function (SDF) for a sphere
float sdSphere(vec3 p, vec3 center, float radius) {
    return length(p - center) - radius;
}

// Combine multiple spheres using SDF blending
float map(vec3 p) {
    float d = 1e6; // Initialize to a large distance
    for (int i = 0; i < 100; i++) {
        d = opSmoothUnion(sdSphere(p, uParticlePositions[i], uSphereRadius), d, uBlendFactor);
    }
    return d;
}

// Calculate normals from the SDF gradient
vec3 calcNormal(vec3 p) {
    const float h = 1e-5;
    const vec2 k = vec2(1, -1);
    return normalize(
        k.xyy * map(p + k.xyy * h) +
        k.yyx * map(p + k.yyx * h) +
        k.yxy * map(p + k.yxy * h) +
        k.xxx * map(p + k.xxx * h)
    );
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec3 rayOri = vec3((uv - 0.5) * vec2(uResolution.x / uResolution.y, 1.0) * 6.0, 3.0);
    vec3 rayDir = vec3(0.0, 0.0, -1.0);

    float depth = 0.0;
    vec3 p;

    for (int i = 0; i < 64; i++) {
        p = rayOri + rayDir * depth;
        float dist = map(p);
        depth += dist;
        if (dist < 1e-6) break;
    }

    depth = min(60.0, depth);
    vec3 n = calcNormal(p);

    float light = max(0.0, dot(n, uLightDirection));
    vec3 col = uBaseColor * (0.5 + 0.5 * light);
    col *= exp(-depth * 0.15);
    gl_FragColor = vec4(col, 1.0 - (depth - 0.5) / 2.0);
}
