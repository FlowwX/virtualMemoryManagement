/* Description: Memory Manager BSP3*/
/* Prof. Dr. Wolfgang Fohl, HAW Hamburg */
/* Winter 2016
 * 
 * This is the memory manager process that
 * works together with the vmaccess process to
 * mimic virtual memory management.
 *
 * The memory manager process will be invoked
 * via a SIGUSR1 signal. It maintains the page table
 * and provides the data pages in shared memory
 *
 * This process is initiating the shared memory, so
 * it has to be started prior to the vmaccess process
 *
 * TODO:
 * currently nothing
 * */

#include "mmanage.h"

struct vmem_struct *vmem = NULL;
FILE *pagefile = NULL;
FILE *logfile = NULL;
int signal_number = 0;          /* Received signal */
int vmem_algo = VMEM_ALGO_FIFO;

int (*algorithm) (void);

struct pf_entry
{
    int pt_id;
    int data[VMEM_PAGESIZE];
};

int
main(int argc, char** argv)
{
    struct sigaction sigact;

    /* Init pagefile */
    init_pagefile(MMANAGE_PFNAME);
    if(!pagefile) {
        perror("Error creating pagefile");
        exit(EXIT_FAILURE);
    }

    /* Open logfile */
    logfile = fopen(MMANAGE_LOGFNAME, "w");
    if(!logfile) {
        perror("Error creating logfile");
        exit(EXIT_FAILURE);
    }

    /* Create shared memory and init vmem structure */
    vmem_init();
    if(!vmem) {
        perror("Error initialising vmem");
        exit(EXIT_FAILURE);
    }
    else {
       PDEBUG("vmem successfully created\n");
    }

    /* Setup signal handler */
    /* Handler for USR1 */
    sigact.sa_handler = sighandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    if(sigaction(SIGUSR1, &sigact, NULL) == -1) {
        perror("Error installing signal handler for USR1");
        exit(EXIT_FAILURE);
    }
    else {
       PDEBUG("USR1 handler successfully installed\n")
    }

    if(sigaction(SIGUSR2, &sigact, NULL) == -1) {
        perror("Error installing signal handler for USR2");
        exit(EXIT_FAILURE);
    }
    else {
        PDEBUG("USR2 handler successfully installed\n");
    }

    if(sigaction(SIGINT, &sigact, NULL) == -1) {
        perror("Error installing signal handler for INT");
        exit(EXIT_FAILURE);
    }
    else {
        PDEBUG("INT handler successfully installed\n");
    }

    /* Signal processing loop */
    while(1) {
        signal_number = 0;
       
        pause();
        if(signal_number == SIGUSR1) {  /* Page fault */
            PDEBUG("Processed SIGUSR1\n");
            signal_number = 0;
        }
        else if(signal_number == SIGUSR2) {     /* PT dump */
            PDEBUG("Processed SIGUSR2\n");
            signal_number = 0;
        }
        else if(signal_number == SIGINT) {
            //PDEBUG(stderr, "Processed SIGINT\n");
            PDEBUG("Processed SIGINT\n");
        }
        
    }



    return 0;
}





void
init_pagefile(const char *pfname){
    pagefile = fopen(pfname, "w+");

    //init page file structure
    int i;
    for(i=0;i<VMEM_NPAGES; i++){
        struct pf_entry entry = {i};
        fwrite(&entry, sizeof(struct pf_entry), 1, pagefile);
    }

}

void
vmem_init(){

    int fd = shm_open(SHMNAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR );
    if (fd == -1){
        perror("Can't create shared memory");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, 0) == -1){
        perror("Can't set size of shared memory to zero");
        exit(EXIT_FAILURE);
    }


    if (ftruncate(fd, SHMSIZE) == -1){
        perror("Can't set size of shared memory");
        exit(EXIT_FAILURE);
    }

    vmem = mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    //init admin data
    vmem->adm.size = sizeof(*vmem);
    vmem->adm.mmanage_pid = getpid();

    //init pagetable
    int i, flag;
    for(i=0; i<VMEM_NPAGES;i++){
        flag=0;
        vmem->pt.entries[i].frame = VOID_IDX;
        if(i<VMEM_NFRAMES){
            flag = PTF_PRESENT;
            vmem->pt.framepage[i] = i;
            vmem->pt.entries[i].frame = i;
        }       
        vmem->pt.entries[i].flags = flag;
        
        vmem->pt.entries[i].count = 0;
    }

    //init data
    for(i=0; i<(VMEM_NFRAMES * VMEM_PAGESIZE); i++){
        vmem->data[i] = VOID_IDX;
    }
    
}

