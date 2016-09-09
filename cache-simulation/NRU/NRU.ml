type 'a t = (int * 'a) list

let prot = 13

let  discard  lst   = 
  Random.self_init ();
  let rec inter lst acc n k = match lst with
  | [] -> failwith "no element"
  | [a] -> (List.rev acc)
  | t::q ->  if (n < prot) then (inter q (t::acc) (n+1) k) 
            else 
              ( if (k = 0)  then (List.rev acc)@q
              else (inter q (t::acc) n (k - 1)))
  in
  let k = ((Random.int (List.length lst - prot)) + prot) in
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
  | l, None -> res
  | l, Some v->   (addr, v)::l, Some v

