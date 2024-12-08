# analysis of the Filter behaviour



## Geometric convergence

The following showcases the evolution of the distance between estimates and real positions. In this test one round of updates are done to the anchors and then updates are only performed according to the mesh. This test uses a 1000 particles per filter, which is sub-optimal.

![geom_convergence](geometry_convergence.png)

## Convergence according to N 

The following showcases the evolution of the distance between estimates and real positions according to the number of particles. 

![convergence](distance_evolution.png)

## runtime

Here is the runtime according to the number of particules.

![runtime](runtime_evolution.png)

# strategies

## Best First

This strategy starts each round by ordering the particles according to their average variance. Then best nodes are updated first from the best particles.

![best_first](Strategy_BestFirst.png)

## Worst first

This strategy starts each round by ordering the particles according to their average variance. Then worst nodes are updated first from the best particles.

![worst_first](Strategy_WorstFirst.png)

## only better 

This strategy only perfomes updates to nodes that have a smaller avg variance then the particle.

![only_better](Strategy_onlyBetter.png)

## only worst

This strategy only perfomes updates to nodes that have a larger avg variance then others.

![only_worst](Strategy_onlyWorstfromBetter.png)

## only worst axis from better

This strategy only perfomes updates to nodes that have a larger avg variance then others, but only on the axis with the largest variance.

![only_worst_axis](Strategy_onlyWorstAxisFromBetter.png)

## random

This strategy randomly selects nodes to update.

![random](Strategy_Random.png)

