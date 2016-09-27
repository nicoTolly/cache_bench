#references 

https://computing.llnl.gov/tutorials/openMP/ProcessThreadAffinity.pdf
pdf sur les affinités, comment lier une tâche à un coeur ou une socket.

http://www.openmp.org/mp-documents/OpenMP3.1.pdf
OpenMP specification

http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.107.9768&rep=rep1&type=pdf
modèle mémoire openmp

https://en.wikipedia.org/wiki/Work_stealing

http://mspiegel.github.io/publications/ijhpca11.pdf
article sur une implementation du runtime d'openmp pour NUMA

http://prace.it4i.cz/sites/prace.it4i.cz/files/files/advancedopenmptutorial_2.pdf
Des "conseils" pour utiliser openMP avec NUMA

http://runtime.bordeaux.inria.fr/forestgomp/
Un projet runtime pour openMP avec du support pour NUMA

https://en.wikipedia.org/wiki/Non-uniform_memory_access

https://gcc.gnu.org/onlinedocs/libgomp/
la doc de gcc pour libgomp ( utile pour comparer )

http://llvm.org/docs/LangRef.html
la doc de la llvm intermediate representation

http://reviews.llvm.org/D13991
le commit lié à l'utilisation de hwloc

http://developerblog.redhat.com/2014/03/10/determining-whether-an-application-has-poor-cache-performance-2/
site expliquant comment evaluer la performance avec perf

https://terboven.com/2012/06/21/the-design-of-openmp-thread-affinity/
design pour openmp places, affinity (cores, scatter,etc.)

https://www.open-mpi.org/projects/hwloc/doc/v1.3.1/a00040.php#gacd37bb612667dc437d66bfb175a8dc55
doc de hwloc et description des differents objets.

http://frankdenneman.nl/2015/02/27/memory-deep-dive-numa-data-locality/
An article on NUMA architecture, what it is and why it exists

https://software.intel.com/sites/landingpage/IntrinsicsGuide/#techs=AVX,AVX2&cats=Load&expand=483,485,484,3024,3262,3043
Intel reference manual for avx intrinsics

http://www.realworldtech.com/haswell-cpu/4/
Sandy bridge and Haswell, which instructions they have, what they
do, etc.


http://www.realworldtech.com/sandy-bridge/8/
informations on sandy bridge L3 cache

https://software.intel.com/en-us/articles/detecting-memory-bandwidth-saturation-in-threaded-applications/
self speaking

http://www.drdobbs.com/parallel/eliminate-false-sharing/217500206
serie of articles of false sharing (private threads data located in same cache line)
