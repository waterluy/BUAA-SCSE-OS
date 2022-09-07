// implement fork from user space

#include "lib.h"
#include <mmu.h>
#include <env.h>


/* ----------------- help functions ---------------- */

/* Overview:
 * 	Copy `len` bytes from `src` to `dst`.
 *
 * Pre-Condition:
 * 	`src` and `dst` can't be NULL. Also, the `src` area
 * 	 shouldn't overlap the `dest`, otherwise the behavior of this
 * 	 function is undefined.
 */
void user_bcopy(const void *src, void *dst, size_t len)
{
	void *max;

	//	writef("~~~~~~~~~~~~~~~~ src:%x dst:%x len:%x\n",(int)src,(int)dst,len);
	max = dst + len;

	// copy machine words while possible
	if (((int)src % 4 == 0) && ((int)dst % 4 == 0)) {
		while (dst + 3 < max) {
			*(int *)dst = *(int *)src;
			dst += 4;
			src += 4;
		}
	}

	// finish remaining 0-3 bytes
	while (dst < max) {
		*(char *)dst = *(char *)src;
		dst += 1;
		src += 1;
	}

	//for(;;);
}

/* Overview:
 * 	Sets the first n bytes of the block of memory
 * pointed by `v` to zero.
 *
 * Pre-Condition:
 * 	`v` must be valid.
 *
 * Post-Condition:
 * 	the content of the space(from `v` to `v`+ n)
 * will be set to zero.
 */
void user_bzero(void *v, u_int n)
{
	char *p;
	int m;

	p = v;
	m = n;

	while (--m >= 0) {
		*p++ = 0;
	}
}
/*--------------------------------------------------------------*/

/* Overview:
 * 	Custom page fault handler - if faulting page is copy-on-write,
 * map in our own private writable copy.
 *
 * Pre-Condition:
 * 	`va` is the address which leads to a TLBS exception.
 *
 * Post-Condition:
 *  Launch a user_panic if `va` is not a copy-on-write page.
 * Otherwise, this handler should map a private writable copy of
 * the faulting page at correct address.
 */
/*** exercise 4.13 ***/
static void
pgfault(u_int va)
{
	u_int *tmp;
	//	writef("fork.c:pgfault():\t va:%x\n",va);
	int perm = ((((Pte *)(*vpt))[VPN(va)]) & 0xfff) | PTE_R;
	if ((perm & PTE_COW) == 0) {
		user_panic("fork.c:pgfault():\t va:%x\n",va);
	}
	//map the new page at a temporary place
	perm = perm & (~PTE_COW);
	if (syscall_mem_alloc(0, USTACKTOP, perm) < 0) {
		user_panic("fork.c:pgfault() syscall_mem_alloc:\t va:%x\n",va);
		return;
	}
	//copy the content
	user_bcopy((void *)ROUNDDOWN(va, BY2PG), (void *)USTACKTOP, BY2PG);
	//map the page on the appropriate place
	if (syscall_mem_map(0, USTACKTOP, 0, va, perm) < 0) {
		user_panic("fork.c:pgfault() syscall_mem_map:\t va:%x\n",va);
		return;
	}
	//unmap the temporary place
	if (syscall_mem_unmap(0, USTACKTOP) < 0) {
		user_panic("fork.c:pgfault() syscall_mem_unmap:\t va:%x\n",va);
		return;
	}
}

/* Overview:
 * 	Map our virtual page `pn` (address pn*BY2PG) into the target `envid`
 * at the same virtual address.
 *
 * Post-Condition:
 *  if the page is writable or copy-on-write, the new mapping must be
 * created copy on write and then our mapping must be marked
 * copy on write as well. In another word, both of the new mapping and
 * our mapping should be copy-on-write if the page is writable or
 * copy-on-write.
 *
 * Hint:
 * 	PTE_LIBRARY indicates that the page is shared between processes.
 * A page with PTE_LIBRARY may have PTE_R at the same time. You
 * should process it correctly.
 */
