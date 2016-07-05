// compile time recursion
template<int N> inline void nop()
{
    nop<N-1>();
    asm("nop");
}

template<> inline void nop<0>() { }

void nops()
{
    nop<1000>();
}
