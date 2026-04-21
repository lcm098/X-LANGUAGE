
for(var x =1; x <= 50; x++)
{
    if(x == 30) {
        break;
    } else {
        if(x % 2 != 0) {
            continue;
        }
        print(x);
    }
}