/*** exercise 4.10 ***/
static void
duppage(u_int envid, u_int pn)
{
	u_int addr;
	u_int perm;

	addr = pn << PGSHIFT;
	perm = (((Pte *)(*vpt))[pn]) & 0xfff;
	if ((perm & PTE_R) == 0) {
		if (syscall_mem_map(0, addr, envid, addr, perm) < 0) {
			user_panic("duppage not implemented");
		}
	} else if ((perm & PTE_COW) != 0) {
		if (syscall_mem_map(0, addr, envid, addr, perm) < 0) {
			user_panic("duppage not implemented");
		}
	} else if ((perm & PTE_LIBRARY) != 0) {
		if (syscall_mem_map(0, addr, envid, addr, perm) < 0) {
			user_panic("duppage not implemented");
		}
	} else if ((perm & PTE_R) != 0) {
		perm = perm | PTE_COW;
		if (syscall_mem_map(0, addr, envid, addr, perm) < 0) {
			user_panic("duppage not implemented");
		}
		if (syscall_mem_map(0, addr, 0, addr, perm) < 0) {
			user_panic("duppage not implemented");
		}
	}
}

/* Overview:
 * 	User-level fork. Create a child and then copy our address space
 * and page fault handler setup to the child.
 *
 * Hint: use vpd, vpt, and duppage.
 * Hint: remember to fix "env" in the child process!
 * Note: `set_pgfault_handler`(user/pgfault.c) is different from
 *       `syscall_set_pgfault_handler`.
 */
/*** exercise 4.9 4.15***/
extern void __asm_pgfault_handler(void);
int
fork(void)
{
	// Your code here.
	u_int newenvid;
	extern struct Env *envs;
	extern struct Env *env;
	u_int i;

	int r;
	//The parent installs pgfault using set_pgfault_handler
	set_pgfault_handler(pgfault);
	//alloc a new alloc
	newenvid = syscall_env_alloc();
	if (newenvid == 0) { //child
		i = syscall_getenvid();
		env = &envs[ENVX(i)];
	} else { //parent
		/*int i, j;
		Pde *pgdir_fa = (Pde *)(*vpd); // *vpd = (UVPT+(UVPT>>12)*4)  *vpt = UVPT
		Pte *pgtable_fa;
		for (i = 0; i < PDX(UTOP); ++i) {
			if ((pgdir_fa[i] & PTE_V) != 0) {
				//duppage(newenvid, (i << 10));  没必要专门duppage二级页表
				pgtable_fa = ((Pte *)(*vpt)) + (i << 10);//(Pte *)KADDR(PTE_ADDR(pgdir_fa[i]));
				//pgtable_so = (Pte *)KADDR(PTE_ADDR(pgdir_so[i]));
				for (j = 0; j < PTE2PT; ++j) {
					if ((pgtable_fa[j] & PTE_V) != 0) {
						duppage(newenvid, (i << 10) + (j));
						//pgtable_so[j] = pgtable_fa[j];
					}
				}
			}
		}*/
		for (i = 0; i < UTOP -  2 * BY2PG; i += BY2PG) {
			if ((((Pde *)(*vpd))[i >> PDSHIFT] & PTE_V) && (((Pte *)(*vpt))[i >> PGSHIFT] & PTE_V)) {
				duppage(newenvid, VPN(i));
			}
		}
		if ((r = syscall_mem_alloc(newenvid, UXSTACKTOP - BY2PG, PTE_V | PTE_R)) < 0) {
			return r;
		}
		if ((r = syscall_set_pgfault_handler(newenvid, __asm_pgfault_handler, UXSTACKTOP)) < 0) {
			writef("cannot syscall set pgfault handler\n");
			return r;
		}
		if ((r = syscall_set_env_status(newenvid, ENV_RUNNABLE)) < 0) {
			return r;
		}
	}

	return newenvid;
}

// Challenge!
int
sfork(void)
{
	user_panic("sfork not implemented");
	return -E_INVAL;
}
