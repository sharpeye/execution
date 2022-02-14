#include <execution/just.h>
#include <execution/let_value.h>
#include <execution/then.h>
#include <execution/null_receiver.h>

#include <iostream>
#include <string>
#include <memory>

using namespace std::string_literals;
using namespace NExecution;

///////////////////////////////////////////////////////////////////////////////

template <typename T>
void DoPrint(T&& value)
{
    std::cout << std::forward<T>(value);
}

template <typename T>
void DoPrint(std::unique_ptr<T>&& value)
{
    std::cout << std::forward<T>(*value);
}

void Print()
{}

template <typename T>
void Print(T&& v)
{
    DoPrint(std::forward<T>(v));
}

template <typename U, typename V, typename ... Ts>
void Print(U&& v1, V&& v2, Ts&& ... values)
{
    Print(std::forward<U>(v1));
    std::cout << " ";
    Print(std::forward<V>(v2), std::forward<Ts>(values)...);
}

///////////////////////////////////////////////////////////////////////////////

int main()
{
    auto s = Just(1)
        | LetValue([] (int x) {
            return Just(x, x + 1, x + 2);
        })
        | LetValue([] (int a, int b, int c) {
            return Just(a + b + c, 42);
        })
        | Then([] (auto&&... values) {
            std::cout << "=== [OnSuccess] ===\n";
            Print(std::forward<decltype(values)>(values)...);
            std::cout << std::endl;

            return 100500;
        })
        | Then([] (auto&&... values) {
            std::cout << "=== [OnSuccess] ===\n";
            Print(std::forward<decltype(values)>(values)...);
            std::cout << std::endl;
        });

    std::cout << "size: " << sizeof(s) << std::endl;

    auto op = s.Connect(TNullReceiver());
    op.Start();

    return 0;
}
