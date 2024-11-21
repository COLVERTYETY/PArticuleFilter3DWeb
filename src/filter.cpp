#include "filter.h"
#include <cmath>
#include <iostream>


// utilities
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
    // particles.resize(N, {0, 0, 0, 0, 0});
    try {
        particles.resize(N, {0, 0, 0, 0, 0}); // Potentially problematic for large N
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
}

int Filter::getN() const {
    return this->N;
}

void Filter::setN(int N) {
    this->N = N;
    this->particles.resize(N, {0, 0, 0, 0, 0});
}

particle Filter::getEstimate() const {
    return this->estimate;
}

void Filter::initParticles(float measurement, particle anchor) {
    for (int i = 0; i < particles.size(); i++) {
        float ourAntennaDelay =1;
        float adjustedDistance = measurement;
        if (this->modelAntennaDelay){
            // Step 1: Remove the other anchor's antenna delay from the measurement
            float otherAntennaDelay = randomGaussian(anchor.d, std::sqrt(anchor.w)); // Estimated with Gaussian variance
            float correctedMeasurement = measurement - otherAntennaDelay * measurement;
            // Step 2: Estimate this particle's antenna delay
            ourAntennaDelay = randomGaussian(1, std::sqrt(anchor.w));
            // Step 3: Remove our antenna delay from the corrected measurement
            adjustedDistance = correctedMeasurement - ourAntennaDelay * measurement;
            // adjusted distanc shouldn't be negative
            if (adjustedDistance < 0) {
                adjustedDistance = measurement;
                ourAntennaDelay =1;
            }
        } else{
            adjustedDistance = measurement+ randomGaussian(0, std::sqrt(anchor.w));
        }
        // Step 4: Initialize the particle's position on a sphere with Gaussian noise
        float noisyDistance = adjustedDistance; // + randomGaussian(0, std::sqrt(anchor.w));

        // Generate random spherical coordinates
        float azimuthalAngle = static_cast<float>(rand()) / RAND_MAX * 2.0f * M_PI; // 0 to 2π
        float polarAngle = static_cast<float>(rand()) / RAND_MAX * M_PI; // 0 to π

        // Convert spherical coordinates to Cartesian
        float x = noisyDistance * std::sin(polarAngle) * std::cos(azimuthalAngle);
        float y = noisyDistance * std::sin(polarAngle) * std::sin(azimuthalAngle);
        float z = noisyDistance * std::cos(polarAngle);

        // Step 5: Assign the particle properties
        particles[i] = {x, y, z, ourAntennaDelay, 1.0f / N};
    }
    estimate.x = anchor.x;
    estimate.y = anchor.y;
    estimate.z = anchor.z;
    estimate.d = anchor.d;
    // Estimate variance
    float variance = 0;
    for (int i = 0; i < particles.size(); i++) {
        float distance = dist(particles[i], estimate);
        variance += (particles[i].w) * std::pow(distance, 2);
    }
    estimate.w = variance;
    isInitialized = true;
}




void Filter::estimateState(float measurement, particle anchor) {
    if (!isInitialized) {
        // Initialize particles if not already initialized
        initParticles(measurement, anchor);
        return;
    }

    float sum_w = 0;

    // Update particle weights based on measurement
    for (int i = 0; i < particles.size(); i++) {
        // Calculate distance between particle and anchor in 3D
        float distance = dist(particles[i], anchor);

        // Calculate the error, accounting for antenna delay
        float delayErrorComponent = modelAntennaDelay ? particles[i].d * measurement : 0;
        float error = std::abs(measurement - (distance + delayErrorComponent));

        // Compute likelihood (Gaussian probability)
        float likelihood = std::exp(-error * error / (2 * anchor.w * anchor.w));

        // Update particle weight
        particles[i].w = likelihood;
        sum_w += likelihood;
    }

    // Check if weights are too low; reinitialize if necessary
    if (sum_w <= 0.05/N) {
        initParticles(measurement, anchor);
        return;
    }

    // Normalize weights
    for (int i = 0; i < particles.size(); i++) {
        particles[i].w /= sum_w;
    }

    // Estimate position (weighted average)
    particle estimated = {0, 0, 0, 0, 0};
    for (int i = 0; i < particles.size(); i++) {
        estimated.x += particles[i].x * particles[i].w;
        estimated.y += particles[i].y * particles[i].w;
        estimated.z += particles[i].z * particles[i].w;
        estimated.d += particles[i].d * particles[i].w;
    }
    this->estimate = estimated;

    // Estimate variance
    float variance = 0;
    for (int i = 0; i < particles.size(); i++) {
        float distance = dist(particles[i], estimated);
        variance += (particles[i].w) * std::pow(distance, 2);
    }
    this->estimate.w = variance;
    // Resample particles using low-variance resampling
    std::vector<particle> newParticles;
    float wTarget = sum_w / particles.size();
    float r = static_cast<float>(rand()) / RAND_MAX * wTarget;
    float c = particles[0].w;
    int i = 0;
    for (int m = 0; m < particles.size(); m++) {
        float U = r + m * wTarget;
        while (U > c) {
            i++;
            i = i % particles.size();
            c += particles[i].w;
        }
        // Clone particle and add Gaussian noise for resampling
        particle newParticle = particles[i];
        newParticle.x += randomGaussian(0, 0.4* std::sqrt(variance));
        newParticle.y += randomGaussian(0, 0.4* std::sqrt(variance));
        newParticle.z += randomGaussian(0, 0.4* std::sqrt(variance));
        newParticles.push_back(newParticle);
    }
    particles = newParticles; // Replace particles with resampled particles
}
