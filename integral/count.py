import hello, argparse, multiprocessing, Queue, threading

def foo(task):
    res = hello.count(task [0], task [1], task [2])
    return res


if __name__ == '__main__':
    parser = argparse.ArgumentParser (description = "Integral with Simpson's method")
    parser.add_argument("a", help = "start value", type = int)
    parser.add_argument("b", help = "end value", type = int)
    parser.add_argument("N", help = "step number", type = int)
    parser.add_argument("thread_num", help = "number of threads", type = int)
    args = parser.parse_args()

    dec = (args.b - args.a) / (args.thread_num * 1.0)
    prev = args.a
    task = []
    for i in range (args.thread_num):
        task.append (prev)
        prev += dec 
    task.append (prev)

    po = multiprocessing.Pool()
    res = po.map_async(foo,((task [i], task [i+1], args.N / args.thread_num) for i in range(args.thread_num)))
    w = sum(res.get())
    print w
