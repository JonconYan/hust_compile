int main()
{
    int x[10], i;
    int temp;
    for (i = 0; i < 10; i++)
        x[i] = read();
    for (i = 0; i < 10; i++)
    {
        temp = x[i];
        write(temp);
    }    
}