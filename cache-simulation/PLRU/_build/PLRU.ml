
type  'a t = Node of bool *  'a t *  'a t | Leaf of (int * 'a) option 


  (* simple balanced binary tree *)
  (*
let rec initSimpleTree hght = 
  if (hght == 0) then Leaf None else Node(true, initSimpleTree (hght - 1), initSimpleTree (hght - 1))
  *)

let rec initSimpleTree hght = 
  if (hght == 0) then Leaf None else 
    ( if (hght mod 2 == 0 ) then  Node(true, initSimpleTree (hght - 1), initSimpleTree (hght - 1))
    else Node(false, initSimpleTree (hght - 1), initSimpleTree (hght - 1))
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
  | Leaf _ -> t, None

  (*insert val in cache in case of miss*)
let rec insertVal addr v tree =  match tree with
  | Leaf(_) -> Leaf(Some(addr, v))
  | Node(true, ltree, rtree) ->  Node(false, insertVal addr v ltree, rtree )
  | Node(false, ltree, rtree) ->  Node(true,  ltree, insertVal addr v rtree )
