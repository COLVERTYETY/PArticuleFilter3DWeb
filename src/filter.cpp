#include "filter.h"
#include <cmath>
#include <iostream>

#ifdef ARDUINO
  #include <ArduinoEigen.h>
#else
  #include <Eigen/Dense>
#endif



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

float Filter::getEstimateVarAvg() const {
    return (this->estimateVar.x + this->estimateVar.y + this->estimateVar.z) / 3.0f; 
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
    const float eta = 0.01f; // learning rate
    Eigen::Vector3f anchorPos(anchorAvg.x, anchorAvg.y, anchorAvg.z);
    // Define measurement noise covariance for ranging
    const float sigma_r2 = 0.045; // average error of 4.5cm
    // Define anchor variance as a diagonal covariance matrix
    Eigen::Matrix3f SigmaAnchor = Eigen::Matrix3f::Zero(); // Initialize to zero
    SigmaAnchor(0, 0) = anchorVar.x;
    SigmaAnchor(1, 1) = anchorVar.y;
    SigmaAnchor(2, 2) = anchorVar.z;

    // Update particle weights based on measurement
    for (int i = 0; i < particles.size(); i++) {
        float distance = dist(particles[i], anchorAvg);

        //  Step1: Compute error, including delay error component if modeled
        float ourDelayErrorComponent = modelAntennaDelay ? particles[i].d * measurement : 0.0f;
        float anchorDelayErrorComponent = modelAntennaDelay ? anchorAvg.d * measurement : 0.0f;
        float rerror = measurement - (distance + ourDelayErrorComponent + anchorDelayErrorComponent);
        float error = std::abs(rerror);

        // Represent the particle's position as an Eigen vector
        Eigen::Vector3f particlePos(particles[i].x, particles[i].y, particles[i].z);
        // Compute the difference vector between the particle and the anchor
        Eigen::Vector3f diff = particlePos - anchorPos;
    
        // Normalize the vector difference
        float norm = diff.norm();
        if (norm == 0.0f) norm = 1e-6f; // Avoid division by zero
        Eigen::Vector3f n = diff / norm;
           

        // Step 2: Compute ranging covariance matrix using the direction vector
        Eigen::Matrix3f SigmaRanging = sigma_r2 * (n * n.transpose());

        // Step 3: Combine anchor variance and ranging covariance matrices
        Eigen::Matrix3f Sigma = SigmaAnchor + SigmaRanging;

        // Step 4: Invert covariance matrix and compute determinant
        Eigen::Matrix3f SigmaInv = Sigma.inverse();
        float detSigma = Sigma.determinant();

        // Step 5: Compute the error vector in the direction of the difference vector
        Eigen::Vector3f e = rerror * n;

        const float regFactor = 1e-7f;
        // Step 6: Compute the likelihood of the particle given the measurement
        float likelihood = std::exp(-0.5f * e.transpose() * SigmaInv * e) / std::sqrt(std::pow(2 * M_PI, 3) * MAX(detSigma,regFactor));

        // Eigen::Vector3f gradient = -SigmaInv * e ;
        // // Update particle position
        // Eigen::Vector3f delta_p = eta * gradient;
        // particlePos += delta_p;
        // // Update particle in the list
        // particles[i].x = particlePos.x();
        // particles[i].y = particlePos.y();
        // particles[i].z = particlePos.z();

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
        float minVariance  = 1e-5f;
        if(i == oldi){
            // std::cout << "Repeated sampling " << repcounter << std::endl;
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

