#include <cassert>
#include <string>
#include <iostream>
#include <any>

#include "any.hpp"

using utils::any;
using utils::any_cast;
using utils::bad_any_cast;
using std::string;
using std::cout;
using std::cerr;
using std::endl;

void contruct_test() {
    any def;
    any copy_on_type(42);

    def = 3.14;
    def = string("2.71");
    any def_copy(def);
    def = copy_on_type;
    any e;
    assert(e.empty());
}

template<class T>
void check_cast(any& a, bool should_throw) {
    bool thrown = false;
    try {
        double res = any_cast<T>(a);
        std::cout << res << std::endl;
    }
    catch(bad_any_cast const& err) {
        thrown = true;
        std::cerr << err.what() << std::endl;
    }
    assert(should_throw == thrown);
}

void retrieve_value_test() {
    any ia(42);
    auto res = any_cast<double>(&ia);
    assert(res == nullptr);

    auto res2 = any_cast<int>(&ia);
    assert(res2 != nullptr && *res2 == 42);
    
    check_cast<const double&>(ia, true);
    check_cast<int>(ia, false);
    check_cast<int&>(ia, false);
    check_cast<const int&>(ia, false);
}

void swap_test() {
    any a(5), b(string("6"));
    swap(a, b);
    assert(any_cast<string>(a) == "6");
    assert(any_cast<int>(b) == 5);
}

void retrieve_link_test() {
    any a(5);
    any_cast<int&>(a)++;
    assert(any_cast<int>(a) == 6);
    
    any_cast<int&>(a) = 10;
    assert(any_cast<int>(a) == 10);

    any b(any_cast<int&>(a));
    assert(any_cast<int>(a) == any_cast<int>(b));
}

int main() {
    contruct_test();
    retrieve_value_test();
    swap_test();
    retrieve_link_test();
    
    return 0;
} 
