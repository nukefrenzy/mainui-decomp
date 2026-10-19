// Instantiates common libstdc++ templates so tusplit.py can recognize their compiled copies in
// MainUI: at -O0 every instantiation is a separate COMDAT function placed after its file's .text,
// so runs of them mark the tail of a translation unit. vector<T*> compiles the same for every T
// (only link-time words differ), so one pointer type covers all of them.
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

std::string g_probe("x");   // gives the file a static-init pair, like MainUI's own files

template <class T> void use(std::vector<T> &v, const T &x) {
    v.push_back(x);
    v.push_back(T(x));
    v.insert(v.begin(), x);
    v.erase(v.begin());
    v.reserve(4);
    v.resize(2);
    T a = v[0] = v.front() = v.back();
    (void)a;
    for (typename std::vector<T>::iterator it = v.begin(); it != v.end(); ++it) { *it = x; }
    v.pop_back();
    v.size(); v.empty(); v.clear();
    std::vector<T> w(v);
    w = v;
}

void probe(int *p, int i, std::string s, char *c) {
    std::vector<int *> a; use(a, p);
    std::vector<int> b; use(b, i);
    std::vector<std::string> d; use(d, s);
    std::vector<char *> e; use(e, c);
    std::vector<char> f; use(f, *c);
    std::vector<unsigned char> g; use(g, (unsigned char)*c);
}
