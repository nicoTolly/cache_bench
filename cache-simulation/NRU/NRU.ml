type 'a t = (int * 'a) list

let prot = 0

let rec append l1 l2 = match l1 with
  [] -> l2
  | t::q -> append q (t::l2)


(*given a cache, discard a value 
 * chosen randomly among the oldest value
 * returns the updated cache*)
let  discard  lst   = 
  Random.self_init ();
  (* returns a list without the kth element *)
  let rec discardElmt q k acc = match q, k with
    | [], 0 -> List.rev acc
    | hd::tl, 0 -> append acc tl
    | hd::tl, _ -> discardElmt tl (k - 1) (hd::acc)
    | [], _ -> Printf.printf "k = %d \n" k; failwith "could not discard object" in
  let rec inter lst acc n k = match lst with
    | [] -> failwith "no element"
    | [a] -> (List.rev acc)
    | t::q ->  if (n < prot) then (inter q (t::acc) (n+1) k) 
            else 
              (*( if (k = 0)  then (List.rev acc)@q
              else (inter q (t::acc) n (k - 1)))*)
              (discardElmt lst k acc)
  in
  let k = ((Random.int (List.length lst - prot))  ) in
  inter lst [] 0 k
  
let initCache () = []

let insertVal addr value cache  = 
  if (List.length cache >= 32) then 
    (addr, value)::(discard cache) 
  else
    ((addr, value)::cache)

let getValAtCache addr cache = 
  let rec inter addr lst acc = match lst with
    | [] ->  List.rev acc, None
    | (k, v)::q when (k = addr) ->  (List.rev acc)@q,Some v
    | t::q -> inter addr q (t::acc) in
  let res = inter addr cache [] in match res with
    | l, None  -> res
    | l, Some v->   (addr, v)::l, Some v

