p = 20
import b
from b import add
from typing import List

class A_B:
    v : int
    def __init__(self):
        self.v = 0
    def getv(self):
        return self.v

class A_G(A_B):
    def __init__(self):
        super(A_G)
        self.v = 0

    def setv(self, v):
        self.v = v

    def getv(self):
        return self.v


def pack_f(v: List, k, u="u", *kargs, **kwargs):
    v1 = v[0]
    def f(x):
        a_z = v1 + x + p
        a_w = A_G()
        a_w.setv(add(a_z, x))
        return a_w
    return f

def add5(a, b, p=[2,3,4], u="str", c=3, d=4, e=5):
    return a+b+c+d+e

def main():
    global v
    v = []
    v.append(0)
    v[0] = 10
    f = pack_f(v, 1, 2, 3, 4, 5, 6)
    print(f(3).getv())
    print(__file__)

    tms = b.Op()
    print(type(tms))

    add5(1,2)

    p = A_G()  # type: unsigned
    p.setv(3)
    print(p.getv())


if __name__ == "__main__":
    main()
