import sys

def main():
    inp = sys.stdin.read()
    lines = inp.split('\n')
    stats = [x for x in lines if "client" not in x]
    loss = [x for x in lines if "lost" in x]
    tmp = loss[0].split()
    lostpackets = float(tmp[2])
    allpackets = float(tmp[5])
    print (' & '.join(['%.3f' % round(float(x[2]), 3) for x in [x.split() for x in stats] if len(x) > 1]), "& ", lostpackets/allpackets, " \\\\");

if __name__ == '__main__':
    main()
