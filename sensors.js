import * as THREE from 'three';
// import { ShaderMaterial, Points, BufferGeometry, Float32BufferAttribute } from 'three';
import { SensorPopup } from './sensorPopup.js';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import FilterWrapper from './filterWrapper.js';

function gaussianRandom(mean=0, stdev=1) {
    const u = 1 - Math.random(); // Converting [0,1) to (0,1]
    const v = Math.random();
    const z = Math.sqrt( -2.0 * Math.log( u ) ) * Math.cos( 2.0 * Math.PI * v );
    // Transform to the desired mean and standard deviation:
    return z * stdev + mean;
}
const MAXDISPLAYN = 500;

class Sensor {
    static sensorId = 0;
    static sensorList = [];
    static model = null; // Static property to store the loaded GLTF model
    static loader = new GLTFLoader();
    static offset = new THREE.Vector3(0, -1.2, 0);

    constructor(x, y, z, scene, { isAnchor = false, N = 100 } = {}) {
        this.scene = scene;
        this.position = new THREE.Vector3(x, y, z);
        this.estimatedpos = null;
        this.AntennaDelay = 0;
        this.estAntennaDelay = 0;
        this.variance = 0;
        this.isAnchor = isAnchor;
        this.isInit = false;
        this.estimated = null;
        this.estimatedVar = null;
        if(isAnchor){
            this.variance = 0.1; //high confidence
            this.estimatedpos = new THREE.Vector3(x, y, z);
            this.AntennaDelay = 0;
            this.estimated = {x: x, y: y, z: z, d: this.AntennaDelay};
            this.estimatedVar = {x: this.variance, y: this.variance, z: this.variance, d: 0.1};
        } else{
            this.variance = 1000; //low confidence
            this.estimatedpos = null;
            this.AntennaDelay = 1+Math.random()/10;
            this.estAntennaDelay = 1;
        }
        this.N = N;
        this.oldN = N;
        this.filter = new FilterWrapper(N, false); // Add Filter instance
        this.id = Sensor.sensorId++;
        Sensor.sensorList.push(this);
        this.popup = null; // Initialize popup reference
        this.sphere = null;
        this.showParticles= false;
        this.showEstimation = false;
        this.showGroundTruth = true;
        this.estimatedParticule= null;
        this.particles = [];
        this.createMesh();
        this.initParticles();
        this.updateProperties();

        // wait for filter to be ready.
        //  test the filter wrapper
        
    }

    static loadModel(path) {
        if (Sensor.model) return Promise.resolve(Sensor.model);

        return new Promise((resolve, reject) => {
            Sensor.loader.load(
                path,
                (gltf) => {
                    Sensor.model = gltf.scene; // Cache the model for reuse
                    resolve(Sensor.model);
                },
                undefined,
                (error) => {
                    console.error('An error occurred while loading the GLTF model:', error);
                    reject(error);
                }
            );
        });
    }
    async createMesh() {
        if (!Sensor.model) {
            await Sensor.loadModel('assets/HexSense.gltf'); // Path to your GLTF file
        }
        this.mesh = Sensor.model.clone(); // Clone the cached model for this instance
        this.mesh.position.set(this.position.x, this.position.y, this.position.z);
        this.mesh.position.add(Sensor.offset);
        this.mesh.rotation.set(-Math.PI / 2, 0, 0); // Adjust rotation if necessary
        this.mesh.scale.set(0.01, 0.01, 0.01); // Adjust scale if necessary
        this.scene.add(this.mesh);

        // Ensure userData is set for all child meshes
        this.mesh.traverse((node) => {
            if (node.isMesh) {
                node.material = node.material.clone();
                node.userData.sensor = this;
            }
        });
        
    }

