#include "filter.h"
#include <cmath>
#include <iostream>


#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

float randomExponential(float lambda) {
    float u = static_cast<float>(rand()) / RAND_MAX; // Uniform random value [0, 1)
    return -std::log(1.0f - u) / lambda;
}

float randomGaussian(float mean, float stddev) {
    static bool hasSpare = false;
    static float spare;

    if (hasSpare) {
        hasSpare = false;
        return mean + stddev * spare;
    }

    hasSpare = true;
    float u, v, s;
    do {
        u = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
        v = static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);

    s = std::sqrt(-2.0f * std::log(s) / s);
    spare = v * s;
    return mean + stddev * u * s;
}

float dist(particle p1, particle p2) {
    return std::sqrt(
        std::pow(p1.x - p2.x, 2) +
        std::pow(p1.y - p2.y, 2) +
        std::pow(p1.z - p2.z, 2)
    );
}



// Constructor
Filter::Filter(int N, bool modelAntennaDelay) {
    this->N = N;
    this->modelAntennaDelay = modelAntennaDelay;
    try {
        particles.resize(N, {0, 0, 0, 0}); // Potentially problematic for large N
    } catch (const std::bad_alloc &e) {
        std::cerr << "Memory allocation failed for N = " << N << ": " << e.what() << std::endl;
        throw;
    }
}


particle Filter::get(int i) const {
    if (i < 0 || i >= particles.size()) {
        throw std::out_of_range("Index out of range");
    }
    return particles[i];
}


void Filter::set(int i, particle d) {
    if (i < 0 || i >= particles.size()) {
        throw std::out_of_range("Index out of range");
    }
    particles[i] = d;
    // this->isInitialized = false;
}

int Filter::getN() const {
    return this->N;
}

void Filter::setN(int N) {
    this->N = N;
    this->particles.resize(N, {0, 0, 0, 0});
    this->isInitialized = false;
}

particle Filter::getEstimateAvg() const {
    return this->estimateAvg;
}

particle Filter::getEstimateVar() const {
    return this->estimateVar;
}




void Filter::initParticles(float measurement, float P_NLoss, particle anchorAvg, particle anchorVar) {
    if (particles.empty()) {
        throw std::runtime_error("Particles vector is empty. Initialize the particles before calling initParticles.");
    }

    for (int i = 0; i < particles.size(); ++i) {
        float ourAntennaDelay = 0.0f;
        float adjustedDistance = measurement;

        // Step 1: Model antenna delay using exponential distribution
        if (this->modelAntennaDelay) {
            const float lambda = 10.0f; // 10 gives ~0.1 as the most likely value -> exponential distribution only used for initialization

            // Generate our antenna delay in the range [0, 1)
            do {
                ourAntennaDelay = randomExponential(lambda);
            } while (ourAntennaDelay >= 1.0f);

            // Adjust measurement for antenna delay
            float otherAntennaDelay = randomGaussian(anchorAvg.d, std::sqrt(anchorVar.d));
            float correctedMeasurement = measurement - otherAntennaDelay * measurement;

            adjustedDistance = correctedMeasurement - ourAntennaDelay * measurement;

            // Ensure non-negative adjusted distance
            if (adjustedDistance < 0) {
                adjustedDistance = measurement;
            }
        } else {
            // Add Gaussian noise to the measurement directly -> lets move this to the coordinates
            adjustedDistance = measurement;// + randomGaussian(0, std::sqrt(anchorVar.d));
        }

        // Step 2: Handle P_NLoss
        // if (static_cast<float>(rand()) / RAND_MAX < P_NLoss) {
        //     // Add constant error or Gaussian noise to simulate loss
        //     adjustedDistance += randomGaussian(5.0f, 1.0f); // how should we choose these values?
        // }

        // Step 3: Initialize the particle's position using spherical coordinates
        float azimuthalAngle = static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI; // 0 to 2π
        float polarAngle = static_cast<float>(rand()) / RAND_MAX * M_PI;           // 0 to π

        float sinPolar = std::sin(polarAngle);
        float cosPolar = std::cos(polarAngle);
        float sinAzimuthal = std::sin(azimuthalAngle);
        float cosAzimuthal = std::cos(azimuthalAngle);

        float x = anchorAvg.x + randomGaussian(0, std::sqrt(anchorVar.x)) + adjustedDistance * sinPolar * cosAzimuthal;
        float y = anchorAvg.y + randomGaussian(0, std::sqrt(anchorVar.y)) + adjustedDistance * sinPolar * sinAzimuthal;
        float z = anchorAvg.z + randomGaussian(0, std::sqrt(anchorVar.z)) + adjustedDistance * cosPolar;


        // Step 4: Assign properties to the particle
        particles[i] = {x, y, z, ourAntennaDelay};
    }

    // Step 5: Compute estimate average and variance
    updateEstimates();

    isInitialized = true;
}


