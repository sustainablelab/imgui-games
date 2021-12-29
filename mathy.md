Super simple way to think about if a point is on a line or not on
a line segment.

# Point

This is the point:

    P : x,y

Represent points as a `Vec2`. This is a struct. Initialize like
this:

    Vec2{-1.f, -1.f}

- screen center is `Vec2{0,0}`
- coordinates are normalized
- up and right is positive

So point `Vec2{-1.f, -1.f}` is the bottom left corner of the
screen.

# Line segment

This is the line segment. It's defined by two points:

    A : x1,y1
    B : x2,y2

This is a line segment (two points):

    Vec2{-1.f, -1.f}
    Vec2{+1.f, -1.f}

That's a horizontal line at the bottom of the screen.

# Point and line segment

The following is true if the point lies on the line passing
through A and B.

    P = A + λ(B-A)

And the point lies on the line segment (i.e., it lies on the line
and is between points A and B) if λ is between 0 and 1.

Be clear in the implementation of the arithmetic operations by
writing out the points as `{x,y}`:

    P = A + λ(B-A)
    {x,y} = {x1,y1} + λ{x2-x1,y2-y1}

Manipulate this to get the **affine combination** form:

    P = A + λB - λA
    P = A - λA + λB
    P = (1-λ)A + λB
    {x,y} = (1-λ){x1,y1} + λ{x2,y2}

The **affine combination** form says that P is a linear
combination of A and B where the coefficients (λ and 1-λ) add up
to 1.

Actually, the original form is more useful:

    P = A + λ(B-A)

Just move A to the other side to get the **scaled vector** form:

    P-A = λ(B-A)
    {x-x1,y-y1} = λ{x2-x1,y2-y1}
    {x-x1,y-y1} = {λx2-λx1,λy2-λy1}

That says that I can think of AP and AB as vectors where A is the
tail of both vectors, P is the head of one vector, and B is the
head of the other vector.

That says P is on the line passing through AB if
AP is a scaled version of AB. And P is on the line segment (it is
between A and B) if λ is between 0 and 1. For example,
considering the extremes: P is A when λ is 0, and P is B when λ
is 1.
