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
    EXPECT_GT(estimate2.w, estimate3.w);
    int N = 2;
    for (int i=0; i<2;i++){
        filter.estimateState(dist(truePos, anchor0), anchor0);
        filter.estimateState(dist(truePos, anchor1), anchor1);
        filter.estimateState(dist(truePos, anchor2), anchor2);
        filter.estimateState(dist(truePos, anchor3), anchor3);
    }
    estimate = filter.getEstimate();
    std::cout << "Estimated disatnce after "<< N <<" updates: " << dist(estimate, truePos) <<" with variance: "<< estimate.w <<std::endl;
 
    //  check that the estimated position is close to the true position
    EXPECT_NEAR(estimate.x, 0.0, 1.0);
    EXPECT_NEAR(estimate.y, 0.0, 1.0);
    EXPECT_NEAR(estimate.z, 0.0, 1.0);
    EXPECT_NEAR(estimate.d, 0.0, 1.0);

}

TEST(FilterTest, EstimateGeometry) {
    std::ofstream memLog("Geometry convergence.csv");
    memLog << "id, x,y,z,d,w,distance\n";
    Filter node0(1000, false);
    Filter node1(1000, false);
    Filter node2(1000, false);
    Filter node3(1000, false);
    particle node0Position{ -3, 0.5, 5, 0.0, 0.01 };
    particle node1Position{ -6, 0.5, 12, 0.0, 0.01 };
    particle node2Position{ -9, 10, 16, 0.0, 0.01 };
    particle node3Position{ -4, 0.5, 20, 0.0, 0.01 };
    particle anchor0{ 2, 0.5, -8, 0.0, 0.01 };
    particle anchor1{ 12, 0.5, -8, 0.0, 0.01 };
    particle anchor2{ 12, 14, -8, 0.0, 0.01 };
    particle anchor3{ 12, 0.5, 2, 0.0, 0.01 };
    auto anchors = {anchor0, anchor1, anchor2, anchor3};
    std::vector<Filter> nodes = {node0, node1, node2, node3};
    std::vector<particle> nodePositions = {node0Position, node1Position, node2Position, node3Position};
    // i rounds
    int N= 3;
    for(int i=0; i<N; i++){
        // std::cout << "Round "<< i << std::endl;
        //  for each node
        for(int j=0; j<4; j++){
            // std::cout << "Node "<< j << std::endl;
            // update according to the anchors
            for(auto anchor: anchors){
                nodes[j].estimateState(dist(nodePositions[j], anchor),  anchor);
            }
        }
        // std::cout << "Done anchors" << std::endl;
        // for each node, update according to mesh 
        for(int j=0; j<4; j++){
            // update according to the anchors
            std::cout << "Node "<< j << std::endl;
            for(int k=0; k<4; k++){
                if (k==j){
                    continue;
                }
                nodes[j].estimateState(dist(nodePositions[j], nodePositions[k]),  nodes[k].getEstimate());
                particle estimate = nodes[j].getEstimate();
                memLog << j << "," << estimate.x << "," << estimate.y << "," << estimate.z << "," << estimate.d << "," << estimate.w << "," << dist(estimate, nodePositions[j]) << "\n";
            }
        }
        // std::cout << "Done Mesh" << std::endl;

    }
    //  print final error
    for(int j=0; j<4; j++){
        particle estimate = nodes[j].getEstimate();
        std::cout << "Node "<< j << " Estimated position after "<<N<<" updates: (" << estimate.x << ", " << estimate.y << ", " << estimate.z << ", " << estimate.d << ", "<< estimate.w<<")" << std::endl;
        std::cout << "Node "<< j << " Estimated distance after "<<N<<" updates: " << dist(estimate, nodePositions[j]) <<" with variance: "<< estimate.w <<std::endl;
        //  check that the estimated position is close to the true position
        EXPECT_NEAR(estimate.x, nodePositions[j].x, 1.5);
        EXPECT_NEAR(estimate.y, nodePositions[j].y, 1.5);
        EXPECT_NEAR(estimate.z, nodePositions[j].z, 1.5);
        EXPECT_NEAR(estimate.d, nodePositions[j].d, 1.5);
    }
    memLog.close();


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
