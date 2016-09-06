open Array
open Printf

type cacheTree = Node of bool * cacheTree * cacheTree | Leaf of (int * float) option  | Empty

let rec ispowof2 nb = match (nb mod 2) with
 | 0 -> if (nb <= 2 ) then true else ispowof2 (nb / 2)
 | 1 -> false
 | _ -> raise(Failure "modulo has failed")

let log2 n = 
  let rec inter n acc =
  if (n == 1) then acc else inter (n / 2) (acc + 1)
  in inter n 0

let decomposeBase2 nb = 
  let rec getMaxPow nb acc = 
    if (acc * 2 > nb) 
      then acc
    else getMaxPow nb (acc * 2)
  in
  let rec inter n l = match n with
    | 0 -> l
    | 1 -> 0::l
    | _ -> let m = getMaxPow n 2 in inter (n - m) ((log2 m)::l)
  in
  inter nb []

  (* simple balanced binary tree *)
  (*
let rec initSimpleTree hght = 
  if (hght == 0) then Leaf None else Node(true, initSimpleTree (hght - 1), initSimpleTree (hght - 1))
  *)

let rec initSimpleTree hght = 
  if (hght == 0) then Leaf None else 
    ( if (hght mod 2 == 0 ) then  Node(true, initSimpleTree (hght - 1), initSimpleTree (hght - 1)
    )
    else Node(false, initSimpleTree (hght - 1), initSimpleTree (hght - 1)
    )

    )
(*search for a value in cache, return Some if success, None if fail*)
let rec getValAtCache addr t = match t with
  | Node(b, left, right) -> 
      (let treeLeft, resLeft = getValAtCache addr left and treeRight, resRight= getValAtCache addr right in
      match (resLeft, resRight) with 
    | Some a, _ -> Node(false, treeLeft, treeRight), Some a 
    | None, Some b -> Node(true, treeLeft, treeRight), Some b
    | None, None -> Node(b, treeLeft, treeRight), None
    )
  | Leaf(Some(a, v)) when (a == addr) -> t, Some v
  | Leaf(_) | Empty -> t, None

  (*insert val in cache in case of miss*)
let rec insertVal addr v tree =  match tree with
  | Empty -> raise(Failure "found an empty leaf")
  | Leaf(_) -> Leaf(Some(addr, v))
  | Node(_, ltree, Empty) -> Node(true, insertVal addr v ltree, Empty)
  | Node(_, Empty, rtree) -> Node(false, Empty, insertVal addr v rtree)
  | Node(true, ltree, rtree) ->  Node(false, insertVal addr v ltree, rtree )
  | Node(false, ltree, rtree) ->  Node(true,  ltree, insertVal addr v rtree )

let getValAt addr tree arr = 
  let newTree, res = getValAtCache addr tree in  match  res with
    | None -> false, arr.(addr), insertVal addr arr.(addr) newTree
    | Some v -> true, v, newTree

let rec loop niter arr cache= 
  let rec inter it arr cache acc = 
  if (it < niter) then 
    (let hit, res, newCache = getValAt (Random.int (Array.length arr)) cache arr in 
      if (hit) then 
        inter (it + 1) arr newCache (acc + 1)
      else
        inter (it + 1) arr newCache acc 
    )
  else
    acc
  in
  inter 0 arr cache 0;;


let () = 
  Random.self_init ();
  let  memArray = Array.make 300 0.0 in
    for i = 0 to (Array.length memArray - 1) do memArray.(i) <- (float_of_int i) done;
  let cache = initSimpleTree 5 in
  printf "%d hits \n" (loop 10000 memArray cache) 





