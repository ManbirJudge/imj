import math

def NormCoeff(n):
    if n == 0:
        return 1.0 / math.sqrt(2.0)
    else:
        return 1.0

precision = 8

idct_table = [
    [
        NormCoeff(u) * math.cos(((2.0 * x + 1.0) * u * math.pi) / 16.0)
        for x in range(precision)
    ]
    for u in range(precision)
]

print(idct_table)