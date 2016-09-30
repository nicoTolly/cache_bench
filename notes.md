#Notes sur OpenMP et l'algorithme de Cholesky


trouvé d'abord une implémentation de Cholesky en C standard,
celle-ci fonctionne aussi bien avec clang qu'avec gcc.


gcc (GCC) 5.3.0
Copyright © 2015 Free Software Foundation, Inc.
Ce logiciel est libre; voir les sources pour les conditions de copie.  Il n'y a PAS
GARANTIE; ni implicite pour le MARCHANDAGE ou pour un BUT PARTICULIER.


clang version 3.7.1 (tags/RELEASE_371/final)
Target: x86_64-unknown-linux-gnu
Thread model: posix

les versions de gcc et clang de mon ordinateur.

Cherché ensuite une version de cholesky implémentée pour openMP, pas mal galéré.
Finalement, après avoir trouvé un lourd dossier de benchmark comprenant notamment un cholesky,
sans réussir à l'adapter, j'ai simplement adapté le code en C original. Ajouté ceci :

#pragma scop
#pragma omp parallel
	{
#pragma omp for private (j, k)
	for (int i = 0; i < n; i++)
		for (int j = 0; j < (i+1); j++) {
			double s = 0;
			for (int k = 0; k < j; k++)
				s += L[i * n + k] * L[j * n + k];
			L[i * n + j] = (i == j) ?
				sqrt(A[i * n + i] - s) :
				(1.0 / L[j * n + j] * (A[i * n + j] - s));
		}
}
#pragma endscop

( toutes les pragmas ).

Compile sans souci ( et fonctionne ) avec gcc : gcc -Wall -o cholesky cholesky.c -lm

Pour clang par contre, il  y a un souci: il faut ajouter une option à la commande de base.
clang -o cholesky cholesky.c -lm -fopenmp

Aussi du faire: 
sudo cp /usr/lib/gcc/x86_64-unknown-linux-gnu/5.3.0/include/omp.h /usr/lib/clang/3.7.1/include/

on aurait pu se contenter d'un -I dans la ligne de commande.

Ensuite tout fonctionne sans problème. Il s'agirait maintenant de voir comment openMP modifie le
comportement du programme ( si même il le modifie ).

Premier point : les fichiers assembleurs générés sont les memes.

En fait les pragmas sont ignorées.

libgomp est bien installé


Tenté un petit hello avec openmp. Marche bien avec gcc à condition d'ajouter -fopenmp également.
Ne fonctionne pas avec Clang


#04/04

Le programme parallèle est en fait super instable: parfois des nan ou des inf ( float proche de 0 ) interviennent ).

Aussi, mettre j et i dans shared change le programme en boucle infinie. À éclaircir.


Récupéré des fichiers de test, notamment un exemple de boucle. Le mot clé schedule est sympa : permet 
de distribuer les tâches aux différents threads. On choisit static ou dynamic ( static répartissant a 
priori entre les threads, dynamic permettant de les répartir à l'exécution et d'en donner plus 
au plus rapide ).

Le souci est en fait simple : il y a de grosses dépendances entre les itérations de la boucle.

Tenté d'ajouter la clause ordered, ne change rien.

Placé le pragma uniquement sur la boucle interne. Comme on peut s'y attendre, le comportement 
devient déterministe (plus de dépendances entre itérations ).

On peut même paralléliser les deux boucles internes ( en fait, on calcule une colonne d'après
la colonne précédente, mais dans une même colonne, les lignes sont indépendantes ).

C'est sans doute à ça que servait la clause "private(j, k)"

Déclarer i, j et k avant les boucles déclenche une boucle infinie ( problème de localité quelconque )



#my program

Réalisé ( sur conseil de Fabrice ) un petit programme qui se contente de tourner sur le CPU, en faisant
des calculs inutiles ( juste pour tourner ).

L'outil perf permet d'analyser les performances du programme, les défauts de pages ou défauts de cache 
notamment. On devrait voir les cache miss augmenter quand on va allouer des tableaux privés de la taille 
du cache L1.

Utilise timeout pour arrêter le programme après 10s.

la commande est : perf stat -e cache-misses timeout 10 ./example

On voit en effet que le nombre de caches-misses augmente si on met les deux tableaux en private,
et qu'en plus on met une grande taille

# re cholesky
Retour rapide au cholesky: on peut arriver à un programme déterministe (testé seulement 4 ou 5 fois) 
en ajoutant une directive ordered.
Cette directive encadre un bloc qu'on veut voir être exécuté dans le même ordre que dans le cas séquentiel.
Il faut préciser que for est ordered ET rajouter le pragma ordered dans la boucle.

