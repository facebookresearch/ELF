#pragma once

#include <selene.h>
#include <vector>

struct Foo {
    int x;
    const int y;
    Foo(int x_) : x(x_), y(3) {}
    int GetX() { return x; }
    int DoubleAdd(int y) {
        return 2 * (x + y);
    }
    void SetX(int x_) {
        x = x_;
    }
};

bool test_register_obj(sel::State &state) {
    Foo foo_instance(1);
    state["foo_instance"].SetObj(foo_instance, "double_add", &Foo::DoubleAdd);
    const int answer = state["foo_instance"]["double_add"](3);
    return answer == 8;
}

bool test_register_obj_member_variable(sel::State &state) {
    Foo foo_instance(1);
    state["foo_instance"].SetObj(foo_instance, "x", &Foo::x);
    state["foo_instance"]["set_x"](3);
    const int answer = state["foo_instance"]["x"]();
    return answer == 3;
}

bool test_register_obj_to_table(sel::State &state) {
    Foo foo1(1);
    Foo foo2(2);
    Foo foo3(3);
    auto foos = state["foos"];
    foos[1].SetObj(foo1, "get_x", &Foo::GetX);
    foos[2].SetObj(foo2, "get_x", &Foo::GetX);
    foos[3].SetObj(foo3, "get_x", &Foo::GetX);
    const int answer = int(foos[1]["get_x"]()) +
        int(foos[2]["get_x"]()) +
        int(foos[3]["get_x"]());
    return answer == 6;
}

bool test_mutate_instance(sel::State &state) {
    Foo foo_instance(1);
    state["foo_instance"].SetObj(foo_instance, "set_x", &Foo::SetX);
    state["foo_instance"]["set_x"](4);
    return foo_instance.x == 4;
}

bool test_multiple_methods(sel::State &state) {
    Foo foo_instance(1);
    state["foo_instance"].SetObj(foo_instance,
                                 "double_add", &Foo::DoubleAdd,
                                 "set_x", &Foo::SetX);
    state["foo_instance"]["set_x"](4);
    const int answer = state["foo_instance"]["double_add"](3);
    return answer == 14;
}

bool test_register_obj_const_member_variable(sel::State &state) {
    Foo foo_instance(1);
    state["foo_instance"].SetObj(foo_instance, "y", &Foo::y);
    const int answer = state["foo_instance"]["y"]();
    state("tmp = foo_instance.set_y == nil");
    return answer == 3 && state["tmp"];
}

bool test_bind_vector_push_back(sel::State &state) {
    std::vector<int> test_vector;
    state["vec"].SetObj(test_vector, "push_back",
                        static_cast<void(std::vector<int>::*)(int&&)>(&std::vector<int>::push_back));
    state["vec"]["push_back"](4);
    return test_vector[0] == 4;
}

bool test_bind_vector_push_back_string(sel::State &state) {
    std::vector<std::string> test_vector;
    state["vec"].SetObj(test_vector, "push_back",
                        static_cast<void(std::vector<std::string>::*)(std::string&&)>(&std::vector<std::string>::push_back));
    state["vec"]["push_back"]("hi");
    return test_vector[0] == "hi";
}

bool test_bind_vector_push_back_foos(sel::State &state) {
    std::vector<Foo> test_vector;
    state["Foo"].SetClass<Foo, int>();
    state["vec"].SetObj(test_vector, "push_back",
                        static_cast<void(std::vector<Foo>::*)(Foo&&)>(&std::vector<Foo>::push_back));
    state["vec"]["push_back"](Foo{1});
    state["vec"]["push_back"](Foo{2});
    return test_vector.size() == 2;
}

struct FooHolder {
    Foo foo;
    FooHolder(int num) : foo(num) {}
    Foo & getRef() {
        return foo;
    }
    Foo * getPtr() {
        return &foo;
    }
    Foo getValue() {
        return foo;
    }
    void acceptFoo(Foo *) {};
};
struct ObjBar{};

bool test_obj_member_return_pointer(sel::State &state) {
    state["Foo"].SetClass<Foo, int>("get", &Foo::GetX);
    FooHolder fh{4};
    state["fh"].SetObj(fh, "get", &FooHolder::getPtr);
    state("foo = fh:get()");
    state("foox = foo:get()");
    return state["foox"] == 4;
}

bool test_obj_member_return_ref(sel::State &state) {
    state["Foo"].SetClass<Foo, int>("get", &Foo::GetX);
    FooHolder fh{4};
    state["fh"].SetObj(fh, "get", &FooHolder::getRef);
    state("foo = fh:get()");
    state("foox = foo:get()");
    return state["foox"] == 4;
}

bool test_obj_member_return_val(sel::State &state) {
    state["Foo"].SetClass<Foo, int>("get", &Foo::GetX);
    FooHolder fh{4};
    state["fh"].SetObj(fh, "get", &FooHolder::getValue);
    state("foo = fh:get()");
    state("foox = foo:get()");
    return state["foox"] == 4;
}

bool test_obj_member_wrong_type(sel::State &state) {
    state["Foo"].SetClass<Foo, int>();
    state["Bar"].SetClass<ObjBar>();
    FooHolder fh{5};
    state["fh"].SetObj(fh, "acceptFoo", &FooHolder::acceptFoo);
    state("bar = Bar.new()");

    bool error_encounted = false;
    state.HandleExceptionsWith([&error_encounted](int, std::string, std::exception_ptr) {
        error_encounted = true;
    });

    state("fh.acceptFoo(bar)");
    return error_encounted;
}
