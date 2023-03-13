# Analysis of Kernel Oops
 
## Command executed : echo hello_world > /dev/faulty

## terminal Output:
``` 
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000  
Mem abort info:  
  ESR = 0x96000045  
  EC = 0x25: DABT (current EL), IL = 32 bits  
  SET = 0, FnV = 0  
  EA = 0, S1PTW = 0  
  FSC = 0x05: level 1 translation fault  
Data abort info:  
  ISV = 0, ISS = 0x00000045  
  CM = 0, WnR = 1  
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000042053000  
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000  
Internal error: Oops: 96000045 [#1] SMP  
Modules linked in: hello(O) scull(O) faulty(O)  
CPU: 0 PID: 149 Comm: sh Tainted: G           O      5.15.18 #1  
Hardware name: linux,dummy-virt (DT)  
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)  
pc : faulty_write+0x14/0x20 [faulty]  
lr : vfs_write+0xa8/0x2a0  
sp : ffffffc008d13d80  
x29: ffffffc008d13d80 x28: ffffff80020f0cc0 x27: 0000000000000000  
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000  
x23: 0000000040001000 x22: 0000000000000012 x21: 0000005566639390  
x20: 0000005566639390 x19: ffffff8002070a00 x18: 0000000000000000  
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000  
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000  
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000  
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000  
x5 : 0000000000000001 x4 : ffffffc0006f0000 x3 : ffffffc008d13df0  
x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000  
Call trace:  
 faulty_write+0x14/0x20 [faulty]  
 ksys_write+0x68/0x100  
 __arm64_sys_write+0x20/0x30  
 invoke_syscall+0x54/0x130  
 el0_svc_common.constprop.0+0x44/0x100  
 do_el0_svc+0x44/0xb0  
 el0_svc+0x28/0x80  
 el0t_64_sync_handler+0xa4/0x130  
 el0t_64_sync+0x1a0/0x1a4  
Code: d2800001 d2800000 d503233f d50323bf (b900003f)   
---[ end trace a12ce830353c88cc ]---  
```
> What is Kernel oops: 
>> An “Oops” is what the kernel throws at us when it finds something faulty, or an exception, in the kernel code. It’s somewhat like the segfaults of user-space. An Oops dumps its message on the console; it contains the processor status and the CPU registers of when the fault occurred. The offending process that triggered this Oops gets killed without releasing locks or cleaning up structures. The system may not even resume its normal operations sometimes; this is called an unstable state

> Reason for Kernel oops:
>> The Reason for Kernel oops is due to the NULL pointer dereferencing

> Analysis from Data abort:
>> 1. modules loaded when the failure encountered were : hello(O) scull(O) faulty(O).  
>> 2. pc : faulty_write+0x14/0x20 [faulty] indicates that while excuting 0x14 instruction on faulty program of size 0x20 (including the opcode and operand) encountered fault
>> 3. CPU:0 denotes on which CPU the error occured  
>> 4. Internal error: Oops: 96000045 [#1] SMP  --> # 1indicates Kernel Oops occured ones and bit 0 (protection fault), 2 (write to mem failure), 6 are set by kernel  
>> 5. Tainted: G O 5.15.18 #1 --> Indicates Propreity module was loaded   


**References**
1. https://bit.ly/3YE9wtX
2. https://bit.ly/420SwRH
