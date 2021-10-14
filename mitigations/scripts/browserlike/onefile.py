fn = 'symmetric__-aes-gcm__3d-raytrace.js/stat-file.txt'
d = {}
with open(fn, 'r') as f:
    for line in f:
        if line == "\n":
            print("skipping empty")
            continue
        if "---------- End Simulation Statistics   ----------" in line:
            print("skipping last line")
            continue
        if "---------- Begin Simulation Statistics ----------" in line:
            print("skipping first line")
            continue
        arr = line.split()
        k = arr[0]
        v = arr[1]
        d[k] = v

    print(d)

