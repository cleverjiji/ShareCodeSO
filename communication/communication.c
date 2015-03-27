#include "communication.h"
#include "utility.h"
#include "wrapper.h"
#include <signal.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sched.h>


COMMUNICATION_INFO *main_info = NULL;

struct sigaction client_sigusr1;
BOOL sigusr1_used = false;

void sigusr1_handler(INT32 sig, siginfo_t *si, void *puc)
{
	SC_INFO("recieve sigusr1 signal!\n");
	ASSERT((sig==SIGUSR1) && (main_info->flag==0));
	struct ucontext *uc = (struct ucontext *)puc;
	UINT64 current_rbp;
	__asm__ __volatile__(
		   "movq %%rbp, %[mem]\n\t"
		   :[mem]"+m"(current_rbp)
		   ::"cc"
		   );

	main_info->origin_rbp = current_rbp;
	main_info->origin_uc = uc;
	main_info->process_id = getpid();
	main_info->flag = 1;
	
	while(main_info->flag==1)
		sched_yield();

	return ;
}

void init_communication()
{
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = sigusr1_handler;
	//shield all sets
	//sigfillset(&sa.sa_mask);
	if(sigaction(SIGUSR1, &sa, NULL)==-1){
		SC_ERR("sigaction error!\n");
	}
}

typedef void (*sighandler_t)(int);
sighandler_t wrapper_signal(int signum, sighandler_t handler) {
	if(signum == SIGUSR1) {
		//SC_INFO("client sigusr1 registered!\n");
		client_sigusr1.sa_handler = handler;
		return 0;
	}else{
		sighandler_t (*real_signal)(int signum, sighandler_t handler) = \
			(sighandler_t(*)(int signum, sighandler_t handler))dlsym(RTLD_NEXT, "signal");
		ASSERT(real_signal);
		return real_signal(signum, handler);
	}
}

int wrapper_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
	if(signum == SIGUSR1) {
		if (sigusr1_used) { //sigusr1 is registered by client application
			//SC_INFO("client sigaction registered!\n");
			client_sigusr1.sa_handler = act->sa_handler;
			client_sigusr1.sa_sigaction = act->sa_sigaction;
			client_sigusr1.sa_mask = act->sa_mask;
			client_sigusr1.sa_flags = act->sa_flags;

			return 0;
		}else {
			//SC_INFO("so sigaction registered!\n");
			
			sigusr1_used = true;
		}
	}
	
	int (* real_sigaction)(int signum, const struct sigaction *act, struct sigaction *old_act) =\
		 (int(*)(int signum, const struct sigaction *act, struct sigaction *oldact))dlsym(RTLD_NEXT, "sigaction");
	ASSERT(real_sigaction);
	return real_sigaction(signum, act, oldact);
}

