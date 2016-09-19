
let rec ispowof2 nb = match (nb mod 2) with
 | 0 -> if (nb <= 2 ) then true else ispowof2 (nb / 2)
 | 1 -> false
 | _ -> raise(Failure "modulo has failed")

 (* integer base 2 logarithm *)
let log2 n = 
  let rec inter n acc =
  if (n == 1) then acc else inter (n / 2) (acc + 1)
  in inter n 0

  (*get decomposition in base 2 of a number
   * returns a list of the exponents in decreasing order
   * decomposeBase2 21 returns [4, 2, 0]
   * *)
let decomposeBase2 nb = 
  (* given a number, returns the greatest
   * power of two inferior or equal to it *)
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
