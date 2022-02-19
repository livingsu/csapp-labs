# lab4 Y86-64 simulator笔记

基本上按照书上走，填好nexti()函数即可。

我犯的错误：

1. compte_cc中，只有加法和减法会设置OF（补码溢出）

```c
/*
 * compute_cc: modify condition codes according to operations 
 * args
 *     op: operations (A_ADD, A_SUB, A_AND, A_XOR)
 *     argA: the first argument 
 *     argB: the second argument
 *     val: the result of operation on argA and argB
 *
 * return
 *     PACK_CC: the final condition codes
 */
cc_t compute_cc(alu_t op, long_t argA, long_t argB, long_t val)
{
    int a = argA, b = argB, v = val;
    bool_t zero = (v == 0);
    bool_t sign = (v < 0);
    bool_t ovf = FALSE;
    
    switch (op)
    {
    case 0: ovf = ((a<0)==(b<0))&&((v<0)!=(a<0)); break;    // add
    case 1: ovf = ((a<0)!=(b<0))&&((v<0)==(b<0)); break;    //sub
    default:
        break;
    }
    return PACK_CC(zero,sign,ovf);
}
```

2. bool_t是枚举类型（int常量），得到的ZF，SF，OF都是bool_t，那么做位运算的时候，结果不是0和1，而是4字节的int。**结果还需要取最后一位！**

   比如，ZF是1，则~ZF=0xffff fffe，由转回bool_t就不对了。

   ```c
   /*
    * cond_doit: whether do (mov or jmp) it?  
    * args
    *     PACK_CC: the current condition codes
    *     cond: conditions (C_YES, C_LE, C_L, C_E, C_NE, C_GE, C_G)
    *
    * return
    *     TRUE: do it
    *     FALSE: not do it
    */
   bool_t cond_doit(cc_t cc, cond_t cond) 
   {
       bool_t zf = GET_ZF(cc);
       bool_t sf = GET_SF(cc);
       bool_t of = GET_OF(cc);
   
       switch (cond){
       case C_YES: return TRUE;
       case C_LE:  return ((sf^of)|zf)&0x1;
       case C_L:   return (sf^of)&0x1;
       case C_E:   return zf&0x1;
       case C_NE:  return (~zf)&0x1;
       case C_GE:  return (~(sf^of))&0x1;
       default:    return (~(sf^of)&~zf)&0x1;
       }
   }
   ```

   

