# lab2 bomb笔记

## GDB

打断点：

```shell
break explode_bomb
```

在断点处停止运行：

```shell
kill
```



把答案放在文件a中，在gdb中只需

```
run a
```

即可输入之前的正确答案。



## start

反汇编二进制文件bomb，将汇编代码输出到assembly文件：

```shell
objdump -d bomb >assembly
```



找到main函数，里面包含了6个phase

## phase 1

![image-20220217193458326](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220217193458326.png)

调用了strings_not_equal这个函数，如果结果为0，调用explode_bomb函数。所以关键是strings_not_equal(a,b)

通过断点，发现a是自己输入的字符串，b是0x402400，用

```shell
print (char*) $esi
```

得到结果是

```
Border relations with Canada have never been better.
```

## phase 2

![image-20220217203450601](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220217203450601.png)

首先是read_six_numbers(string s, $rsp)有两个参数。

![image-20220217203610935](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220217203610935.png)

发现里面调用了sscanf(str,format,...)库函数，用法是：将str按照format的格式，赋值给后面的变量，返回成功的数量。

```c
sscanf("12 23","%d %d",a,b);
```

结合函数名，猜想函数作用：读取str，并赋值给6个数（放在栈上）。

在400f0a处打断点，并输出rsp：

![image-20220217204204087](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220217204204087.png)

猜想正确。



然后仔细观察后面的代码。

![image-20220217204304916](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220217204304916.png)

400f0a：说明第一个数是1

400f30：rbx是第二个数

400f35：rbp是第七个数（超出边界）

400f17：eax=rbx前一个数

400f1c：比较rbx和2*eax的值。

可以看出，就是一个等比数列。

故答案是"1 2 4 8 16 32"

## phase 3

![image-20220217205926869](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220217205926869.png)

sscanf是加载两个十进制数"%d %d"，到0x8(rsp)和0xc(rsp)中。

400f75：显然是一个switch，通过断点发现正好是和下标一一对应的，故第二个数就是0xcf=207

答案："0 207"，也可以是其他下标，比如1对应的是0x2c3

## phase 4

![image-20220217215848662](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220217215848662.png)

需要输入两个整数m,n，其中m需要满足：func4(m,0,0xe)==0，

401051：说明n=0

将func4逐行翻译：（sar reg表示右移1位）

```c++
int f(int a,int b,int c){
    int m=c;	// eax
    m=c-b;
    int n=m;	// ecx
    n>>=31; //logical shift
    m+=n;
    m>>=1;
    n=m+b;
    
    if(n<=a){
        m=0;
        if(n>=a){
            return m;
        }
        m=f(a,1+n,c);
        m=1+2m;
        return m;
    }else{
        c=n-1;
        m=f(a,b,c);
        m=2m;
        return m;
    }
}
```

精简为:

```c++
int f(int a,int b,int c){
    int n=(((c-b)+(c-b)>>31)>>1)+b;
    
    if(n==a){
        return 0;
    }else if(n<a){
        return 1+2*f(a,n+1,c);
    }else{
        return 2*f(a,b,n-1);
    }
}
```

要使	`f(x,0,0xe)=0`	成立，求x

```
n=7
```

故x=7，答案是"7 0"

## phase 5

![image-20220218101109246](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220218101109246.png)

string_length字符串长度是6

![image-20220218101158361](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220218101158361.png)

这里是一个循环，rbx是输入字符串（str），循环6次，将每个循环的字符，和0xF and以后作为下标，取0x4024b0数组的元素，重新组成字符串，要等于0x40245e这里的字符串。

数组0x4024b0：

![image-20220218100700986](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220218100700986.png)

比较的字符串：（有6位）

![image-20220218100746096](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220218100746096.png)

在数组中，下标为9 f e 5 6 7

对应ASCII码可以是：0x39 0x4f 0x4e 0x35 0x36 0x37，即9ON567

故答案是9ON567

## phase 6

需要分段解读。做法：将jmp的地方注上标号（比如.L1，.L2等），就可以看出控制的逻辑。

### 6.1

read_six_number读取6个数。

