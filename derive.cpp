#include <stdio.h>

class Base {
public:
    Base() {printf("Base\n");}
    virtual ~Base() {printf("~Base\n");}

    void fun1() {printf("Base::fun1\n");}
    virtual void fun2() {printf("Base::fun2\n");}

    void fun3() {fun1(); fun2();}
    virtual void fun4() {fun1(); fun2();}
};

class Derive : public Base {
public:
    Derive() : Base() {printf("Derive\n");}
    ~Derive() {printf("~Derive\n");}

    void fun1() {printf("Derive::fun1\n");}
    virtual void fun2() {printf("Derive::fun2\n");}
    void fun3() {fun1(); fun2();}
    virtual void fun4() {fun1(); fun2();}
};

int main() {
    Base* p = new Derive;
    p->fun1();
    p->fun2();
    p->fun3();
    p->fun4();
    delete p;
    return 0;
}
