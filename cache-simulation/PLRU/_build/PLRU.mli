  type  'a t 
  val initSimpleTree : int ->  'a t
  val getValAtCache : int ->  'a t ->  'a t * 'a option
  val insertVal : int -> 'a ->  'a t ->  'a t