Sinon, fait un petit script pour récupérer les caches-misses et les stocker dans un fichier ( countMissed.sh)
Utilisé awk pour le script ( on pipe le perf dans un grep ( il faut piper le stderr ) et on utilise awk
dessus ).

Les caches-misses sont vraiment aléatoires. Je pense qu'ils dépendent aussi du reste de l'environnement.

Grosses difficultés pour installer clang et llvm.

Le linkage à la fin pose problème.

Récupéré à chaque fois les release 3.8 pour llvm, puis clang ( cloné dans llvm/tools ) puis openmp
( cloné dans llvm/projects ). Utilisé à chaque fois le clonage en ssh sur github, comme conseillé par
Philippe ( ça a pris quand même pas mal de temps ).

Créé build dans stage. 
fait ( dans build ) cmake ../llvm -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
puis make omp

Ensuite, toujours dans build:
cmake "Unix Makefiles"
make -j 3

Obtenu à la fin une erreur sur le ld qui a reçu une interruption

En fait, le linkage plante parce que la RAM est débordée ( le linkage essaie de prendre 10 Gi, il n'y en
a que 8 ).

On peut installer la release plutôt que la version debug, beaucoup plus lourde.

La ligne de cmake est alors:
(voir plus tard)

Possibilité avec perf d'obtenir uniquement les caches-misses pour le L1, séparés en cache-load-misses et
cache-store-misses.

Difficile de noter une différence sur ces chiffres là également.


Fini par réussir à compiler et installer: finalement il ne faut pas installer la release mais MinSizeRel.
La ligne cmake est : 
cmake ../llvm/ -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=MinSizeRel

Il faut aussi exporter LD_LIBRARY_PATH. À faire : faire cela automatiquement.

#06/04

Clang finalement installé.

Essayé de compiler cholesky avec : il semble que la directive ordered soit mal comprise. On retombe 
en effet sur les nan, etc.

Fabrice m'a donné un bout de code pour faire tourner des benchmarks. Compilation pas si simple:

Il faut ajouter le flag -mavx pour indiquer le matériel présent ( des instructions de parallélisation
simd ) 
ajouter aussi ( avec gcc, pas testé avec clang ) le flag -fpic ( position independant code )
Sans ça, ld renvoie une erreur.

Fait un Makefile pour pouvoir compiler tous les exécutables à la fois ( en définissant les bonnes
macros )


Idées: profiling, regarder l'exécution dynamiquement pour prédire les accès mémoire futurs pour placer
les affinités.

Voir l'algo d'Alain ?

Autre idée:

Avec philippe, voir le "vol de tache".
Une tache qui fait beaucoup d'appel à distance perd en performance.
Vérifier si une tache va faire beaucoup d'appel à la mémoire, si oui, interdire
le vol. Prioriser les vols sur les taches computationnelles et favoriser les taches
"mémorielles" en priorité.

Trouver des exemples de programmes en openmp. Beaucoup de taches différentes ?

##Brancher un programme compilé avec clang sur une librairie gcc

