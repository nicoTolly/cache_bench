  type  'a t
  val insertVal : int -> 'a -> 'a t -> 'a t   
  val getValAtCache : int -> 'a t -> Array.array -> 'a t * 'a option

