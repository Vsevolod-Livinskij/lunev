#include <iostream>
#include <cmath>
#include <boost/python.hpp>

//g++ -I/usr/include/python2.7 -fPIC -c hello.cpp
//g++ -shared hello.o -lboost_python -o hello.so


double f (double x)
{
    return sin (x);
}

double count(double a, double b, int N) 
{ 
    //const int N = 10000*10000; // количество шагов (уже умноженное на 2)
    double s = 0;
    double h = (b - a) / N;
    for (int i=0; i<=N; ++i) {
        double x = a + h * i;
        s += f(x) * ((i==0 || i==N) ? 1 : ((i&1)==0) ? 2 : 4);
    }
    s *= h / 3;
    return s;
}

BOOST_PYTHON_MODULE(hello) {
using namespace boost::python;
def("count", count); }

