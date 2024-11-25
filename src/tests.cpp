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
    particle d = {1.0, 2.0, 3.0, 4.0};
    filter.set(0, d);
    particle retrieved = filter.get(0);
    EXPECT_FLOAT_EQ(retrieved.x, 1.0);
    EXPECT_FLOAT_EQ(retrieved.y, 2.0);
    EXPECT_FLOAT_EQ(retrieved.z, 3.0);
    EXPECT_FLOAT_EQ(retrieved.d, 4.0);
}

//  add a test to plot the init particles
TEST(FilterTest, InitPArticles){
    Filter filter(100, false);
    particle anchor = {0.0, 0.0, 0.0, 0.0};
    particle anchorVar = {0.001, 0.001, 0.001, 0.001};
    filter.estimateState(10.0, 0, anchor, anchorVar);
    std::ofstream runtimeLog("initParticles.csv");
    runtimeLog << "x,y,z,d\n";
    for (int i = 0; i < 100; i++) {
        particle retrieved = filter.get(i);
        runtimeLog << retrieved.x << "," << retrieved.y << "," << retrieved.z << "," << retrieved.d<< "\n";
    }
    filter.estimateState(10.0, 0, anchor, anchorVar);
    runtimeLog.close();
}

TEST(FilterTest, EstimateState) {
    Filter filter(1000, false);
    particle anchorVars = {0.1, 0.1, 0.1, 0.1};
    particle anchor0 = {0.0, 10.0, 0.0, 0.0};
    particle anchor1 = {10.0, 0.0, 0.0, 0.0};
    particle anchor2 = {0.0, 0.0, 10.0, 0.0};
    particle anchor3 = {10.0, 10.0, 10.0, 0.0};
    particle truePos = {0.0, 0.0, 0.0, 0.0};
    filter.estimateState(dist(truePos, anchor0),0, anchor0, anchorVars);
    particle estimateavg1 = filter.getEstimateAvg();
    particle estimatevar1 = filter.getEstimateVar();
    // std::cout << "Estimated position after 1 update: (" << estimateavg1.x << ", " << estimateavg1.y << ", " << estimateavg1.z << ", " << estimateavg1.d << ")" << std::endl;
    // std::cout << "Estimated variance after 1 update: (" << estimatevar1.x << ", " << estimatevar1.y << ", " << estimatevar1.z << ", " << estimatevar1.d << ")" << std::endl;
    filter.estimateState(dist(truePos, anchor1),0, anchor1, anchorVars);
    particle estimateavg2 = filter.getEstimateAvg();
    particle estimatevar2 = filter.getEstimateVar();
    // std::cout << "Estimated position after 2 update: (" << estimateavg2.x << ", " << estimateavg2.y << ", " << estimateavg2.z << ", " << estimateavg2.d << ")" << std::endl;
    // std::cout << "Estimated variance after 2 update: (" << estimatevar2.x << ", " << estimatevar2.y << ", " << estimatevar2.z << ", " << estimatevar2.d << ")" << std::endl;
    filter.estimateState(dist(truePos, anchor2),0, anchor2, anchorVars);
    particle estimateavg3 = filter.getEstimateAvg();
    particle estimatevar3 = filter.getEstimateVar();
    // std::cout << "Estimated position after 3 update: (" << estimateavg3.x << ", " << estimateavg3.y << ", " << estimateavg3.z << ", " << estimateavg3.d << ")" << std::endl;
    // std::cout << "Estimated variance after 3 update: (" << estimatevar3.x << ", " << estimatevar3.y << ", " << estimatevar3.z << ", " << estimatevar3.d << ")" << std::endl;
    filter.estimateState(dist(truePos, anchor3),0, anchor3, anchorVars);
    particle estimateavg4 = filter.getEstimateAvg();
    particle estimatevar4 = filter.getEstimateVar();
    // std::cout << "Estimated position after 4 update: (" << estimateavg4.x << ", " << estimateavg4.y << ", " << estimateavg4.z << ", " << estimateavg4.d << ")" << std::endl;
    // std::cout << "Estimated variance after 4 update: (" << estimatevar4.x << ", " << estimatevar4.y << ", " << estimatevar4.z << ", " << estimatevar4.d << ")" << std::endl;
    //  check that w gets lower as estimates are performed
    EXPECT_GT(estimatevar1.x, estimatevar2.x);
    EXPECT_GT(estimatevar1.y, estimatevar2.y);
    EXPECT_GT(estimatevar1.z, estimatevar2.z);
    int N = 2;
    for (int i=0; i<N;i++){
        filter.estimateState(dist(truePos, anchor0), 0, anchor0, anchorVars);
        filter.estimateState(dist(truePos, anchor1), 0, anchor1, anchorVars);
        filter.estimateState(dist(truePos, anchor2), 0, anchor2, anchorVars);
        filter.estimateState(dist(truePos, anchor3), 0, anchor3, anchorVars);
    }
    estimateavg4 = filter.getEstimateAvg();
    estimatevar4 = filter.getEstimateVar();
    std::cout << "Estimated distance after "<< N+4 <<" updates: " << dist(estimateavg4, truePos) << std::endl;
 
    //  check that the estimated position is close to the true position
    EXPECT_NEAR(dist(estimateavg4, truePos), 0.0, 1.0);

}

