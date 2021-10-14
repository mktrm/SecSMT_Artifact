from glob import glob
for fn in glob("*/stat-file.txt"):
    print(fn)
    descriptor, suffix = fn.split("/")
    arch, cryptobench, jsbench = descriptor.split("__")
    print(f"arch: {arch}, cryptobench: {cryptobench}, jsbench: {jsbench}")

