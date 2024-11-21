import * as THREE from 'three';
// import { FirstPersonControls } from 'three/addons/controls/FirstPersonControls.js';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { Sensor } from './sensors.js';
import { SensorPopup } from './sensorPopup.js';


// Basic Three.js setup
const container = document.getElementById('container');
var clock = new THREE.Clock();
// Scene, Camera, Renderer setup
const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
const renderer = new THREE.WebGLRenderer();
renderer.setSize(window.innerWidth, window.innerHeight);
container.appendChild(renderer.domElement);

// Load the spherical skybox texture
const textureLoader = new THREE.TextureLoader();
const skyboxTexture = textureLoader.load('assets/sk2.jpg', () => {
    // Set the texture to wrap around a large sphere
    const skyboxGeometry = new THREE.SphereGeometry(500, 60, 40); // Large sphere
    const skyboxMaterial = new THREE.MeshBasicMaterial({
        map: skyboxTexture,
        side: THREE.BackSide // Render the inside of the sphere
    });
    const skybox = new THREE.Mesh(skyboxGeometry, skyboxMaterial);
    scene.add(skybox);
});
// Add a grid for the ground
const gridHelper = new THREE.GridHelper(100, 100); // Grid size: 100 units, 100 divisions
scene.add(gridHelper);

// Add lighting
const ambientLight = new THREE.AmbientLight(0x404040); // Soft white light
scene.add(ambientLight);

const directionalLight = new THREE.DirectionalLight(0xffffff, 0.5); // Directional light
directionalLight.position.set(10, 10, 10);
scene.add(directionalLight);

// Camera setup
camera.position.set(20, 10, 10);

// Add a transparent cube for the center object
const cubeGeometry = new THREE.BoxGeometry(1, 1, 1);
const cubeMaterial = new THREE.MeshBasicMaterial({
    color: 0xADD8E6, // Light blue color
    opacity: 0.5,    // Set transparency level
    transparent: true, // Enable transparency
});
const centerObject = new THREE.Mesh(cubeGeometry, cubeMaterial);
centerObject.position.set(0, 0.5, 0); // Center object at origin, slightly raised
scene.add(centerObject); // Add the cube to the center object

// OrbitControls setup
const controls = new OrbitControls(camera, renderer.domElement);
controls.target = centerObject.position;
controls.enableDamping = true; // Smooth rotation
controls.dampingFactor = 0.05;

// Keyboard layout handling
let isZQSD = false; // Default to WASD
function detectKeyboardLayout() {
    const lang = navigator.language || navigator.userLanguage;
    isZQSD = lang.startsWith('fr') || lang.startsWith('de'); // Adjust for European layouts
}
detectKeyboardLayout();

// Movement logic
const moveSpeed = 0.5;
function moveCenter(key) {
    // Get camera directions
    const forward = new THREE.Vector3();
    const right = new THREE.Vector3();
    const up = new THREE.Vector3(0, 1, 0); // Fixed up vector

    camera.getWorldDirection(forward); // Forward is the direction the camera is looking at
    forward.y = 0; // Flatten to horizontal plane
    forward.normalize();

    right.crossVectors(forward, up).normalize(); // Right is perpendicular to forward and up

    // Adjust movement based on controls
    if (isZQSD) {
        // ZQSD Controls (European)
        if (key === 'KeyZ') centerObject.position.addScaledVector(forward, moveSpeed); // Forward
        if (key === 'KeyS') centerObject.position.addScaledVector(forward, -moveSpeed); // Backward
        if (key === 'KeyQ') centerObject.position.addScaledVector(right, -moveSpeed); // Left
        if (key === 'KeyD') centerObject.position.addScaledVector(right, moveSpeed); // Right
    } else {
        // WASD Controls (American)
        if (key === 'KeyW') centerObject.position.addScaledVector(forward, moveSpeed); // Forward
        if (key === 'KeyS') centerObject.position.addScaledVector(forward, -moveSpeed); // Backward
        if (key === 'KeyA') centerObject.position.addScaledVector(right, -moveSpeed); // Left
        if (key === 'KeyD') centerObject.position.addScaledVector(right, moveSpeed); // Right
    }
    // Move camera vertically
    if (key === 'ShiftLeft') centerObject.position.y += moveSpeed; // Move up
    if (key === 'ControlLeft') centerObject.position.y -= moveSpeed; // Move down
    if (key === 'Escape') {
        // Iterate through sensors and hide any open popups
        Sensor.sensorList.forEach(sensor => {
            if (sensor.popup) {
                sensor.popup.hide();
            }
        });
    }
    if (key === 'Enter'){
        // create a new sensor at the center object
        new Sensor(centerObject.position.x, centerObject.position.y, centerObject.position.z, scene, { isAnchor: false, N: 1000 });
    }
    // if press delete, delete the sensor
    if (key === 'Backspace'){
        // delete the sensor that intersects with the center object
        Sensor.sensorList.forEach(sensor => {
            if (sensor.mesh.position.distanceTo(centerObject.position) < 1.5){
                console.log('delete sensor: ', sensor.id);
                sensor.removeFromScene();
                Sensor.sensorList.splice(Sensor.sensorList.indexOf(sensor), 1);
            }
        });
    }
 
    // Update the orbit target to the new position
    controls.target.copy(centerObject.position);
}

