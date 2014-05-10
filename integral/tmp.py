import hello, argparse, multiprocessing, Queue, threading

def foo(a, b):
    while True:
        try:
            args = task.get(block = False)
            #print args
            #res = hello.count(args [0], args [1])
            resp.put (args)
            task.task_done()
            print "-----------------"
        except:
            break
    print "bubu"
    
def print_thread():
    result = None
    print "haha"
    while True:
        try:
            result += resp.get()
            resp.task_done()
        except:
            break
    print result


if __name__ == "__main__":

    parser = argparse.ArgumentParser (description = "Integral with Simpson's method")
    parser.add_argument("a", help = "start value", type = int)
    parser.add_argument("b", help = "end value", type = int)
    parser.add_argument("thread_num", help = "number of threads", type = int)
    args = parser.parse_args()

    task = Queue.Queue()
    resp = Queue.Queue()

    dec = (args.b - args.a) / (args.thread_num * 4.0)
    prev = args.a
    for i in range (args.thread_num * 4):
        task.put ([prev, prev + dec])
        prev += dec 
        print prev

    for i in range (args.thread_num):
        tmp = multiprocessing.Process(target=foo, args=(0, 3))
        #tmp.daemon = True
        tmp.start()	

    print "0"
    #task.join()
    print "1"
    t = threading.Thread (target = print_thread)
    print "2"
    #t.daemon = True
    print "3"
    t.start()
    print "4"		
    #resp.join()
