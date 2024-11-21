class SensorPopup {
    static template = null; // Static property to hold the template

    constructor(sensor) {
        this.sensor = sensor; // Associated sensor
        this.popup = null; // Will hold the popup element
        this.isDragging = false; // Track if the popup is being dragged
        this.offsetX = 0;
        this.offsetY = 0;

        this.createPopup(); // Create the popup using the template
    }

    static async loadTemplate() {
        if (!this.template) {
            try {
                const response = await fetch('popup.html');
                this.template = await response.text(); // Store the loaded template
            } catch (error) {
                console.error('Error loading popup.html:', error);
            }
        }
    }


    createPopup() {
        if (!SensorPopup.template) {
            console.error('Popup template not loaded. Call SensorPopup.loadTemplate() before creating instances.');
            return;
        }
    
        this.popup = document.createElement('div');
        this.popup.className = 'sensor-popup';
        this.popup.style.position = 'absolute';
        this.popup.style.display = 'none';
        document.body.appendChild(this.popup);

        this.render(); // Initial rendering of content
        this.addListeners();
    }

    render(){
        // Populate template with sensor data
        const html = SensorPopup.template
            .replace(/{{id}}/g, this.sensor.id)
            .replace(/{{x}}/g, this.sensor.position.x.toFixed(1))
            .replace(/{{y}}/g, this.sensor.position.y.toFixed(1))
            .replace(/{{z}}/g, this.sensor.position.z.toFixed(1))
            .replace(/{{estX}}/g, this.sensor.estimatedpos?.x.toFixed(1) || 'N/A')
            .replace(/{{estY}}/g, this.sensor.estimatedpos?.y.toFixed(1) || 'N/A')
            .replace(/{{estZ}}/g, this.sensor.estimatedpos?.z.toFixed(1) || 'N/A')
            .replace(/{{posError}}/g, this.sensor.getPositionError().toFixed(2) || 'N/A')
            .replace(/{{variance}}/g, this.sensor.variance.toFixed(2))
            .replace(/{{antennaDelay}}/g, this.sensor.AntennaDelay.toFixed(2))
            .replace(/{{estAntennaDelay}}/g, this.sensor.estAntennaDelay.toFixed(2))
            .replace(/{{antennaError}}/g, this.sensor.getAntennaDelayError().toFixed(2) || 'N/A')
            .replace(/{{isAnchor}}/g, this.sensor.isAnchor ? 'checked' : '')
            .replace(/{{N}}/g, this.sensor.N);

        this.popup.innerHTML = html;
    }
    update() {
        if (!this.popup) {
            console.error("Popup element not initialized.");
            return;
        }
    
        // Update static text fields directly using IDs
        this.popup.querySelector("#sensor-x").textContent = this.sensor.position.x.toFixed(1);
        this.popup.querySelector("#sensor-y").textContent = this.sensor.position.y.toFixed(1);
        this.popup.querySelector("#sensor-z").textContent = this.sensor.position.z.toFixed(1);
    
        this.popup.querySelector("#sensor-est-x").textContent = this.sensor.estimatedpos?.x.toFixed(1) || "N/A";
        this.popup.querySelector("#sensor-est-y").textContent = this.sensor.estimatedpos?.y.toFixed(1) || "N/A";
        this.popup.querySelector("#sensor-est-z").textContent = this.sensor.estimatedpos?.z.toFixed(1) || "N/A";
    
        this.popup.querySelector("#sensor-pos-error").textContent = this.sensor.getPositionError().toFixed(2) || "N/A";
        this.popup.querySelector("#sensor-variance").textContent = this.sensor.variance.toFixed(2);
        this.popup.querySelector("#sensor-antenna-delay").textContent = this.sensor.AntennaDelay.toFixed(2);
        this.popup.querySelector("#sensor-est-antenna-delay").textContent = this.sensor.estAntennaDelay.toFixed(2);
        this.popup.querySelector("#sensor-antenna-error").textContent = this.sensor.getAntennaDelayError().toFixed(2) || "N/A";
    
        // Update interactive elements
        this.popup.querySelector(`#N-${this.sensor.id}`).value = this.sensor.N;
        this.popup.querySelector(`#isAnchor-${this.sensor.id}`).checked = this.sensor.isAnchor;
        this.popup.querySelector(`#showParticles-${this.sensor.id}`).checked = this.sensor.showParticles || false;
        this.popup.querySelector(`#showEstimation-${this.sensor.id}`).checked = this.sensor.showEstimation || false;
        this.popup.querySelector(`#showGroundTruth-${this.sensor.id}`).checked = this.sensor.showGroundTruth || false;
    }
    
    
    addListeners() {
        const header = this.popup.querySelector('.popup-header');
        const closeButton = this.popup.querySelector('.popup-close');
    
        // Add listeners for draggable functionality
        header.addEventListener('mousedown', (event) => this.startDrag(event));
        window.addEventListener('mousemove', (event) => this.drag(event));
        window.addEventListener('mouseup', () => this.endDrag());
    
        // Close button
        closeButton.addEventListener('click', () => this.hide());
    
        // Collect all checkboxes and inputs
        const checkboxes = this.popup.querySelectorAll('input[type="checkbox"]');
        const numberInputs = this.popup.querySelectorAll('input[type="number"]');
    
        // Handle checkboxes
        checkboxes.forEach((checkbox) => {
            checkbox.addEventListener('change', (event) => {
                const field = checkbox.id.split('-')[0]; // Get the property name (e.g., 'anchor')
                this.sensor[field] = checkbox.checked; // Update sensor property
                this.sensor.updateProperties(); // Reflect changes in the sensor
            });
        });
    
        // Handle number inputs
        numberInputs.forEach((input) => {
            input.addEventListener('input', (event) => {
                const field = input.id.split('-')[0]; // Get the property name (e.g., 'N')
                this.sensor[field] = parseInt(input.value, 10) || 0; // Update sensor property
                this.sensor.updateProperties(); // Reflect changes in the sensor
            });
        });
    
        // Handle "Update from Anchors" button
        const updateFromAnchorsButton = this.popup.querySelector(`#updateFromAnchors-${this.sensor.id}`);
        updateFromAnchorsButton.addEventListener('click', () => {
            console.log(`Updating sensor ${this.sensor.id} from anchors`);
            this.sensor.updateFromAnchors();
        });
    
        // Handle "Update from Mesh" button
        const updateFromMeshButton = this.popup.querySelector(`#updateFromMesh-${this.sensor.id}`);
        updateFromMeshButton.addEventListener('click', () => {
            console.log(`Updating sensor ${this.sensor.id} from mesh`);
            this.sensor.updateFromMesh();
        });
    }
    

    startDrag(event) {
        this.isDragging = true;
        this.offsetX = event.clientX - this.popup.offsetLeft;
        this.offsetY = event.clientY - this.popup.offsetTop;
        this.popup.style.cursor = 'grabbing';
    }

    drag(event) {
        if (this.isDragging) {
            this.popup.style.left = `${event.clientX - this.offsetX}px`;
            this.popup.style.top = `${event.clientY - this.offsetY}px`;
        }
    }

    endDrag() {
        this.isDragging = false;
        this.popup.style.cursor = 'grab';
    }

    show(event) {
        this.popup.style.left = `${event.clientX + 10}px`;
        this.popup.style.top = `${event.clientY + 10}px`;
        this.popup.style.display = 'block';
    }

    hide() {
        this.popup.style.display = 'none';
    }
}

export { SensorPopup };