// Keyboard input handling
window.addEventListener('keydown', (event) => {
    moveCenter(event.code);
});

// Ensure the popup template is loaded before creating sensors
await SensorPopup.loadTemplate();
await Sensor.loadModel('assets/HexSense.gltf'); // Load the GLTF model
// Create a few sensor objects
new Sensor(0, 0.5, 0, scene, { isAnchor: false, N: 1000 });
new Sensor(5, 0.5, 5, scene, { isAnchor: true, N: 100 });
new Sensor(10, 0.5, -15, scene, { isAnchor: true, N: 500 });
new Sensor(-10, 0.5, -15, scene, { isAnchor: true, N: 200 });
new Sensor(-15, 0.5, 15, scene, { isAnchor: true, N: 300 });
// Raycaster for detecting clicks
const raycaster = new THREE.Raycaster();
const mouse = new THREE.Vector2();

window.addEventListener('click', (event) => {
    mouse.x = (event.clientX / window.innerWidth) * 2 - 1;
    mouse.y = -(event.clientY / window.innerHeight) * 2 + 1;

    raycaster.setFromCamera(mouse, camera);
    const intersects = raycaster.intersectObjects(Sensor.sensorList.map(sensor => sensor.mesh));

    if (intersects.length > 0) {
        const sensor = intersects[0].object.userData.sensor;
        sensor.showPopup(event); // Show the popup for the clicked sensor
        // sensor.startPulse(); // Pulse the sensor
    }
});

// hover effect
const hoverPopup = document.createElement('div');
hoverPopup.className = 'hover-popup';
hoverPopup.style.position = 'absolute';
hoverPopup.style.display = 'none';
hoverPopup.style.pointerEvents = 'none'; // Prevent interference with mouse events
hoverPopup.style.backgroundColor = 'rgba(0, 0, 0, 0.8)';
hoverPopup.style.color = 'white';
hoverPopup.style.padding = '5px 10px';
hoverPopup.style.borderRadius = '5px';
hoverPopup.style.fontSize = '12px';
hoverPopup.style.zIndex = '1001'; // Ensure it appears above other elements
document.body.appendChild(hoverPopup);

window.addEventListener('mousemove', (event) => {
    // Update mouse position
    mouse.x = (event.clientX / window.innerWidth) * 2 - 1;
    mouse.y = -(event.clientY / window.innerHeight) * 2 + 1;

    // Update raycaster
    raycaster.setFromCamera(mouse, camera);

    // Detect intersections
    const intersects = raycaster.intersectObjects(Sensor.sensorList.map(sensor => sensor.mesh));
    if (intersects.length > 0) {
        const sensor = intersects[0].object.userData.sensor;

        // Update hover popup content and position
        hoverPopup.textContent = `Sensor ID: ${sensor.id}`;
        hoverPopup.style.left = `${event.clientX + 10}px`; // Offset to avoid cursor overlap
        hoverPopup.style.top = `${event.clientY + 10}px`;
        hoverPopup.style.display = 'block';
    } else {
        // Hide the hover popup when not hovering over a sensor
        hoverPopup.style.display = 'none';
    }
});




// Animation loop
function animate() {
    requestAnimationFrame(animate);
    let delta = clock.getDelta();
    // sensors.forEach(sensor => sensor.anim(delta));
    Sensor.sensorList.forEach(sensor => {
        sensor.anim(delta);
    });

    // Update raymarching time
    controls.update(delta);
    renderer.render(scene, camera);
}

// Handle window resize
window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
});

// Start the animation loop
animate();