void Filter::updateEstimates() {
    // this update strategy uses frequency rather than weights
    float x = 0.0f, y = 0.0f, z = 0.0f, d = 0.0f;
    float varx = 0.0f, vary = 0.0f, varz = 0.0f, vard = 0.0f;

    // Update the estimate average
    for (const auto& p : particles) {
        x += p.x;
        y += p.y;
        z += p.z;
        d += p.d;
    }
    estimateAvg = {x / particles.size(), y / particles.size(), z / particles.size(), d / particles.size()};

    // Update the estimate variance
    for (const auto& p : particles) {
        varx += std::pow(p.x - estimateAvg.x, 2);
        vary += std::pow(p.y - estimateAvg.y, 2);
        varz += std::pow(p.z - estimateAvg.z, 2);
        vard += std::pow(p.d - estimateAvg.d, 2);
    }
    estimateVar = {varx / particles.size(), vary / particles.size(), varz / particles.size(), vard / particles.size()};
}

void Filter::estimateState(float measurement, float P_NLoss, particle anchorAvg, particle anchorVar) {
    if (!isInitialized) {
        // Initialize particles if not already initialized
        initParticles(measurement, P_NLoss, anchorAvg, anchorVar);
        return;
    }

    std::vector<float> weights(particles.size(), 0.0f);
    float sum_w = 0.0f;

    // Update particle weights based on measurement
    for (int i = 0; i < particles.size(); i++) {
        float distance = dist(particles[i], anchorAvg);

        // Compute error, including delay error component if modeled
        float ourDelayErrorComponent = modelAntennaDelay ? particles[i].d * measurement : 0.0f;
        float anchorDelayErrorComponent = modelAntennaDelay ? anchorAvg.d * measurement : 0.0f;
        float rerror = measurement - (distance + ourDelayErrorComponent + anchorDelayErrorComponent);
        float error = std::abs(rerror);

        float dx = particles[i].x - anchorAvg.x;
        float dy = particles[i].y - anchorAvg.y;
        float dz = particles[i].z - anchorAvg.z;

        // Compute the total 3D distance
        float norm = std::sqrt(dx * dx + dy * dy + dz * dz);
        // Avoid division by zero for zero distance
        if (norm == 0.0f) norm = 1e-6f;
        // Normalize the vector difference
        float nx = dx / norm;
        float ny = dy / norm;
        float nz = dz / norm;

        float e_x = nx * rerror;
        float e_y = ny * rerror;
        float e_z = nz * rerror;
        const float regFactor = 1e-6f;
        // Compute the likelihood using per-axis variances
        float likelihood = std::exp(
            -0.5f * (
                (e_x * e_x) / (anchorVar.x + regFactor) +
                (e_y * e_y) / (anchorVar.y + regFactor) +
                (e_z * e_z) / (anchorVar.z + regFactor)
            )
        );

        // Update particle weight
        weights[i] = likelihood;
        sum_w += likelihood;
    }

    // std::cout << "Sum of weights: " << sum_w << std::endl;
    // Reinitialize particles if weights are too low
    if (sum_w <= 0.005f / particles.size()) {
        // std::cout << "Resampling due to low weights" << std::endl;
        initParticles(measurement, P_NLoss, anchorAvg, anchorVar);
        return;
    }

    // Normalize weights
    for (int i=0;i<weights.size();i++){
        weights[i] /= sum_w;
    }

    // Resample particles using low-variance resampling
    std::vector<particle> newParticles;
    float wTarget = 1.0f / particles.size();
    float r = static_cast<float>(rand()) / RAND_MAX * wTarget;
    float c = weights[0];
    int i = 0;
    int oldi = 0;
    int repcounter = 0;
    for (int m = 0; m < particles.size(); m++) {
        float U = r + m * wTarget;
        while (U > c) {
            i = (i + 1) % particles.size();
            c += weights[i];
        }
        float minVariance  = 1e-1f;
        if(i == oldi){
            minVariance *=repcounter*1.05;
            repcounter++;
        } else{
            repcounter = 1;
        }
        oldi = i;
        // Resample particle with added Gaussian noise
        particle newParticle = particles[i];
        newParticle.x += randomGaussian(0, MAX(minVariance, repcounter*::sqrt(anchorVar.x)/10));
        newParticle.y += randomGaussian(0, MAX(minVariance, repcounter*::sqrt(anchorVar.y)/10));
        newParticle.z += randomGaussian(0, MAX(minVariance, repcounter*::sqrt(anchorVar.z)/10));
        newParticle.d = MAX(0,newParticle.d+randomGaussian(0, MAX(1e-7f, std::sqrt(anchorVar.d) / 10.0)));
        newParticles.push_back(newParticle);
    }
    particles = newParticles;
    updateEstimates();
}

