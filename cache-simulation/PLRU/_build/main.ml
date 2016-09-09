open Array
open Printf

module type Cache = sig
  type  t
  val initSimpleTree : int ->  t
  val insertVal : int -> float ->  t ->  t   
  val getValAtCache : int ->  t ->  t * float option
end

  (*Look for a variable, first in cache, then in memory in case of miss
   * insert new value in cache in case of miss
   * returns a boolean, the searched value and the new cache
   * The boolean is true in case of hit, false in case of miss
   * *)
let getValAt addr tree arr = 
  let newTree, res = PLRU.getValAtCache addr tree  in  match  res with
    | None -> false, arr.(addr), PLRU.insertVal addr arr.(addr) newTree
    | Some v -> true, v, newTree

    (*loop and emulate load niter times
     * *)
let rec loop niter arr = 
  let rec inter it arr cache acc = 
  if (it < niter) then 
    ((* let plru = (module PLRU : Cache) in *)
      let hit, res, newCache = getValAt (it mod (Array.length arr)) cache arr  in 
      if (hit) then 
        inter (it + 1) arr newCache (acc + 1)
      else
        inter (it + 1) arr newCache acc 
    )
  else
    acc
  in
  let cache = PLRU.initSimpleTree 5 in
  inter 0 arr cache 0;;


let () = 
  Random.self_init ();
  let arrlen = if (Array.length Sys.argv > 1) then int_of_string (Sys.argv.(1)) else 100 in
  let  memArray = Array.make arrlen 0.0 in
    for i = 0 to (Array.length memArray - 1) do memArray.(i) <- (float_of_int i) done;
  (*let cache = PLRU.initSimpleTree 5 in*)
  printf "%d hits \n" (loop 10000 memArray ) 





