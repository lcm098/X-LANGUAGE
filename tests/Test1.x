#ifndef MATH_UTILS [

    #define MATH_UTILS [()
        pass;
    ]

    #define square [(n)
        return n * n;
    ]

    #define clamp [(val, lo, hi)
        if (val < lo) { return lo; }
        if (val > hi) { return hi; }
        return val;
    ]
]

#ifdef MATH_UTILS [
    print("Already loaded.");
]

{
    let scores = vector[72, 95, 48, 88, 61, 100, 55];
    let total = 0;
    let i = 0;

    while (i < 7) {
        total = total + scores[i];
        i++;
    }

    let avg = total / 7;
    print avg;

    let clamped = @clamp(avg, 60, 90);
    print clamped;

    let sq = @square(5);
    print sq;
}