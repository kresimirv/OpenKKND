#include "src/Coroutine.h"

#include <cstdlib>
#include <iterator>

#if defined(__GNUC__)
#include <cstdint>
#include <ucontext.h>
#include <map>
static std::map<Coroutine*, ucontext_t> g_coro_ctx;
#endif

Coroutine::Coroutine() 
{
    static int _id = 0;

    id = _id++;
    yield_to = nullptr;
    context = nullptr;
    stack = 0;
    next_depricated = nullptr; // next = nullptr;
    debug_handler_name = nullptr;
}

Coroutine::~Coroutine()
{
}
std::list<Coroutine*> coroutine_list;
Coroutine *volatile coroutine_current = nullptr;

#if defined(__GNUC__)
intptr_t coroutine_current_stack = 0;
#else
int coroutine_current_stack = 0;
#endif

// alloc coroutine list
bool coroutine_list_alloc() 
{
    //add one empty coroutine to list
    Coroutine *c = new Coroutine();
    coroutine_list.push_back(c);
   
    return true;
}

// release all coroutine list elements and remove them from list
void coroutine_list_free() 
{
    for (auto coroutine : coroutine_list) {
        coroutine_list_clear(coroutine);
    }
    coroutine_list.clear();
}

// release coroutine resource and remove it from list
void coroutine_list_remove(Coroutine *coroutine)
{
    if (coroutine == nullptr) {
        return;
    }

    if (coroutine->context) {
        free(coroutine->context);
    }

#if defined(__GNUC__)
    g_coro_ctx.erase(coroutine);
#endif

    coroutine_list.remove(coroutine);
}

// release coroutine resource
void coroutine_list_clear(Coroutine *coroutine)
{
    if (coroutine == nullptr) {
        return;
    }

    if (coroutine->context) {
        free(coroutine->context);
    }

#if defined(__GNUC__)
    g_coro_ctx.erase(coroutine);
#endif
}

// get first element in coroutine list
Coroutine *coroutine_list_get_head()
{
    if (coroutine_list.empty())
    {
        return nullptr;
    }

    return *coroutine_list.begin();
}

void nullsub() {}

#if defined(__GNUC__)

static ucontext_t g_main_ctx;
static bool g_main_ctx_ready = false;

static ucontext_t *get_main_ctx() {
    if (!g_main_ctx_ready) {
        getcontext(&g_main_ctx);
        g_main_ctx_ready = true;
    }
    return &g_main_ctx;
}

int Coroutine::resume() 
{
    auto it = g_coro_ctx.find(this);
    if (it == g_coro_ctx.end()) {
        // Switching to idle/main coroutine -> return to main context
        Coroutine *prev = coroutine_current;
        if (prev && g_coro_ctx.count(prev)) {
            this->yield_to = prev;
            coroutine_current = this;
            swapcontext(&g_coro_ctx[prev], get_main_ctx());
        }
        return 0;
    }

    // Switching to a script coroutine
    Coroutine *prev = coroutine_current;
    this->yield_to = prev;
    coroutine_current = this;

    ucontext_t *save = (prev && g_coro_ctx.count(prev)) ? &g_coro_ctx[prev] : get_main_ctx();
    swapcontext(save, &it->second);
    return 0;
}

Coroutine *couroutine_create(void(*function)(), const char *debug_handler_name)
{
    Coroutine *coroutine = new Coroutine();
    if (coroutine == nullptr) {
        return nullptr;
    }
    coroutine_list.push_back(coroutine);

    size_t stack_size = 1048576;
    void *stack = malloc(stack_size);
    if (stack == nullptr) {
        return nullptr;
    }

    ucontext_t ctx;
    getcontext(&ctx);
    ctx.uc_stack.ss_sp = stack;
    ctx.uc_stack.ss_size = stack_size;
    ctx.uc_stack.ss_flags = 0;
    ctx.uc_link = nullptr;
    makecontext(&ctx, function, 0);

    coroutine->context = (int *)stack;
    coroutine->stack = (intptr_t)stack;
    coroutine->debug_handler_name = debug_handler_name;

    g_coro_ctx[coroutine] = ctx;
    return coroutine;
}

#else // MSVC original implementation

Coroutine *couroutine_create(void(*function)(), const char *debug_handler_name)
{
    Coroutine *coroutine = new Coroutine();
    if (coroutine == nullptr) {
        return nullptr;
    }
    coroutine_list.push_back(coroutine);

    size_t stack_size = 1048576;
    int *result = (int *)malloc(stack_size);
    if (result == nullptr) {
        return nullptr;
    }

    coroutine->context = result;
    coroutine->debug_handler_name = debug_handler_name;
    int SP = stack_size >> 2;
    coroutine->context[SP - 1] = (int)nullsub;
    coroutine->context[SP - 2] = 0;
    coroutine->context[SP - 3] = (int)function;
    coroutine->context[SP - 4] = (int)&coroutine->context[SP - 4];
    coroutine->stack = (int)&coroutine->context[SP - 7];

    return coroutine;
}

__declspec(naked) int coroutine_yield_asm(Coroutine *self) 
{
  __asm {
    push    ebp
    mov     ebp, esp
    push    ebx
    push    esi
    push    edi
    mov     coroutine_current_stack, esp
  }
  coroutine_current->stack = coroutine_current_stack;
  self->yield_to = coroutine_current;
  coroutine_current = self;
  coroutine_current_stack = self->stack;
  __asm {
    mov     esp, coroutine_current_stack
    xor     eax, eax
    pop     edi
    pop     esi
    pop     ebx
    pop     ebp
    retn
  }
}

int Coroutine::resume() 
{
  return coroutine_yield_asm(this);
}

#endif
