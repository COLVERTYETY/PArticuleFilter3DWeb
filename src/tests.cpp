#include <gtest/gtest.h>
#include <fstream>
#include <chrono>
#include <sys/resource.h> // For memory usage
#include "filter.h"

// Helper function to get current memory usage in kilobytes
size_t getMemoryUsage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss; // Memory usage in KB
}

// Original tests
TEST(FilterTest, Constructor) {
    Filter filter(5);
    EXPECT_EQ(filter.getN(), 5);
}

TEST(FilterTest, GetSet) {
    Filter filter(1);
    particle d = {1.0, 2.0, 3.0, 4.0, 0.5};
    filter.set(0, d);
    particle retrieved = filter.get(0);
    EXPECT_FLOAT_EQ(retrieved.x, 1.0);
    EXPECT_FLOAT_EQ(retrieved.y, 2.0);
    EXPECT_FLOAT_EQ(retrieved.z, 3.0);
    EXPECT_FLOAT_EQ(retrieved.d, 4.0);
    EXPECT_FLOAT_EQ(retrieved.w, 0.5);
}

//  add a test to plot the init particles
TEST(FilterTest, InitPArticles){
    Filter filter(100, false);
    particle anchor = {0.0, 0.0, 0.0, 0.1, 0.001};
    filter.estimateState(10.0, anchor);
    std::ofstream runtimeLog("initParticles.csv");
    runtimeLog << "x,y,z,d,w\n";
    for (int i = 0; i < 100; i++) {
        particle retrieved = filter.get(i);
        runtimeLog << retrieved.x << "," << retrieved.y << "," << retrieved.z << "," << retrieved.d << "," << retrieved.w << "\n";
    }
    filter.estimateState(10.0, anchor);
    runtimeLog.close();
}

TEST(FilterTest, EstimateState) {
    Filter filter(1000, false);
    particle anchor0 = {0.0, 10.0, 0.0, 0.0, 0.01};
    particle anchor1 = {10.0, 0.0, 0.0, 0.0, 0.01};
    particle anchor2 = {0.0, 0.0, 10.0, 0.0, 0.01};
    particle anchor3 = {10.0, 10.0, 10.0, 0.0, 1.1};
    particle truePos = {0.0, 0.0, 0.0, 1.0, 0.01};
    filter.estimateState(10.0, anchor0);
    particle estimate1 = filter.getEstimate();
    filter.estimateState(10.0, anchor1);
    particle estimate2 = filter.getEstimate();
    filter.estimateState(10.0, anchor2);
    particle estimate3 = filter.getEstimate();
    filter.estimateState(17.3205, anchor3);
    particle estimate = filter.getEstimate();
    std::cout << "Estimated position after 1 update: (" << estimate.x << ", " << estimate.y << ", " << estimate.z << ", " << estimate.d << ", "<< estimate.w<<")" << std::endl;
    //  check that w gets lower as estimates are performed
    // EXPECT_GT(estimate1.w, estimate2.w);
    // EXPECT_GT(estimate2.w, estimate3.w);
    for (int i=0; i<100;i++){
        filter.estimateState(10.0, anchor0);
        filter.estimateState(10.0, anchor1);
        filter.estimateState(10.0, anchor2);
        filter.estimateState(17.3205, anchor3);
    }
    estimate = filter.getEstimate();
    std::cout << "Estimated disatnce after "<< 100 <<" updates: " << dist(estimate, truePos) <<" with variance: "<< estimate.w <<std::endl;
 
    //  check that the estimated position is close to the true position
    EXPECT_NEAR(estimate.x, 0.0, 2.0);
    EXPECT_NEAR(estimate.y, 0.0, 2.0);
    EXPECT_NEAR(estimate.z, 0.0, 2.0);
    EXPECT_NEAR(estimate.d, 0.0, 2.0);

}

// Log memory usage for different N values
TEST(FilterPerformanceTest, MemoryUsage) {
    std::ofstream memLog("memory_usage.csv");
    memLog << "N,MemoryUsage(KB)\n";

    particle reference = {0.0, 0.0, 0.0, 0.1, 0.01};
    for (int N : {10, 100, 200, 600, 1000, 2000, 5000, 10000}) {
        size_t beforeMem = getMemoryUsage();
        Filter filter(N);
        filter.estimateState(1.0, reference);
        size_t afterMem = getMemoryUsage();
        size_t memUsage = afterMem - beforeMem;
        memLog << N << "," << memUsage << "\n";
    }

    memLog.close();
}

TEST(FilterPerformanceTest, Runtime) {
    std::ofstream runtimeLog("runtime.csv");
    runtimeLog << "N,Runtime(ms)\n";

    for (int N : {10, 100, 200, 600, 1000, 2000, 5000, 10000}) {
        std::cout << "Testing with N = " << N << std::endl;
        Filter filter(N);
        particle anchor = {0.0, 0.0, 0.0, 0.1, 0.01};

        auto start = std::chrono::high_resolution_clock::now();
        try {
            filter.estimateState(1.0, anchor);
        } catch (const std::exception &e) {
            std::cerr << "Error during estimateState with N = " << N << ": " << e.what() << std::endl;
            continue;
        }
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> runtime = end - start;
        runtimeLog << N << "," << runtime.count() << "\n";
    }

    runtimeLog.close();
}



int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
