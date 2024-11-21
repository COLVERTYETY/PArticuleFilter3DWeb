#ifndef FILTER_H
#define FILTER_H

#include <vector>
#include <stdexcept>

// Define the particle particle structure
struct particle {
    float x, y, z, d, w;
} typedef particle;

// Define the Filter class
class Filter {
private:
    std::vector<particle> particles;
    int N;
    float w_sum;
    particle estimate;
    bool isInitialized = false;
    bool modelAntennaDelay = true;
    void initParticles(float measurement, particle anchor);

public:
    // Constructor to initialize the vector with N elements
    Filter(int N, bool modelAntennaDelay = true);

    // Retrieve an element by index
    particle get(int i) const;
    void set(int i, particle d);
    int getN() const;
    void setN(int N);
    particle getEstimate() const;
    void estimateState(float measurement, particle anchor);

};

// utility functions
float dist(particle p1, particle p2);
float randomGaussian(float mean, float stddev);

#endif // FILTER_H
