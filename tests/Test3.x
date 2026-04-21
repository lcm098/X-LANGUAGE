

let i = 0;

label repeat [
    if(i < 10) {

        print(i);
        ++i;
        jump repeat;
    }
]

if(i < 10) {
    jump repeat;
} else {
    pass;
}