# Trees

mlpack includes a number of space partitioning trees and other trees for its
geometric techniques.  These trees are built on [data matrices](../matrices.md)
where each column in the matrix is a point in the tree.  Trees are organized
such that "nearby" points (with respect to a given distance metric) are
generally grouped in the same node or branch of the tree.

All trees in mlpack implement
the [same API](../../developer/trees.md), allowing easy plug-and-play usage of
different trees.  The following tree types are available in mlpack:

 * [`KDTree`](trees/kdtree.md)
 * [`MeanSplitKDTree`](trees/mean_split_kdtree.md)
 * [`BallTree`](trees/ball_tree.md)
 * [`MeanSplitBallTree`](trees/mean_split_ball_tree.md)
 * [`RPTree`](trees/rp_tree.md)
 * [`MaxRPTree`](trees/max_rp_tree.md)
 * [`UBTree`](trees/ub_tree.md)
 * [`BinarySpaceTree`](trees/binary_space_tree.md)

 * [`CoverTree`](trees/cover_tree.md)

 * [`Octree`](trees/octree.md)

 * [`RTree`](trees/r_tree.md)
 * [`RStarTree`](trees/r_star_tree.md)
 * [`XTree`](trees/x_tree.md)
 * [`RPlusTree`](trees/r_plus_tree.md)
 * [`RPlusPlusTree`](trees/r_plus_plus_tree.md)
 * [`HilbertRTree`](trees/hilbert_r_tree.md)
 * [`RectangleTree`](trees/rectangle_tree.md)

 * [`SPTree`](trees/sp_tree.md)
 * [`MeanSPTree`](trees/mean_sp_tree.md)
 * [`NonOrtSPTree`](trees/non_ort_sp_tree.md)
 * [`NonOrtMeanSPTree`](trees/non_ort_mean_sp_tree.md)
 * [`SpillTree`](trees/spill_tree.md)

---

In general, it is not necessary to create an mlpack tree directly, but instead
to simply specify the type of tree a particular algorithm should use via a
template parameter.  For instance, all of the algorithms below use mlpack trees
and can have the type of tree specified via template parameters:

<!-- TODO: document these! -->

 * [`KNN`](../methods/knn.md): `k`-nearest-neighbor search (exact and
   approximate)
 * [`NeighborSearch`](/src/mlpack/methods/neighbor_search/neighbor_search.hpp)
   (for k-nearest-neighbor and k-furthest-neighbor)
 * [`RangeSearch`](/src/mlpack/methods/range_search/range_search.hpp)
 * [`KDE`](/src/mlpack/methods/kde/kde.hpp)
 * [`FastMKS`](/src/mlpack/methods/fastmks/fastmks.hpp)
 * [`DTB`](/src/mlpack/methods/emst/dtb.hpp) (for computing Euclidean minimum
   spanning trees)
 * [`KRANN`](/src/mlpack/methods/rann/rann.hpp)

---

***Note:*** if you are looking for documentation on **decision trees**, see the
documentation for the [`DecisionTree`](../methods/decision_tree.md) class.