```
r13=rsp
rsi=rsp
call read_six_number() => rsp 0~5
r14=rsp
r12=0

.L3 (401114)

rbp=r13
rax=r13[0]		// =rsp[i]
rax--			// =rsp[i]-1
if(rax>5) bomb()	// 每个数都是1~6
r12++		// 下标i,0~5
if(r12==6) goto .L1		// 只有此处能跳出循环
rbx=r12		// j=i+1

.L2 (401135)
rax=rbx
rax=rsp[rax]	// =rsp[j]
if(rbp[0]==rax) bomb()	// rsp[i]!=rsp[j]
rbx++	// j++
if(rbx<=5) goto .L2

r13+=4
goto .L3

.L1 (401153)
```

.L3~.L1说明，六个数都应该是1~6，且互相不同。

在.L1打断点，输入0 1 2 3 4 5、1 2 3 4 5 7、1 1 2 3 4 5均不能到达断点，说明结论正确：**六个数是1~6的排列**。

一共有$A_6^6=720$ 种排列，猜的速度太慢。



### 6.2

```
.L1 (401153)
rsi=rsp+0x18	// 0x18=24，rsi是rsp[5]后面的地址
rax=r14 // =rsp
rcx=7

.L4 (401160)
rdx=rcx
rdx-=(rax)
(rax)=rdx	// rsp[0]=rcx-rsp[0]
rax+=4
if(rax!=rsi) goto .L4
rsi=0
goto .L5

.L8 (401176)
rdx=(rdx+0x8)
rax++
if(rax!=rcx) goto .L8
goto .L9

.L6 (401183)
rdx=0x6032d0

.L9 (401188)
(0x20+rsp+2*rsi)=rdx
rsi+=4
if(rsi==0x18) goto .L7	// 只有此处能跳出循环

.L5 (401197)
rcx=(rsp+rsi)	// =rsp[i]
if(rcx<=1) goto .L6
rax=1
rdx=0x6032d0
goto .L8

.L7 (4011ab)
```

**.L4：rsp[i]=7-rsp[i]，i：0~5**

例：1 2 3 4 5 6变为6 5 4 3 2 1



打印0x6032d0：是一个二叉树，且只有左节点。根节点是0x6032d0

![image-20220218132656677](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220218132656677.png)

```c++
struct node{
    int v1;
    int v2;
    node* left;
    node* right;
}
```



**.L8~.L7：从根节点移动rsp[i]-1次（rsp[i]<=1时不移动），将节点值放入(0x20+rsp)[i]中（8个byte为单位）**

打印后发现结果确实如此：

![image-20220218133120222](C:\Users\17538\AppData\Roaming\Typora\typora-user-images\image-20220218133120222.png)

### 6.3

设rsp+0x20是数组arr

```
.L7 (4011ab)
rbx=(rsp+0x20)	// arr[0]
rax=rsp+0x28	// arr+1
rsi=rsp+0x50	// arr+6
rcx=rbx	// arr[0]

.L11 (4011bd)
rdx=(rax)
(rcx+0x8)=rdx
rax+=0x8
if(rax==rsi) goto .L10
rcx=rdx
goto .L11

.L10 (4011d2)
```

核心：(rcx+0x8)=rdx	**表示arr[i].left=arr[i+1]**

运行：

```
.L11
rdx=arr[1]
arr[0].left=rdx
rax=arr+2
rcx=arr[1]

.L1
...
```

### 6.4

```
.L10 (4011d2)
(8+rdx)=0
rbp=5

.L12 (4011df)
rax=(8+rbx)
rax=(rax)
if((rbx)<rax) bomb()

rbx=(rbx+8)
rbp--
if(rbp!=0) goto .L12

ret
```



运行：

```
rbx=arr[0]
.L12
rax=rbx.left
rax=rax.leftVal
if(rax>rbx.leftVal) bomb()
rbx=arr[0].left
rbp=4
```

**意思：从arr[0]出发，左节点的v1必须小于等于该节点的v1**

```c++
node* b=arr[0];
int i=5;
do{
    node* a=b.left;
    if(a.v1>b.v1) bomb();
    b=b.left;
    i--;
}while(i!=0)
```

结合6.3的结论，arr数组的节点的v1的值，应该是从大到小的。则arr数组是：node3，node4，node5，node6，node1，node2，则答案就是"4 3 2 1 6 5"（6.2那里发生转换）