Copier la bibliothèque qu'on veut utiliser dans un répertoire quelconque.
ajouter ce répertoire à LD_LIBRARY_PATH ( éventuellement en l'écrasant ).
on peut vérifier avec ldd ( permet de voir les bibliothèques utilisées ).

#07/04

##Who uses openmp ?

openmp.org/wp/whos-using-openmp/

Quelques applications sont listées sur cette page. Malheureusement, elles sont
pour la plupart commerciales.

Les projets évoquent beaucoup la parallélisation automatique de boucle avec #pragma
omp for mais on trouve aussi la mention d'utilisations de task.

Un des programmes ( libdoubanm ) dit utiliser des task pour faire des parcours de 
graphe.

##Affinity

Il existe une bibliothèque C pour spécifier une affinité sur un thread. 
Les deux fonctions centrales sont sched_getaffinity et sched_setaffinity.
Des macros permettent de travailler sur les structures cpu_set_t.

ATTENTION : il faut définir la macro _GNU_SOURCE AVANT d'inclure <sched.h> sans
quoi les macros ne sont pas définies ( ni les fonctions, d'ailleurs, mais ce n'est
pas un souci dans ce cas).

#08/04

Réécrit un programme aff pour tester les affinités avec des forks. Cette fois, le programme
fonctionne bien, on voit avec htop que le travail est bien affecté différamment sur chacun 
des coeurs.

En openmp, ces paramètrages sont probablement redéfinis à part. Il faudrait changer le runtime
pour arriver à quelque chose de plus convaincant. 



##Discussion Philippe

Pas mal de choses, plusieurs possibilités pour la suite.

Existence ( et bientot finition ) d'un programme qui peut garder la localité d'une tache sur
un numa. Une file de tache dans lequel un autre processeur ( éventuellement distant ) peut
piocher. Garder la localité : demander à ce que deux taches soient exécutées au meme endroit.

Dans les choses à faire : soit, à la compilation, "calculer" ou estimer une intensité opérationnelle
pour une tâche. Ensuite, la passer au runtime pour qui applique la stratégie appropriée.

Autrement, déterminer cette stratégie : dans quel cas autoriser le vol, dans quel cas garder la localité.

On peut baser la file du runtime sur l'OI déterminée.

Intensité opérationnelle : "rapport" entre nombre d'accès mémoire à un secteur et taille de ce même secteur. Pas trop de trace sur internet.

Plein de chose sur l'implémentation en mémoire de openmp. Une nouvelle variable est créée pour chaque
variable privée, une simple référence vers le tas pour les variables shared. 

Fork trop coûteux : trop de choses sont copiées ( état de la stack, du tas...) On utilise plutôt pthread.

Kastors

#14/04

retour avec nouvel ordi. Un : le benchmark que m'a donné Fabrice ne
compile plus. Pas encore trouvé de fix ( à voir si j'en trouverai
un ) 

Continué mes petits tests avec cholesky. Génère des matrices 
définies positives symétriques avec Hilbert ( a en plus l'avantage 
d'être déterministe )
Fait aussi un Makefile qui compile trois fichiers : basic-cholesky,
 cholesky-for (avec la directive ordered ) et cholesky-dep ( avec 
 les dépendances ). Surprise (ou pas) la version basique est 
 largement plus rapide que les deux autres.

 La version dep bat nettement la version for, mais reste du même 
 ordre de grandeur.

 Il est probable que la granularité soit trop fine : trop de temps
 passé à répartir les tâches. À noter qu'au début, le programme 
 créait ses tâches dans la boucle interne, ce qui faisait encore 
 plus de granularité. Cette version était moins performante que la 
 boucle for.

 Comment diminuer la granularité ?

 #15/04

 Quelques tests sur la directive ordered de openmp.

 Fait d'abord un fichier avec une boucle simple qui affiche la valeur
 de i. Compilé avec gcc puis clang, on obtient le comportement attendu.

 Pas de souci non plus avec une double boucle
 peut-être des optimisations de code mort ?
 Compliqué un peu les choses, introduit un sleep dans chacune des
 itérations, juste après un print non ordonné, et avant un print ordonné.
 À ce moment, le programme compilé avec gcc se comporte toujours comme 
 attendu ( les print non ordonnés arrivent un peu au hasard, ceux qui sont
 ordonnés apparaissent dans le bon ordre ). Par contre, le programme compilé
 avec clang se comporte de manière étrange, et l'ordre est perdu.

 relativisons les performances observees sur cholesky : même avec une boucle
 simple et sans dépendance, le gain de performance est peu évident (
 peut-être des optimisations de code mort ?)

 Il y a manifestement un souci au niveau de mon chrono. 
 Clock_gettime semble mieux approprie ( clock() se contente
 du temps passe par l'application dans le processeur)

 #18/04

 clock_gettime(CLOCK_MONOTONIC, ...) est bien mieux appropriee
 pour les tests. Les resultats sont bien plus stables qu'avant.

 Des lors, les resultats sont plus proches de ceux attendus. A 
 noter que pour cholesky-dep on obtient des performances tres 
 inferieures a la version basique avec num_threads=1, mais les
 performances deviennent comparables avec num_threads=2. Pas de 
 differences notables lorsque num_threads=4 ou 8 par contre. 

 #19/04

 re-discussion avec Philippe:
 envoyer un mail a fred desprez pour grid5000 et a christian seguy
 pour idchire (oui...)

 Si travail cote runtime : essayer le run-time de clang ( en fait
  celui d'Intel ) plus "facile" d'acces que kaapi notamment.

  fait un wrapper de clock_gettime ( on pourrait eventuellement
   ajouter d'autres timers, mais CLOCK_MONOTONIC fait bien le job,
   coherente avec d'autres trucs)

On obtient ENFIN des temps inferieurs pour une boucle for pour
des iterations tres longues ( au moins 500M calculs flottants )

#20/04

Envoyé mail à fred desprez pour grid5000 et christiqn seguy pour idchire,
suivant le conseil de Philippe.

Petit retour sur les deux runtimes : c'est bien le runtime de clang qui 
plante sur ordered ( le problème n'est apparemment pas situé à la 
compilation ). Testé omp-for avec le runtime intel : copié
build/lib/libomp.so dans le répertoire courant, puis fait 
LD_PRELOAD="libomp.so" ldd ./omp-for qui prouve que libomp.so est bien
utilisée.

Puis fait LD_PRELOAD="libomp.so" ./omp-for.
On a toujours le for désordonné.

#21/04

Aujourd'hui, recu les acces a idchire. Installation de clang (youhou)
et de gcc (version trop vieille). Il faut installer gcc avant clang.

Pour installer la derniere version de gcc, il faut recuperer gmp, mpfr
et mpc. 

Tous les binaires sont installes dans ntollena/local

Il a fallu installer aussi cmake

pour mpfr, bien penser a actualiser LD_LIBRARY_PATH, de sorte qu'il
trouve bien la bonne version de gmp. Ce sera pareil pour gcc


./configure --prefix=/home/ntollena/local/ --with-gmp-lib=/home/ntollena/local/lib/ --with-gmp-include=/home/ntollena/local/include/ --with-mpfr-lib=/home/ntollena/local/lib/ --with-mpfr-include=/home/ntollena/local/include/

Une ligne de configuration (en l'occurence pour mpc, mais dans le principe
 ca doit marcher pour tout)


 La ligne utilisee, apres bien des soucis, pour gcc :

  ../gcc-5.3.0/configure --prefix=/home/ntollena/local/ --with-gmp=/home/ntollena/local --with-mpfr=/home/ntollena/local --with-mpc=/home/ntollena/local --disable-multilib

  faite dans le repertoire gcc/build.

  Bien faire pointer LD_LIBRARY_PATH sur lib.

entre LD_LIBRARY_PATH dans le .bashrc

Il faut python pour compiler clang

#22/04

Clang installé !!

Ça ne servait à rien d'installer gcc.
Commande module load permet de charger des modules.
module unload pour les décharger. 
module list pour lister les modules chargés.
module avail pour lister les modules disponibles.

Pour initialiser la commande, faire source /usr/share/modules/init/bash

Il y a entre autres gcc, python et cmake/

Sinon, pour les soucis de compilation avec clang :
Il faut préciser explicitement la bibliothèque à charger.
Ici, il faut donc ajouter -lrt à la fin de la ligne de compilation.


#25/04

Quelques notes sur le fonctionnement d'openmp.

L'implementation a deux facettes : la compilation et le runtime.

Le compilateur est chargé de transformer chacune des sections openmp
en procédure correspondante. Les variables shared sont passés par référence,
les variables privates directement dans la pile du thread, de manière à 
rendre le scope explicite. Le compilateur fait aussi appel à des fonctions 
de la bibliothèque du runtime ( appel par exemple aux fonctions kmpc... dans
l'assembleur généré par clang, appel à gomp pour gcc ).

Le runtime est ensuite de son coté chargé d'appeler les bonnes procédures
sur chacun des threads, selon l'architecture à sa disposition et les 
indications de l'OS et du programmeur ( variable OMP_NUM_THREADS par ex )
Tout ceci est sans doute à préciser.

##llvm Intermediate Representation

Assembleur, yay !!

Quelques remarques sur l'IR de llvm,

@ est utilisé pour la déclaration de variables/fonctions globales.
% est utilisé pour les variables locales.

Les préfixes permettent de ne pas s'inquiéter de collisions avec les
noms réservés (qui ne prennent pas de préfixes )

Pour les types : relativement naturel pour les entiers. i32 est un entier
stocké sur 32 bits. Notons que le nombre de bits est arbitraire.

type de fonctions : <return-type > (types des paramètres éventuels )
par ex i32 (i32, i32) prend deux entiers en arguments et renvoie un
entier.

type pointeurs : spécifiés par des étoiles, le type pointé et le nombre
d'éléments est spécifié entre crochets.
Ex : ```[4 x i32]* ``` représente un pointeur vers 4 entiers.
``` i32 (i32 *)* ``` est un pointeur vers une fonction qui prend un 
pointeur sur un entier en paramètre et renvoie un entier. 

assez naturellement, les tableaux sont représentés entre crochets :
```[5 x i16]``` est un tableau de 5 entiers de 16 bits.

cas particulier : une string constante peut être représentée par
"c"chaine constante"".

instructions binaires : ```add ty op1 op2``` 
op1 et op2 doivent etre de meme types ( entier ouéventuellement 
des vecteurs d'entiers ) on peut ajouter nsw et nuw pour "non 
signed wrapped" et "non unsigned wrapped" qui font que la valeur 
renvoyée est "empoisonné" en cas de dépassement d'entier.

fadd pour additionner des types flottants.

Des fonctions équivalentes existent pour sub, mul et div

alloca alloue de la mémoire sur la pile. Le type est obligatoire.
alloca peut prendre un nb d'éléments en deuxième argument, et un 
alignement en troisième.
Comme la variable est allouée sur la pile, elle est automatiquement
détruite à la sortie de la fonction.

Ex : alloca i32, i32 10, align 2048 ( puissance de deux obligatoire )

getelementptr sert à extraire l'adresse d'un élément d'une structure
aggrégée.

syntax :
<result> = getelementptr <ty>, <ty>* <ptrval>{, <ty> <idx>}*
<result> = getelementptr inbounds <ty>, <ty>* <ptrval>{, <ty> <idx>}*
<result> = getelementptr <ty>, <ptr vector> <ptrval>, <vector index type> <idx>

un code en C :
```
struct RT {
  char A;
  int B[10][20];
  char C;
};
struct ST {
  int X;
  double Y;
  struct RT Z;
};

int *foo(struct ST *s) {
  return &s[1].Z.B[5][13];
}
```
et le code llvm correspondant :

```
%struct.RT = type { i8, [10 x [20 x i32]], i8 }
%struct.ST = type { i32, double, %struct.RT }

define i32* @foo(%struct.ST* %s) nounwind uwtable readnone optsize ssp {
entry:
  %arrayidx = getelementptr inbounds %struct.ST, %struct.ST* %s, i64 1, i32 2, i32 1, i64 5, i64 13
  ret i32* %arrayidx
}
```

##libgomp and libomp

Le runtime de gcc N'EST PAS compatible avec un programme 
compilé avec clang.

La raison est sans doute que les fonctions appelées sont 
différentes ( les kmp... pour clang et GOMP... pour gcc ). On
peut supposer que des indirections permettent au runtime de clang
de rediriger les appels à GOMP vers des fonctions kmp, ce qui 
permet d'utiliser le runtime de clang avec des fonctions compilées
avec gcc.

Pour la compatibilité gcc -> libomp, les symboles sont définies 
dans kmp_ftn_os.h et le lien vers les fonctions kmp est fait 
dans kmp_gsupport.c.
Toujours bon à savoir

Fonction utile : nm permet de lister tous les symboles définis 
dans un fichier binaires exécutable. Permet de savoir quelle 
fonction on appelle.

Petite note : sleep caste automatiquement son argument en int, 
il serait bon d'y faire quelque chose


#26/04

Après beaucoup trop de recherche, trouvé la définition de 
kmp_fork_call dans kmp_runtime.c

kmp.h l2296, la structure qui definit les infos sur un thread


27/04

Pour la definition de tâches :

kmpc_alloc_task est la première fonction appelée.

Elle appelle elle-même kmp_alloc_task ( les fonctions kmp sont
internes au runtime, kmpc sont externes ) qui alloue une structure
de taille suffisante en fonction des variables qui seront accédées.

On trouve un kmp_alloc_task_deque... Voir ce que c'est.

```
//------------------------------------------------------------------------------
// __kmp_alloc_task_deque:
// Allocates a task deque for a particular thread, and initialize the necessary
// data structures relating to the deque.  This only happens once per thread
// per task team since task teams are recycled.
// No lock is needed during allocation since each thread allocates its own
// deque.
```

Commentaires de kmp_alloc_task_deque

kmp_alloc est un wrapper de malloc qui prend en plus en compte l'alignement
Utilisé pour toutes les allocations.

Structure intéressante : kmp_thread_data_t, l2234 de kmp.h, définit
toutes les infos relatives au thread.

```
// Data for task team but per thread
typedef struct kmp_base_thread_data {
    kmp_info_p *            td_thr;                // Pointer back to thread info
                                                   // Used only in __kmp_execute_tasks_template, maybe not avail until task is queued?
    kmp_bootstrap_lock_t    td_deque_lock;         // Lock for accessing deque
    kmp_taskdata_t **       td_deque;              // Deque of tasks encountered by td_thr, dynamically allocated
    kmp_uint32              td_deque_head;         // Head of deque (will wrap)
    kmp_uint32              td_deque_tail;         // Tail of deque (will wrap)
    kmp_int32               td_deque_ntasks;       // Number of tasks in deque
                                                   // GEH: shouldn't this be volatile since used in while-spin?
    kmp_int32               td_deque_last_stolen;  // Thread number of last successful steal
#ifdef BUILD_TIED_TASK_STACK
    kmp_task_stack_t        td_susp_tied_tasks;    // Stack of suspended tied tasks for task scheduling constraint
#endif // BUILD_TIED_TASK_STACK
} kmp_base_thread_data_t;

typedef union KMP_ALIGN_CACHE kmp_thread_data {
    kmp_base_thread_data_t  td;
    double                  td_align;       /* use worst case alignment */
    char                    td_pad[ KMP_PAD(kmp_base_thread_data_t, CACHE_LINE) ];
} kmp_thread_data_t;

```
Des trucs qui seront utiles par la suite :
```

typedef hwloc_cpuset_t kmp_affin_mask_t;
# define KMP_CPU_SET(i,mask)       hwloc_bitmap_set((hwloc_cpuset_t)mask, (unsigned)i)
# define KMP_CPU_ISSET(i,mask)     hwloc_bitmap_isset((hwloc_cpuset_t)mask, (unsigned)i)
# define KMP_CPU_CLR(i,mask)       hwloc_bitmap_clr((hwloc_cpuset_t)mask, (unsigned)i)
# define KMP_CPU_ZERO(mask)        hwloc_bitmap_zero((hwloc_cpuset_t)mask)
```

KMP_CPU_SET est appelé dans kmp_balanced_affinity. Il permet de configurer
l'affinité des threads sur les coeurs ( je pense )
fonction défini à la ligne 4505 de kmp_affinity.cpp

Elle équilibre la charge de travail entre les différents coeurs, 
en tenant notamment compte des disparités d'architecture.

Plusieurs cas sont envisagés, un cas d'architecture uniforme, un autre
pour architectures non uniformes ( les structures qui gèrent les deux
cas sont différentes ), si le nombre de threads à répartir est inférieur
ou supérieur au nombre de coeurs.

kmp_balanced_affinity est appelé dans kmp_fork_barrier

```
enum affinity_type {
    affinity_none = 0,
    affinity_physical,
    affinity_logical,
    affinity_compact,
    affinity_scatter,
    affinity_explicit,
    affinity_balanced,
    affinity_disabled,  // not used outsize the env var parser
    affinity_default
};

enum affinity_gran {
    affinity_gran_fine = 0,
    affinity_gran_thread,
    affinity_gran_core,
    affinity_gran_package,
    affinity_gran_node,
#if KMP_GROUP_AFFINITY
    //
    // The "group" granularity isn't necesssarily coarser than all of the
    // other levels, but we put it last in the enum.
    //
    affinity_gran_group,
#endif /* KMP_GROUP_AFFINITY */
    affinity_gran_default
};
```
les possibilités de configuration pour l'affinité. kmp_balanced_affinity
est utilisée pour le cas affinity_balanced, sans trop de surprise.

kmp_fork_barrier est appelée notamment à la fin de kmp_internal_fork.
Ça correspond à la spec qui dit qu'une barrière implicite est placée
en début d'une région parallèle ( mais aussi à la fin )


Il est possible de compiler libomp de manière à utiliser hwloc plutôt
ue leurs built-in fonctions qui ont l'air assez compliquée. 
Peut-être envisager de recompiler ma version si cela peut rendre les
choses plus simples.

#28/04

les fonctions liées à la gestion des tâches sont dans kmp_tasking.cpp
( sans déconner... ) 

dans les points d'entrée pour le tasking, on a kmpc_omp_task_alloc,
kmpc_single, kmpc_omp_task, kmpc_omp_taskwait et kmpc_end_single, 
avec d'autres sans doute.

kmpc_fork_call est appelé avec 8 pour valeur de gtid (je ne sais pas
trop pourquoi )

intéressant : à chaque appel de task_alloc est passé en argument la 
taille des données privées de la tâche, ainsi que la taille des données
partagées. Peut-être un truc à exploiter ici.


Idchire a les tailles de caches suivantes :

L1d et i : 32K
L2 : 256K
L3 : 20480K

Avec un tableau de float (32 bits ou 4 octets ) on est censé dépasser la
taille du cache pour une taille de 8192 éléments

Retour aux affinités :

la variable KMP_AFFINITY permet d'épingler les threads aux processeurs.
compact met les threads aussi près que possible les uns des autres
scatter au contraire les éloigne autant que possible
balanced est censé faire un truc équilibré.

affinity balanced n'est pas dispo sur idchire
L'option n'est pas disponible dans le cas où il y a plusieurs packages.

KMP_AFFINITY="explicit; proclist=[0,7]" OMP_NUM_THREADS=2 ./task-test2

Ligne de commande pour épingler deux threads sur deux processeurs 
différents. Si on veut avoir deux sockets différentes, on peut faire
par exemple proclist=[0,8]

Les temps de calculs varie, tout en restant du même ordre de grandeur,
parfois du simple au double pour task-test2

Les deux tâches définies dans ce programme font chacune accès à la 
même variable partagée et écrivent dedans.

#29/04

L'équivalent de KMP_AFFINITY pour gcc est GOMP_CPU_AFFINITY

utilisation : GOMP_AFFINITY="0 4" pour lier les deux premiers
threads aux processeurs 0 et 4.

Problème : impossible de vérifier que l'effet escompté a bien
eu lieu ( pas de verbose )

La question de la journée:

Créé un programme openmp simple avec deux tâches partageant
un tableau et faisant des accès en lecture/écriture à ce tableau.

Testé ensuite le programme dans deux configurations ; une où les 
deux threads tournent sur deux processeurs du même noeud et une
où il tourne sur deux processeurs de deux noeuds différents.

Je m'attendais au départ à ce que l'écart de performance entre
les deux versions soient de plus en plus grand avec l'augmentation
de la taille du tableau.

En fait, le résultat est très différent :

On s'aperçoit que la performance varie énormément pour des valeurs
très petites de N (50 par exemple) mais s'estompe pour des valeurs
plus grandes. 

De plus, la performance est même bien meilleure pour des résultats
plus grands.

Les observations sont les mêmes avec gcc et clang qualitativement,
quand bien même gcc est beaucoup plus performant. On peut noter
aussi qu'avec gcc les différences deviennent rapidement 
inexistantes.

Deuxième test :

On regarde la valeur que prend t[0] à la fin.
Du fait de l'absence de synchronisation, le résultat est
censé être indéfini (rien ne règle les accès)

Peut-être intéressant de regarder ce qu'il se passe avec un code
optimisé.

#02/05

Par rapport aux accès à t[0]:

La valeur est très instable pour des valeurs de N petites, elle
devient stable quand N devient grand (=2.0).
En fait, c'est logique, pour un N plus grand t1[0] est beaucoup
moins incrémenté.

Nota bene : 

2.0 est la valeur attendue pour une exécution séquentielle.
On en déduit que dans ce cas on a pas eu de race condition.

Dans le cas N=50000, le programme est stable quand il est
maintenu sur un même noeud et plutôt instable sinon.

Donc la localité a un impact sur la stabilité du programme.

Preuve est faite, en tout cas, que les écritures en mémoire
sont faites régulièrement et pas en une seule fois à la fin.

for i in {1..40}; do KMP_AFFINITY="explicit, proclist=[0,8]" OMP_NUM_THREADS=2 perf stat -e cache-references -e cache-misses ./task-test2 2>&1  | grep cache | python getNumber.py >> result/clang/cache-misses-50.txt; done

Mesure de performances par rapport aux caches :

Les caches misses augmentent de plusieurs ordres de grandeurs
quand on passe du programme exécuté localement au programme
exécuté sur deux noeuds différents.

fait un script pour récupérer les valeurs de cache misses, les
pourcentages, les moyennes et les écart-types.

Les optimisations font réapparaître les différences de performance
entre le programme exécuté localement et sur deux coeurs distants.

Comment est maintenu la cohérence entre le cache et la mémoire ?

Juste refait le programme avec cette fois les variables en
private. Cette fois, très peu de cache misses dans les deux
cas, comme attendu.

#03/05

Refait les tests de cache-misses avec le programme compilé cette
fois avec gcc. Les résultats se confirment.

Truc marrant : GOMP_CPU_AFFINITY semble marcher avec un programme
compilé avec clang.

pas de différence apparente entre le programme exécuté sur les
coeurs 0 et 1 ou les coeurs 0 et 7 (apparemment pas de partage
de cache L1 ou L2)

Compatibilité GOMP_CPU_AFFINITY définie dans kmp_settings.c

Le compte des cycles CPU est en relative cohérence avec le
timer. Donc pas de problème de timer mal ajusté, ou mesurant
le mauvais événement.


Des infos cools chez François et Philippe:

Pour eux, la différence de performance est significative.
À plus de 15%, on peut commencer à s'inquiéter sérieusement.
Donc le travail est intéressant.

D'autre part, pour voir l'architecture de la machine:
charger le module hwloc (module add hwloc)
puis faire lstopo

Si on a ouvert la connexion ssh en mode X
ssh -X ntollena@idchire.imag.fr

On a une interface graphique, ce qui est bon.


Les coeurs ne partagent QUE le cache L3 entre eux. Les caches
L2 et L1 sont individuels.

Pas d'idée particulière sur le pourquoi de la performance qui
croit avec la taille du tableau.

Peut-être quelque chose à tenter avec la bibliothèque atomic ?
À voir.

Il est bon de noter que la stratégie de base de libomp est
"scatter", donc potentiellement nuisible en cas de variables
partagées.

Peut-être qu'une simple stratégie consistant à adopter "compact"
en voyant des variables partagées pourraient améliorer la 
performance - c'est sans doute ce que fait déjà Philippe.

Reste à voir pour les vols de tâches.

OMP_PLACES remplace avantageusement KMP_AFFINITY et 
GOMP_CPU_AFFINITY

In practice, you can assume that int is atomic. You can also assume that pointer types are atomic; that is very convenient. Both of these assumptions are true on all of the machines that the GNU C Library supports and on all POSIX systems we know of. 

Tiré de la documentation de glibc. Donc les standards du C n'impose
aucune règle sur l'atomicité, mais en pratique n'importe quel
système POSIX l'implémente.

Pour les opérations atomiques :
impossible de réaliser des opérations arithmétiques atomiques
de base sur les flottants ( le manque de support hardware
rendrait l'implémentation trop inefficace )

Donc il faut réaliser les tests avec des entiers si l'on veut
avoir des résultats.

La ligne de compilation pour les std::atomic :
g++ -Wall -fopenmp -O1 -std=c++11 -o task-atomic task-atomic.cpp mytimer.c -lrt

Depuis le site de Redhat:

Une métrique pertinente pour mesurer l'utilisation du cache est
le rapport des caches-misses sur le nombre d'instructions.

Dans notre cas, ce rapport est dans les deux configurations très
bon, quand bien même il est moins bon sur deux noeuds différents.

Une autre métrique utile est le nombre d'instructions par cycle.
Dans le cas N=50, ce rapport tombe à 1,01 pour une application
localisée, contre 0,21 pour l'application "étendue" (besoin d'un
meilleur terme). C'est sans doute là qu'il faut chercher la raison
de la perte de performance.

En fait, le nombre d'instructions explose avec la réduction de la
taille du tableau. Moins d'un million d'instructions pour le
programme au tableau de 5 millions, contre 3 milliards pour
le programme à 50.

Il ne semble pas y avoir d'instructions supplémentaires exécutées
entre les deux configurations (compact et scatter)

#04/05

L'explosion du nombre d'instructions est fausse : juste dûe à un
stackoverflow.

Taille maximum de la pile : 8MB, soit 2M de flottants.

On peut s'en sortir en plaçant des variables globales.

Les effets sur la performance sont assez bizarres.

Si l'on met les données sur le tas, les rapports s'inversent :
le programme est plus efficace quand il est partagé entre les 
noeuds !!

résultat apparemment valable uniquement si la taille du tableau
dépasse celle de la pile.

#09/05

L'overhead pour allouer la mémoire devient très significatif
environ à partir de 2^25 flottants ( à la louche )
Ce coût peut être attribué sans trop de doute à la boucle
d'initialisation.

On observe toujours la différence repérée plus haut lorsque 
le tableau fait plus de 2^22 flottants.

On peut peut-être l'attribuer à une concurrence pour les accès
au cache ?

Pour forrestgomp :

Comme le nom l'indique, une adaptation de libgomp.
Idée de base : différentes stratégie de scheduling pour chaque
niveau de hiérarchie.

Utilise le cache-misse rate pour évaluer l'affinité !!

Le principe est de migrer occasionnellement des données pour
les tenir autant que possible près des tâches les exécutant.
Cela implique en particulier qu'une mesure de la localité
des données est faite !!

Un benchmark intéressant est utilisé, STREAM.

Pour mesurer un événement en particulier :

perf stat -e rNNN, avec NNN l'identifiant de l'événement
en hexadécimal, umask puis numéro.

Par exemple si umask=03H et event number=D3H, alors
on utilise perf stat -e r3d3
(r pour raw)

"instructions" correspond à rc0


#10/05

Changé les écritures par des lectures dans mon programme.

Dès lors, plus aucune différence de performance entre les
versions. En lecture, une fois les données transférées,
il n'y a plus besoin d'y toucher et il est possible que le 
coût ne soit pas si grand.

Idchire ne fonctionne plus...

#11/05

Idchire ne fonctionne toujours pas. Créé un compte sur 
GRID5000, accepté par fred.

La procédure est la suivante :

ssh ntollenaere@access.grid5000.fr

puis ssh grenoble ( par exemple )

puis oarsub -I pour ouvrir une session interactive.

Plein d'autres options, à voir sur le site 
https://www.grid5000.fr/mediawiki/index.php/Getting_Started

```__kmp_steal_task ``` à la ligne 1683 du fichier kmp_tasking


Le choix du thread où voler les tâches est fait au hasard.
C'est dommage, l'architecture est pourtant décrite. On pourrait
au moins privilégier le coeur, cpu ou noeud courant.

Le choix du thread est fait dans _kmp_execute_task_template,
qui appelle __kmp_steal_task pour faire le choix d'une tâche
en attente dans le thread.

Une fois la tâche choisie, elle est lancée avec __kmp_invoke_task


#12/05

Toujours pas de Idchire...

Suite du rétro-engineering sur le programme.

Tentative de description d'automate faite dans le cahier.

Le master est sans doute chargé de créer chacune des tâches. Il
faudrait voir ça dans kmp_fork_call.

La répartion des tâches entre les threads semble se faire uniquement
grâce au mécanisme de stealing. Il faudrait voir dans le scheduling
comment les emplacements des threads sont choisis.