void
dump_pt(){
    int i,k;
    printf("\n\nStruct:vmem_adm_struct:START\n");
        printf("\tStruct:vmem_adm_struct:START\n");
            printf("\t\tsize:\t\t%d\n", vmem->adm.size);
            printf("\t\tmmanage_pid:\t%d\n", vmem->adm.mmanage_pid);
            printf("\t\treq_pageno:\t%d\n", vmem->adm.req_pageno);
            printf("\t\tnext_alloc_idx:\t%d\n", vmem->adm.next_alloc_idx);
            printf("\t\tpf_count:\t%d\n", vmem->adm.pf_count);
            printf("\t\tg_count:\t%d\n", vmem->adm.g_count);
        printf("\tStruct:vmem_adm_struct:END\n");
        printf("\tStruct:pt_struct:START\n");
            printf("\t\tStruct[]:pt_struct:START\n");
                printf("\t\t\tentries:START\n");
                    for(i=0; i<VMEM_NPAGES; i++){
                        printf("id:%d flags:%d frame: %d count: %d\n",i, vmem->pt.entries[i].flags, vmem->pt.entries[i].frame, vmem->pt.entries[i].count);
                    }
                printf("\t\t\tentries:END\n");
            printf("\t\tStruct[]:pt_struct:END\n");
            printf("\t\tInt[]:framepage:START\n");
                    for(i=0; i<VMEM_NFRAMES; i++){
                        printf("id:%d page:%d\n",i, vmem->pt.framepage[i]);
                    }
            printf("\t\tInt[]:framepage:END\n");
        printf("\tStruct:pt_struct:END\n");
        printf("\tInt[]:data:START\n");
        
        
        printf("\t\t---  +00  +01  +02  +03  +04  +05  +06  +07");
        for(i=0; i<(VMEM_NFRAMES * VMEM_PAGESIZE); i++){
            if(i%8==0){
                if((i)<10){
                    printf("\n\t\t00");
                    printf("%d ", i);
                }
                else if((i)<100){
                    printf("\n\t\t0");
                    printf("%d ", i);
                }
                else{
                    printf("\n\t\t%d ", i);
                }
            }
            
            printf("[%d] ", vmem->data[i]);
        }

        printf("\n\tInt[]:data:END\n");
    printf("Struct:vmem_adm_struct:END\n\n");

    //read pagefile
    fseek( pagefile, 0, SEEK_SET );
    for(i=0;i<VMEM_NPAGES; i++){
        struct pf_entry entry;
        fread(&entry, sizeof(struct pf_entry), 1, pagefile);
        printf("id:%d[", entry.pt_id);
        for(k=0;k<VMEM_PAGESIZE;k++){
            printf("%d, ", entry.data[k]);
        }
        printf("]\n");
    }

}

void 
sighandler(int signo){
    signal_number = signo;

    if(signal_number==SIGINT){
        printf("\n\tProgramm wird beendet!\n");
        int i;
        for(i=3; i>0; i--){
            printf("\t\t...%d...\n", i);
            //sleep(1);
        }
        exit(0);
    }

    if(signal_number==SIGUSR1){
        
        //search free frame / find frame for outsource to pagefile / place page substitution algorithms here
        algorithm = find_remove_fifo;
        int fid = algorithm();
        //load new page into shm data
        fetch_page(vmem->adm.req_pageno);
        
        //update pt
        vmem->pt.entries[vmem->adm.req_pageno].flags = PTF_PRESENT;
        vmem->pt.entries[vmem->adm.req_pageno].frame = fid;
        vmem->pt.entries[vmem->adm.req_pageno].count += 1;

        //do statistic
        vmem->adm.pf_count+=1;
        //unblock
        sem_post(&vmem->adm.sema);
        dump_pt();
    }

    if(signal_number==SIGUSR2){
        PDEBUG("Handle SIGUSR2\n");
        dump_pt();
    }
}




/* Do not change!  */
void
logger(struct logevent le)
{
    fprintf(logfile, "Page fault %10d, Global count %10d:\n"
            "Removed: %10d, Allocated: %10d, Frame: %10d\n",
            le.pf_count, le.g_count,
            le.replaced_page, le.req_pageno, le.alloc_frame);
    fflush(logfile);
}

void 
store_page(int pt_idx){
    int fid = vmem->pt.entries[pt_idx].frame;
    //get data from shm
    struct pf_entry entry;
    int i;
    for(i=0;i<VMEM_PAGESIZE; i++){
        entry.data[i]=vmem->data[fid*VMEM_PAGESIZE+i];
    }
    //write to pagefile
    entry.pt_id = pt_idx;
    fseek( pagefile, pt_idx*sizeof(struct pf_entry), SEEK_SET );
    fwrite(&entry, sizeof(struct pf_entry), 1, pagefile);
    
}

void 
fetch_page(int pt_idx){
    //read from pagefile
    struct pf_entry entry;
    fseek( pagefile, pt_idx*sizeof(struct pf_entry), SEEK_SET );
    fread(&entry,sizeof(struct pf_entry), 1, pagefile);
    //get frame & write to shm data
    int fid, i, address;
    for(i=0; i<VMEM_NFRAMES; i++){
        if(vmem->pt.framepage[i]==pt_idx){
            fid=i;
            break;
        }
    }
    for(address=fid*VMEM_PAGESIZE, i=0; address<VMEM_PAGESIZE*fid+VMEM_PAGESIZE; address++, i++){
        vmem->data[address] = entry.data[i];
    }

};

int 
find_remove_fifo(void){
    int naidx = vmem->adm.next_alloc_idx;
    int pid = vmem->pt.framepage[naidx];
   
    //if page is dirty write to pagefile
    if((vmem->pt.entries[pid].flags & PTF_DIRTY)==PTF_DIRTY){
        store_page(pid);
    }
    //set new pid to framepage
    vmem->pt.framepage[naidx] = vmem->adm.req_pageno;
    //update pagetable
    vmem->pt.entries[pid].flags = 0;
    vmem->pt.entries[pid].frame = VOID_IDX;

    //increase +1, for next page substitution
    vmem->adm.next_alloc_idx++;
    if(vmem->adm.next_alloc_idx>=VMEM_NFRAMES){
        vmem->adm.next_alloc_idx = 0;
    }
    
    return naidx;
}
