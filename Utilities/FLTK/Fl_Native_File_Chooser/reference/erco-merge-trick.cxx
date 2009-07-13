#define FLTK1

#include <stdio.h>

#ifdef FLTK1
#define FOO_CLASS Fl_Foo
#else
#define FOO_CLASS fltk::Foo
#endif

#ifdef FLTK1
class Fl_Foo {
#else
namespace fltk {
class Foo {
#endif

public:
    void aaa();
    void bbb();

#ifdef FLTK1
};
#else
};
}
#endif

void FOO_CLASS::aaa() {
   printf("aaa\n");
}

void FOO_CLASS::bbb() {
   printf("bbb\n");
}

#ifdef FLTK1
int main() {
   Fl_Foo foo;
   foo.aaa();
   foo.bbb();
}
#else
int main() {
   fltk::Foo foo;
   foo.aaa();
   foo.bbb();
}
#endif
