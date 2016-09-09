  type  'a t 
  val initCache : unit ->  'a t
  val getValAtCache : int ->  'a t ->  'a t * 'a option
  val insertVal : int -> 'a ->  'a t ->  'a t
