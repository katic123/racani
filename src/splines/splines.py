import math

def write(name, pts):
    f = open(name, "w")
    for p in pts:
        f.write("%.3f %.3f %.3f\n" % (p[0], p[1], p[2]))
    f.close()

# helix: radius 10, rises 40 units over 2 turns
pts = []
n = 32
for i in range(n + 1):
    t = 4 * math.pi * i / n
    pts.append((20 + 10 * math.cos(t), 20 + 10 * math.sin(t), 2 + 40 * i / n))
write("spline_helix.txt", pts)

# trefoil knot (closed, so repeat the first 3 points at the end)
pts = []
n = 36
for i in range(n):
    t = 2 * math.pi * i / n
    x = math.sin(t) + 2 * math.sin(2 * t)
    y = math.cos(t) - 2 * math.cos(2 * t)
    z = -math.sin(3 * t)
    pts.append((20 + 5 * x, 20 + 5 * y, 20 + 5 * z))
for i in range(3):
    pts.append(pts[i])
write("spline_trefoil.txt", pts)

# loop-the-loop in the x/height plane, with a slight sideways drift
pts = []
pts.append((0, 0, 2))
pts.append((8, 0, 2))
n = 24
for i in range(n + 1):
    t = 2 * math.pi * i / n
    pts.append((16 + 8 * math.sin(t) + 8 * t / (2 * math.pi), 6 * t / (2 * math.pi), 10 - 8 * math.cos(t)))
pts.append((40, 8, 2))
pts.append((50, 8, 2))
write("spline_loop.txt", pts)