// TEST(FilterTest, RandomizedConvergence){
//     particle anchor0{ 2, 0.5, -8, 0.0, 0.01 };
//     particle anchor1{ 12, 0.5, -8, 0.0, 0.01 };
//     particle anchor2{ 12, 14, -8, 0.0, 0.01 };
//     particle anchor3{ 12, 0.5, 2, 0.0, 0.01 };
//     std::vector<particle> anchors = {anchor0, anchor1, anchor2, anchor3};
//     for(int i=0; i<50;i++){
//         Filter test(1000, false);
//         particle randPos{static_cast<float>(static_cast<float>(rand()) / RAND_MAX * 50.0f - 25.0f), static_cast<float>(static_cast<float>(rand()) / RAND_MAX * 50.0f - 25.0f), static_cast<float>(static_cast<float>(rand()) / RAND_MAX * 50.0f - 25.0f), 0.0f, 0.01f};
//         int estimationRounds = 20;
//         for(int j = 0; j<estimationRounds;j++){
//             for(auto anchor: anchors){
//                 test.estimateState(dist(randPos, anchor), anchor);
//             }
//         }
//         particle estimate = test.getEstimate();
//         // std::cout << "Estimated position after "<<estimationRounds<<" updates: (" << estimate.x << ", " << estimate.y << ", " << estimate.z << ", " << estimate.d << ", "<< estimate.w<<")" << std::endl;
//         // std::cout << "Estimated distance after "<<estimationRounds<<" updates: " << dist(estimate, randPos) <<" with variance: "<< estimate.w <<std::endl;
//         //  check that the estimated position is close to the true position
//         EXPECT_NEAR(estimate.x, randPos.x, 1.5);
//         EXPECT_NEAR(estimate.y, randPos.y, 1.5);
//         EXPECT_NEAR(estimate.z, randPos.z, 1.5);
//     }
// }

TEST(FilterTest, ConvergenceAnchors){
    // in this test we study the convergence of the node according to N Particules
    std::ofstream memLog("Particule_convergence.csv");
    memLog << "N,x,y,z,d,varx,vary,varz,vard,distance\n";
    particle nodePosition{ -10, 0.5, 10, 0.0};
    particle anchor0{ 2, 0.5, -8, 0.0};
    particle anchor1{ 12, 0.5, -8, 0.0};
    particle anchor2{ 12, 14, -8, 0.0};
    particle anchor3{ 12, 0.5, 2, 0.0};
    particle anchorVars{ 0.1, 0.1, 0.1, 0.1};
    std::vector<particle> anchors = {anchor0, anchor1, anchor2, anchor3};
    int N =100;
    int estimationRounds = 5;
    for(int i=1;i<N;i++){
        Filter test(i*100, false);
        for (size_t est = 0; est < estimationRounds; est++)
        {
            for(int j=0; j<4; j++){
                test.estimateState(dist(nodePosition, anchors[j]), 0, anchors[j], anchorVars);
            }
            particle estimate = test.getEstimateAvg();
            particle estimateVar = test.getEstimateVar();
            memLog << i*100 << "," << estimate.x << "," << estimate.y << "," << estimate.z << "," << estimate.d << "," << estimateVar.x << "," << estimateVar.y << "," << estimateVar.z << "," << estimateVar.d << "," << dist(estimate, nodePosition) << "\n";
            // if(dist(test.getEstimateAvg(), nodePosition)<0.5){
            //     break;
            // }
        }
        
    }
}

