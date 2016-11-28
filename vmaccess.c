
#include "vmem.h"

struct vmem_struct *vmem = NULL;

void check_vm(void);
void write_to_data(int value, int page, int offset);
void update_framepages(void);

void
vm_init(void){

    int fd = shm_open(SHMNAME, O_RDWR, 0);
    if (fd == -1){
        perror("Can't connect to shared memory");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, SHMSIZE) == -1){
        perror("Can't set size of shared memory");
        exit(EXIT_FAILURE);
    }

    vmem = mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    

}


int
vmem_read(int address){
	check_vm();

	int page   = address/VMEM_PAGESIZE;
	int offset = address%VMEM_PAGESIZE;

	//set requested page to shm
	vmem->adm.req_pageno = page;

	//look into pt/ check if page is present
	if((vmem->pt.entries[page].flags & PTF_PRESENT)==0){

		//async call to mmanage process
		kill(vmem->adm.mmanage_pid, SIGUSR1);
		//block process
		sem_wait(&vmem->adm.sema);
	}

	//page is in shm, get data value
	int i, fid;
	fid=-1;
	for(i=0; i<VMEM_NFRAMES; i++){
        if(vmem->pt.framepage[i]==page){
            fid=i;
            break;
        }
    }

    update_framepages();

    //do statistic
    vmem->adm.g_count+=1;

	return vmem->data[fid*VMEM_PAGESIZE+offset];
}


void
vmem_write(int address, int data){
	check_vm();
	
	int page   = address/VMEM_PAGESIZE;
	int offset = address%VMEM_PAGESIZE;



	//set requested page to shm
	vmem->adm.req_pageno = page;

	//look into pt/ check if page is present
	if((vmem->pt.entries[page].flags & PTF_PRESENT)==PTF_PRESENT){
		//printf("page:%d offset:%d value: %d\n", page, offset, data);
		write_to_data(data, page, offset);
	}
	else{
		//printf("(!PRESENT)page:%d offset:%d value: %d\n", page, offset, data);
		//async call to mmanage process
		kill(vmem->adm.mmanage_pid, SIGUSR1);
		//block process
		sem_wait(&vmem->adm.sema);
		//page is now in data
		write_to_data(data, page, offset);

	}

	update_framepages();

	//do statistic
    vmem->adm.g_count+=1;
    

}

void
write_to_data(int value, int page, int offset){
	//get frame
	int fid = vmem->pt.entries[page].frame;
	//write into data
	vmem->data[fid*VMEM_PAGESIZE+offset] = value;
	//update pt
	vmem->pt.entries[page].flags = PTF_DIRTY | PTF_PRESENT;
	vmem->pt.entries[page].count++;
}

void
vmem_cleanup(){
	printf("make clean!\n");
}

void 
check_vm(void){
    if(!vmem){
    	vm_init();
    }
}

void 
update_framepages(void){
	int i;
	for(i=0; i<VMEM_NFRAMES; i++){
		int pid = vmem->pt.framepage[i];
		vmem->pt.entries[pid].count++;
	}
	vmem->pt.entries[vmem->adm.req_pageno].count = 0;
}