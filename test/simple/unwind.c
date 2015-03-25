#include <stdio.h>
 #include <string.h>
#include <signal.h>
#include <ucontext.h>
#include <libunwind.h>
#include <stdlib.h>
#include <sys/mman.h>

void do_backtrace()
{
    unw_cursor_t    cursor;
    unw_context_t   context;

    unw_getcontext(&context);
    unw_init_local(&cursor, &context);

    while (unw_step(&cursor) > 0)
    {
        unw_word_t  offset, pc, sp;
        char        fname[64];

        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        unw_get_reg(&cursor, UNW_REG_IP, &pc);

        fname[0] = '\0';
        (void) unw_get_proc_name(&cursor, fname, sizeof(fname), &offset);

        printf ("0x%lx : (%s+0x%lx) [0x%lx]\n", pc, fname, offset, sp);
    }
}
void unwind_stack_by_rbp(unsigned long rbp)
{
    while(rbp!=0){
        unsigned long prev_rbp = *(unsigned long *)rbp;
        printf("return address=0x%lx\n", *((unsigned long*)rbp+1));
        rbp = prev_rbp;
    }    
}
#define REG_RBP 10
void segv_handler(int sig, siginfo_t *si, void *puc)
{
    printf("libunwind====>\n");
    do_backtrace();
    printf("libRbp====>\n");
    struct ucontext *uc = (struct ucontext *)puc;
    unsigned long rbp = uc->uc_mcontext.gregs[REG_RBP];
    unsigned long current_rbp;
    __asm__ __volatile__(
            "movq %%rbp, %[mem]\n\t"
            :[mem]"+m"(current_rbp)
            ::
            );
    printf("current_rbp = 0x%lx, segv rbp = 0x%lx\n", current_rbp, rbp);
    //signal stack
    while(current_rbp!=0){
        unsigned long prev_rbp = *(unsigned long *)current_rbp;
        printf("[SIGNAL]return address=0x%lx\n", *((unsigned long*)current_rbp+1));
        current_rbp = prev_rbp;
    }    
    //stack
    unwind_stack_by_rbp(rbp);
    exit(-1);
}

void register_segv()
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGUSR1);
    sa.sa_sigaction = segv_handler;
    if(sigaction(SIGSEGV, &sa, NULL)==-1)
        printf("err!\n");
}

void copy_func(){
    int *addr = (int*)0;
    *addr = 10;
    return ;
}
void end(){;}
typedef void(*func_type)();
int main(int argc, const char *argv[])
{
    register_segv();
    void *start = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_PRIVATE|MAP_ANON, -1, 0);
    memcpy(start, (void*)copy_func, end-copy_func);
   
    func_type func = (func_type)start;
    //func();
    copy_func();
        
    return 0;
}
