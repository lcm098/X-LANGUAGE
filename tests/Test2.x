
/*
impl sum(num1, num2, num3=3) -> {

    return num1 + num2 + num3;
}

print(sum(num1 = 3, num2 = 4));
*/

// closer

/*
impl makeCounter() -> {
    let count = 0;
    return impl() -> {
        count = count + 1;
        return count;
    };
}

let c = makeCounter();
print c();
print c();
*/


impl sum(callback, x, y) -> {
    print(callback(a=x, b=y));
}

sum(callback = impl (a, b) -> { return a+b; }, x=5, y=6);