TEST(FilterTest, EstimateGeometry) {
    // in this test we perform one round of updates from teh anchors then only from the mesh 
    std::ofstream memLog("Geometry_convergence.csv");
    memLog << "id,x,y,z,d,varx,vary,varz,vard,distance\n";
    Filter node0(10000, false);
    Filter node1(10000, false);
    Filter node2(10000, false);
    Filter node3(10000, false);
    particle node0Position{ -5, -3, 2, 0.0};
    particle node1Position{ -10, 1.0, 12, 0.0};
    particle node2Position{ -14, 10, 18, 0.0};
    particle node3Position{ -20, 0.5, 32, 0.0};
    particle anchor0{ 2, 0.5, -8, 0.0};
    particle anchor1{ 12, 0.5, -8, 0.0};
    particle anchor2{ 12, 14, -8, 0.0};
    particle anchor3{ 12, 0.5, 2, 0.0};
    particle anchorVars{ 0.1, 0.1, 0.1, 0.1};
    std::vector<Filter> nodes = {node0, node1, node2, node3};
    std::vector<particle> nodePositions = {node0Position, node1Position, node2Position, node3Position};
    std::vector<particle> anchors = {anchor0, anchor1, anchor2, anchor3};
    // perform one round of updates from the anchors
    for(int i=0; i<4; i++){
        // memLog << i << "," << nodePositions[i].x << "," << nodePositions[i].y << "," << nodePositions[i].z << "," << nodePositions[i].d << "," << 0 << "," << 0 << "," << 0 << "," << 0 << "," << 0 << "\n";
        for( int j=0; j<4; j++){
            nodes[i].estimateState(dist(nodePositions[i], anchors[j]), 0, anchors[j], anchorVars);
        }
        particle estimate = nodes[i].getEstimateAvg();
        particle estimateVar = nodes[i].getEstimateVar();
        memLog << i << "," << estimate.x << "," << estimate.y << "," << estimate.z << "," << estimate.d << "," << estimateVar.x << "," << estimateVar.y << "," << estimateVar.z << "," << estimateVar.d << "," << dist(estimate, nodePositions[i]) << "\n";
    }
    int N =10;
    // perform N rounds of updates from mesh
    for(int i=0; i<N; i++){
        for(int j=0; j<4; j++){
            for( int k=0; k<4; k++){
                if(j!=k){
                    nodes[j].estimateState(dist(nodePositions[j], nodePositions[k]), 0, nodes[k].getEstimateAvg(), nodes[k].getEstimateVar());
                    particle estimate = nodes[j].getEstimateAvg();
                    particle estimateVar = nodes[j].getEstimateVar();
                    memLog << j << "," << estimate.x << "," << estimate.y << "," << estimate.z << "," << estimate.d << "," << estimateVar.x << "," << estimateVar.y << "," << estimateVar.z << "," << estimateVar.d << "," << dist(estimate, nodePositions[j]) << "\n";
                }
            }
        }
    }
    // check convergence of the nodes
    for(int i=0; i<4; i++){
        //  check that the estimated position is close to the true position
        EXPECT_NEAR(dist(nodes[i].getEstimateAvg(), nodePositions[i]), 0.0, 1.0);
    }
    
}

// Log memory usage for different N values
// TEST(FilterPerformanceTest, MemoryUsage) {
//     std::ofstream memLog("memory_usage.csv");
//     memLog << "N,MemoryUsage(KB)\n";

//     particle reference = {0.0, 0.0, 0.0, 0.1};
//     particle referenceVar = {0.1, 0.1, 0.1, 0.1};
//     for (int N : {10, 100, 200, 600, 1000, 2000, 5000, 10000, 100000}) {
//         size_t beforeMem = getMemoryUsage();
//         Filter* filter = new Filter(N);
//         filter->estimateState(1.0, 0,reference, referenceVar);
//         size_t afterMem = getMemoryUsage();
//         size_t memUsage = afterMem - beforeMem;
//         memLog << N << "," << memUsage << "\n";
//         delete filter;
//     }

//     memLog.close();
// }

TEST(FilterPerformanceTest, Runtime) {
    std::ofstream runtimeLog("runtime.csv");
    runtimeLog << "N,Runtime(ms)\n";

    for (int N : {10, 100, 200, 600, 1000, 2000, 5000, 10000, 100000}) {
        // std::cout << "Testing with N = " << N << std::endl;
        Filter filter(N);
        particle anchor = {0.0, 0.0, 0.0, 0.1};
        particle anchorVar = {0.1, 0.1, 0.1, 0.1};

        auto start = std::chrono::high_resolution_clock::now();
        try {
            filter.estimateState(1.0, 0, anchor, anchorVar);
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
