class Op:
    def __init__(self):
        pass
    def badd(self, o1, o2):
        return o1 + o2

def add(v1, v2):
    a = Op()
    return a.badd(v1, v2)

