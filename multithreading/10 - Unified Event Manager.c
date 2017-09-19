#include "Defs.h"
#include "UnifiedEventManager.h"

// Compilation:
// gcc -std=gnu99 -pthread -lrt ErrorHandling.c LogF.c UnifiedEventManager.c "10 - Unified Event Manager.c" -o uem

// Task:
// TODO

int main(void)
{
    struct uem_event *e;
    
    mqd_t mqd = 0;
    int pid = getpid();
    
    ec_false( uem_register_process(pid, NULL) )
    ec_false( uem_register_pxmsg(mqd, NULL) )
    
    while (true)
    {
        ec_null( e = uem_wait() )
        
        if (e->ue_errno != 0)
        {
            // Show error message
        }
        else
        {
            switch (e->ue_reg->ur_type)
            {
                case UEM_PXMSG:
                    printf("UEM_PXMSG\n");
                    // Handle received message
                    break;
                case UEM_PROCESS:
                    printf("UEM_PROCESS\n");
                    // Handle process cancellation
                    break;
                default:
                    printf("NOTHING\n");
                    break;
            }
        }
    }
    
    return EXIT_SUCCESS;

EC_CLEANUP_BGN
    return EXIT_FAILURE;
EC_CLEANUP_END
}
