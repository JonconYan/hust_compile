int main()
{
    int x[10], i;
    for (i = 0; i < 10; i++)
        x[i] = read();
    return 1;
}

int main()  
{
    int x[10][20],i,j=1;
    for(i=0;i<10;i++)
        x[i][j]=read();
    return 1;
}

int fac(int a)
{
    if (a==1) return 1;
    return a*fac(a-1);
}
int main()
{
    int x;
    x=read();
    write(fac(x));
   return 1;
}

int main()
{
   int a,b,c;
   a=10;b=20;c=30; 
    c=++a + ++a+ b++ +b++;
    c=a==b || a>10 && b<20;
    return 1;
}

int main()
{
    int a,b,c,m;
    while (a<b)
        {
         if (a<b) continue;
         for(c=1;c<10;c++)
               if (c<5) continue;
               else break;      
         }
    return 1;
}

int main()
{
    int a,b,s;
    a=0;
    s=0;
    while(a<6)
          {
           a=a+1;
           if ((b=a/2)*2==a) continue;
           s+=a;
           if (a==10) break;      
         }
    write(s);
    return 1;
}

