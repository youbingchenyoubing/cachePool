#include <iostream>
#include <memory>
class A
{
public:
    void print()
    {
        std::cout << "A::print" << std::endl;
    }
    std::shared_ptr<A> sp( new A );
};
int main()
{
    A* pa = sp.get();
    if ( pa )
    {
        pa->print();
    }
    std::cout << "-> operator: ";
    sp->print();
    std::cout << "* operator: ";
    ( *sp ).print();

    return 0;
}
