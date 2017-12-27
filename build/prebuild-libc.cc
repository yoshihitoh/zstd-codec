#include <emscripten/bind.h>
#include <cstdlib>
#include <cstdio>


using namespace emscripten;


class Dummy
{
public:
    Dummy()
        : buffer_(malloc(10))   // for dlmalloc
    {
    }

    ~Dummy()
    {
        if (buffer_ != nullptr) {
            free(buffer_);
        }
    }

private:
    void* buffer_;
};


EMSCRIPTEN_BINDINGS(prewarm) {
    class_<Dummy>("Dummy")
        .constructor<>()
        ;
}


int main(int argc, char** argv)
{
    Dummy dummy;
    printf("HEllo, world\n");
    return 0;
}
