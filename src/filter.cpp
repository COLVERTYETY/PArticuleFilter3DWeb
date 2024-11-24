#include "filter.h"
#include <cmath>
#include <iostream>


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
    // this->isInitialized = false;
}

int Filter::getN() const {
    return this->N;
}

void Filter::setN(int N) {
    this->N = N;
    this->particles.resize(N, {0, 0, 0, 0, 0});
    this->isInitialized = false;
}

particle Filter::getEstimate() const {
    return this->estimate;
}

void Filter::initParticles(float measurement, particle anchor) {
    for (int i = 0; i < particles.size(); i++) {
        float ourAntennaDelay =0.1;
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
        float x = anchor.x + noisyDistance * std::sin(polarAngle) * std::cos(azimuthalAngle);
        float y = anchor.y + noisyDistance * std::sin(polarAngle) * std::sin(azimuthalAngle);
        float z = anchor.z + noisyDistance * std::cos(polarAngle);

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
        float rerror = measurement - (distance + delayErrorComponent);

        float regularized_factor = anchor.w * anchor.w + 0.01;
        // Compute likelihood (Gaussian probability)
        float likelihood = std::exp(-error * error / (2 * regularized_factor));

        // there is room to improve
        if (std::pow(rerror,2) -2*std::pow(anchor.w,2)*std::log(1.01) > 0){
            // Move the particle to improve likelihood by 1%

            // Calculate the unit vector from the particle to the anchor
            float dx = anchor.x - particles[i].x;
            float dy = anchor.y - particles[i].y;
            float dz = anchor.z - particles[i].z;
            float norm = std::sqrt(dx * dx + dy * dy + dz * dz);

            // Normalize the direction vector
            if (norm > 0) {
                dx /= norm;
                dy /= norm;
                dz /= norm;
            }

            // Calculate the adjustment magnitude
            float adjustment = std::sqrt(std::pow(rerror, 2) - 2 * std::pow(anchor.w, 2) * std::log(1.01));

            // Update the particle's position
            particles[i].x += dx * adjustment;
            particles[i].y += dy * adjustment;
            particles[i].z += dz * adjustment;

        }

        // Update particle weight
        particles[i].w = likelihood;
        sum_w += likelihood;
    }

    // Check if weights are too low; reinitialize if necessary  estimate.w / (10 * N)
    if (sum_w <= 0.005/particles.size()) {
        initParticles(measurement, anchor);
        return;
    }

    // Normalize weights
    float avgw = 0;
    for (int i = 0; i < particles.size(); i++) {
        particles[i].w /= sum_w;
        avgw += particles[i].w;
    }
    avgw /= particles.size();

    // Estimate position (weighted average)
    particle estimated = {0, 0, 0, 0, 0};
    for (int i = 0; i < particles.size(); i++) {
        estimated.x += particles[i].x * particles[i].w;
        estimated.y += particles[i].y * particles[i].w;
        estimated.z += particles[i].z * particles[i].w;
        estimated.d += particles[i].d * particles[i].w;
    }
    
    // Estimate variance
    float variance = 0;
    for (int i = 0; i < particles.size(); i++) {
        float distance = dist(particles[i], estimated);
        variance += (particles[i].w) * std::pow(distance, 2);
    }
    // float variance = 0;
    // float unweighted_variance = 0;

    // for (int i = 0; i < particles.size(); i++) {
    //     float distance = dist(particles[i], estimated);
    //     variance += particles[i].w * std::pow(distance, 2);
    //     unweighted_variance += std::pow(distance, 2);
    // }

    // unweighted_variance /= particles.size(); // Normalize by number of particles

    // std::cout << "Weighted variance: " << variance << ", Unweighted variance: " << unweighted_variance<< " anchor w was: "<<anchor.w << std::endl;
    estimated.w = variance;
    this->estimate = estimated;

    // Resample particles using low-variance resampling
    std::vector<particle> newParticles;
    float wTarget = 1.0 / particles.size();
    float r = static_cast<float>(rand()) / RAND_MAX * wTarget;
    float c = particles[0].w;
    int i = 0;
    int counter = 0;
    for (int m = 0; m < particles.size(); m++) {
        float U = r + m * wTarget;
        while (U > c) {
            i++;
            i = i % particles.size();
            c += particles[i].w;
            // if (i%10 == 0){
            //     std::cout << "U c " << U<<" "<<c << std::endl;
            // }
            counter++;
            if(counter%(1000*N) ==0){
                std::cout << "resampling counter is too high " << counter << std::endl;
                std::cerr << "Resampling is taking too long, aborting" << std::endl;
                throw std::runtime_error("Resampling is taking too long, possible infinite loop");
            }
        }
        // Clone particle and add Gaussian noise for resampling
        float noise_scale = std::max(0.1f, std::sqrt(variance) / 1000.0f); // Decrease noise as variance drops
        particle newParticle = particles[i];
        newParticle.x += randomGaussian(0, noise_scale);
        newParticle.y += randomGaussian(0, noise_scale);
        newParticle.z += randomGaussian(0, noise_scale);
        newParticles.push_back(newParticle);
    }
    particles = newParticles; // Replace particles with resampled particles
}