    initParticles() {
        //  init the array of particules as a array of spheres in the scene
        this.particles = [];
        
        for (let i = 0; i < this.N && i< MAXDISPLAYN ; i++) {
            const particle = new THREE.Vector3(
                this.position.x + gaussianRandom(0, 20),
                this.position.y + gaussianRandom(0, 20),
                this.position.z + gaussianRandom(0, 20)
            );
            const geometry = new THREE.SphereGeometry(0.1, 32, 32);
            const material = new THREE.MeshBasicMaterial({ color: 0x00EE00, opacity: 0.3, transparent: true });
            const sphere = new THREE.Mesh(geometry, material);
            sphere.position.set(particle.x, particle.y, particle.z);
            this.particles.push(sphere);
            this.scene.add(sphere);
        }
        const estimGeometry = new THREE.SphereGeometry(0.1, 32, 32);
        const estimMaterial = new THREE.MeshBasicMaterial({ color: 0x0000EE, opacity: 0.5, transparent: true });
        this.estimatedParticule = new THREE.Mesh(estimGeometry, estimMaterial);
        this.scene.add(this.estimatedParticule);
    }

    removeFromScene() {
        this.scene.remove(this.mesh);
        this.particles.forEach(particle => {
            this.scene.remove(particle);
        });
        this.scene.remove(this.estimatedParticule);
        if (this.popup) {
            this.popup.hide();
            // delete the popup element
            this.popup.popup.remove();
        }
    }

    startPulse() {
        if (!this.sphere) {
            const geometry = new THREE.SphereGeometry(0.1, 32, 32);
            const material = new THREE.MeshBasicMaterial({ color: 0xAA0000, opacity: 1.0, transparent: true });
            this.sphere = new THREE.Mesh(geometry, material);
            this.sphere.position.set(this.position.x, this.position.y, this.position.z);
            this.scene.add(this.sphere);
        }
    }

    anim(delta) {
        // pulsating sphere
        if (this.sphere) {
            this.sphere.scale.multiplyScalar(1.1);
            this.sphere.material.opacity *= 0.95;
            if (this.sphere.scale.x > 200) {
                this.scene.remove(this.sphere);
                this.sphere = null;
            }
        }
    }

    showPopup(event) {
        if (!this.popup) {
            this.popup = new SensorPopup(this); // Create popup if it doesn't exist
        }
        this.popup.show(event); // Show popup at mouse position
        // for(let i = 0; i < this.N; i++){
        //     this.filter.set(i, gaussianRandom(), gaussianRandom(), gaussianRandom(), gaussianRandom(), gaussianRandom());
        // }
        // for(let i = 0; i < this.N; i++){
        //     console.log(this.filter.get(i));
        // }
    }

    
    updateProperties() {
        // Update mesh color based on isAnchor
        const newColor = this.isAnchor ? 0xfc9803 : 0xEEEEEE;
        // this.mesh.material.color.set(newColor);
        this.mesh.visible = this.showGroundTruth;
        // Traverse the GLTF model and update the material color of all meshes
        this.mesh.traverse((node) => {
            if (node.isMesh) {
                if (node.material && node.material.color) {
                    node.material.color.set(newColor); // Update the material color
                } else {
                    console.warn(`Mesh ${node.name} does not have a color property.`);
                }
            }
        });
        //  update particule visibility
        this.particles.forEach(particle => {
            particle.visible = this.showParticles;
        });
        this.estimatedParticule.visible = this.showEstimation;
        if (this.oldN !== this.N) {
            // this.filter.setN(this.N);
            this.filter.N = this.N;
            this.oldN = this.N;
            this.initParticles();
        }
        if(this.isAnchor){
            this.variance = 0.01; //high confidence
            this.estimatedpos = new THREE.Vector3(this.position.x, this.position.y, this.position.z);
            this.AntennaDelay = 0;
        }
    }
    
