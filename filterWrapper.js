class FilterWrapper {
    static module = null; // Shared module for all instances
    static modulePromise = null; // Promise for module initialization

    constructor(N, modelAntennaDelay = false) {
        this.filterInstance = null; // Instance-specific Filter object
        this.modelAntennaDelay = modelAntennaDelay;
        // Ensure the shared module is loaded
        if (!FilterWrapper.modulePromise) {
            FilterWrapper.modulePromise = FilterModule().then((mod) => {
                FilterWrapper.module = mod;
                console.log('FilterModule loaded and shared among instances.');
            });
        }

        // Wait for the module to load and then create the Filter instance
        FilterWrapper.modulePromise.then(() => {
            this.filterInstance = new FilterWrapper.module.Filter(N, modelAntennaDelay);
            console.log(`Filter instance initialized with ${N} elements.`);
        });
    }

    sanityCheck() {
        if (!this.filterInstance) {
            throw new Error("Filter instance not initialized");
        }
    }

    isReady() {
        return this.filterInstance !== null;
    }

    get(index) {
        this.sanityCheck();
        const particle = this.filterInstance.get(index);
        return {
            x: particle.x,
            y: particle.y,
            z: particle.z,
            d: particle.d,
        };
    }

    set(index, x, y, z, d) {
        this.sanityCheck();
        const particle = new FilterWrapper.module.particle();
        particle.x = x;
        particle.y = y;
        particle.z = z;
        particle.d = d;
        this.filterInstance.set(index, particle);
    }

    get N() {
        this.sanityCheck();
        return this.filterInstance.getN();
    }

    set N(n) {
        this.sanityCheck();
        this.filterInstance.setN(n);
    }

    update(measure, P_NLOSS, estimatePos, estimateVar) {
        this.sanityCheck();
        const particleAvg = new FilterWrapper.module.particle();
        particleAvg.x = estimatePos.x;
        particleAvg.y = estimatePos.y;
        particleAvg.z = estimatePos.z;
        particleAvg.d = estimatePos.d;

        const particleVar = new FilterWrapper.module.particle();
        particleVar.x = estimateVar.x;
        particleVar.y = estimateVar.y;
        particleVar.z = estimateVar.z;
        particleVar.d = estimateVar.d;

        const startTime = performance.now();
        this.filterInstance.estimateState(measure, P_NLOSS, particleAvg, particleVar);
        const endTime = performance.now();
        console.log(`estimateState call took ${endTime - startTime} milliseconds.`);
    }

    getEstimatedPosition() {
        this.sanityCheck();
        const particle = this.filterInstance.getEstimateAvg();
        return {
            x: particle.x,
            y: particle.y,
            z: particle.z,
            d: particle.d,
        };
    }
    getEstimatedVariance() {
        this.sanityCheck();
        return this.filterInstance.getEstimateVar();
    }
}

export default FilterWrapper;
