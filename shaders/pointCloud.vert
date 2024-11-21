varying vec3 vWorldPosition;

void main() {
    vec4 worldPos = modelViewMatrix * vec4(position, 1.0);
    gl_Position = projectionMatrix * worldPos;
    vWorldPosition = worldPos.xyz;
}