    static Measure(A, B){
        let gtDistance = A.position.distanceTo(B.position);
        // let AntennaNoise= A.AntennaDelay*gtDistance + B.AntennaDelay*gtDistance;
        let AntennaNoise = 0;
        let gNoise = gaussianRandom(0, 0.1);
        // let gNoise = 0;
        let distance = gtDistance + AntennaNoise + gNoise;
        console.log(`Measuring distance between sensor ${A.id} and sensor ${B.id}`);
        console.log(`disatnce is: ${distance}`);
        return distance
    }
    updateParticles() {
        //  update the position of the particules
        for (let i = 0; i < this.N && i< MAXDISPLAYN; i++) {
            const particle = this.filter.get(i);
            this.particles[i].position.set(particle.x, particle.y, particle.z);
        }
        this.estimatedParticule.position.set(this.estimatedpos.x, this.estimatedpos.y, this.estimatedpos.z);
        this.estimatedParticule.scale.set(this.estimatedVar.x*2, this.estimatedVar.y*2, this.estimatedVar.z*2);
    }


    updateFilter(other){
        let measure = Sensor.Measure(this, other);
        // let otherx = other.estimatedpos.x;
        // let othery = other.estimatedpos.y;
        // let otherz = other.estimatedpos.z;
        // let otherd = other.estAntennaDelay;
        // let otherc = other.variance;
        // console.log(`Sensor ${this.id}: Updating filter with measurement from sensor ${other.id}`);
        // console.log(`Sensor ${this.id}: Measurement: ${measure}`);
        // console.log(`Sensor ${this.id}: Other sensor position: (${otherx}, ${othery}, ${otherz})`);
        // console.log(`Sensor ${this.id}: Other sensor antenna delay: ${otherd}`);
        // console.log(`Sensor ${this.id}: Other sensor confidence: ${otherc}`);
        // this.filter.update(measure, otherx, othery, otherz, otherd, otherc);
        const otherPos = { x: other.estimated.x, y: other.estimated.y, z: other.estimated.z, d: other.estimated.d };
        const otherVar = { x: other.estimatedVar.x, y: other.estimatedVar.y, z: other.estimatedVar.z, d: other.estimatedVar.d };
        const P_NLOSS = 0.0;
        this.filter.update(measure, P_NLOSS, otherPos, otherVar);
    }
    getPositionError() {
        if (!this.estimatedpos) return 0;
        return this.position.distanceTo(this.estimatedpos);
    }
    
    getAntennaDelayError() {
        return Math.abs(this.AntennaDelay - this.estAntennaDelay);
    }

    updateFromAnchors() {
        this.startPulse();
        console.log(`Sensor ${this.id}: Updating properties from anchor sensors.`);
        // Logic for updating from anchors
        for(let i = 0; i < Sensor.sensorList.length; i++){
            if(Sensor.sensorList[i] !== this && Sensor.sensorList[i].isAnchor){
                this.updateFilter(Sensor.sensorList[i]);
            }
        }
        this.isInit = true;
        // console.log(this.filter.getEstimatedPosition());
        this.estimated = this.filter.getEstimatedPosition();
        this.estimatedVar = this.filter.getEstimatedVariance();
        this.estimatedpos = new THREE.Vector3(this.estimated.x, this.estimated.y, this.estimated.z);
        this.variance = (this.estimatedVar.x+this.estimatedVar.y+this.estimatedVar.z)/3;
        this.estAntennaDelay = this.estimated.d;
        this.updateParticles();
        if (this.popup){
            this.popup.update();
        }
    }
    
    updateFromMesh() {
        this.startPulse();
        console.log(`Sensor ${this.id}: Updating properties from the mesh.`);
        if (this.isInit) {
            for(let i = 0; i < Sensor.sensorList.length; i++){
                if(Sensor.sensorList[i] !== this && !Sensor.sensorList[i].isAnchor && Sensor.sensorList[i].isInit){
                    this.updateFilter(Sensor.sensorList[i]);
                }
            }
            this.estimated = this.filter.getEstimatedPosition();
            this.estimatedVar = this.filter.getEstimatedVariance();
            this.estimatedpos = new THREE.Vector3(this.estimated.x, this.estimated.y, this.estimated.z);
            this.variance = (this.estimated.x+this.estimated.y+this.estimated.z)/3;
            this.estAntennaDelay = this.estimated.d;
            this.updateParticles();
        } else {
            console.log('Sensor not initialized. Cannot update properties.');
        }
        if (this.popup){
            this.popup.update();
        }
    }
    
    

}

export { Sensor };
