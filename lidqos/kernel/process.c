/*
 * task.c
 * Copyright (C) Feb 10, 2014 by lidq
 *
 * 多任务处理:
 *  - CPU的tts多任务切换功能
 */

#ifndef _PROCESS_C_
#define _PROCESS_C_

#include <kernel/process.h>

extern s_gdt gdts[GDT_MAX_SIZE];
extern s_pcb *pcb_cur;

u32 process_id = 1;

int pages_of_pcb()
{
	int pages = sizeof(s_pcb) / MM_PAGE_SIZE;
	if (sizeof(s_pcb) % MM_PAGE_SIZE != 0)
	{
		pages++;
	}
	return pages;
}
/*
 * install_task : 安装多任务
 * return : void
 */
void install_process()
{
	//空任务
	s_pcb *pcb_empty = alloc_page(0, pages_of_pcb(), 0, 0);
	init_process(pcb_empty, 0, NULL, 0, 0);

	//载入TSS
	addr_to_gdt(GDT_TYPE_TSS, (u32) &(pcb_empty->tss), &gdts[4], GDT_G_BYTE, sizeof(s_tss) * 8);
	//载入LDT
	addr_to_gdt(GDT_TYPE_LDT, (u32) (pcb_empty->ldt), &gdts[5], GDT_G_BYTE, sizeof(s_gdt) * 2 * 8);

	//载入tss和ldt
	load_tss(GDT_INDEX_TSS);
	load_ldt(GDT_INDEX_LDT);
}

void install_system()
{
	load_process("/usr/bin/system", "");
}

/*
 * create_task : 创建tts任务
 *  - int type : tts任务类型TASK_TYPE_NOR、TASK_TYPE_SPE
 * return : void
 */
void init_process(s_pcb *pcb, u32 pid, void *run, u32 run_offset, u32 run_size)
{
	//s_tss
	pcb->tss.back_link = 0;
	pcb->tss.ss0 = GDT_INDEX_KERNEL_DS;
	pcb->tss.esp1 = 0;
	pcb->tss.ss1 = 0;
	pcb->tss.esp2 = 0;
	pcb->tss.ss2 = 0;
	pcb->tss.cr3 = 0;
	pcb->tss.eflags = 0x3202;
	pcb->tss.eax = 0;
	pcb->tss.ecx = 0;
	pcb->tss.edx = 0;
	pcb->tss.ebx = 0;
	pcb->tss.ebp = 0;
	pcb->tss.esi = 0;
	pcb->tss.edi = 0;
	pcb->tss.eip = 0;
	pcb->tss.esp = 0;
	pcb->tss.esp0 = 0;
	pcb->tss.es = USER_DATA_SEL;
	pcb->tss.cs = USER_CODE_SEL;
	pcb->tss.ss = USER_DATA_SEL;
	pcb->tss.ds = USER_DATA_SEL;
	pcb->tss.fs = USER_DATA_SEL;
	pcb->tss.gs = USER_DATA_SEL;
	pcb->tss.ldt = GDT_INDEX_LDT;
	pcb->tss.trace_bitmap = 0x0;

	//设置多任务的gdt描述符
	addr_to_gdt(LDT_TYPE_CS, 0, &(pcb->ldt[0]), GDT_G_KB, 0xffffffff);
	addr_to_gdt(LDT_TYPE_DS, 0, &(pcb->ldt[1]), GDT_G_KB, 0xffffffff);

	pcb->run = run;
	//设置pcb相关值
	pcb->process_id = pid;
	pcb->tss.eip = (u32) run + run_offset;
	//永远是4G，但为了4K对齐保留了最后一个0x1000
	pcb->tss.esp = 0xfffff000;
	pcb->tss.esp0 = (u32) pcb->stack0 + 0x400;
//	pcb->tss.esp0 = (u32) pcb->stack0 + 0x1000;
	pcb->tss.cr3 = (u32) pcb->page_dir;

	//地址
	u32 address = 0;

	u32 *page_dir = ((u32 *) pcb->page_dir);
	u32 *page_tbl = ((u32 *) pcb->page_tbl);

	/*
	 * 前16M系统内存为已使用
	 * 实际上16M系统内存是不应该让普通程序可写的
	 * 但为了能让普通程序直接操作0xb8000显示缓冲区
	 */
	for (int i = 0; i < 1024; i++)
	{
		for (int j = 0; j < 1024; j++)
		{
			if (i < 4)
			{
				page_tbl[j] = address | 5;
			}
			else
			{
				page_tbl[j] = 6;
			}
			address += 0x1000;
		}
		page_dir[i] = (u32) page_tbl | 7;
		page_tbl += 1024;
	}

	init_process_page((u32) pcb, pages_of_pcb(), pcb->page_dir);
}

/*
 * 将pcb所在的内存加入到页表中
 */
void init_process_page(u32 address, u32 pages, u32 *page_dir)
{
	//mm_pcb所在内存页目录索引
	u32 page_dir_index = (address >> 22) & 0x3ff;
	//mm_pcb所在内存页表索引
	u32 page_table_index = (address >> 12) & 0x3ff;
	address &= 0xFFC00000;
	u32 *page_tbl = (u32 *) (page_dir[page_dir_index] & 0xfffff000);

	//mm_pcb所在内存页表
	for (int i = 0; i < 1024; i++)
	{
		//设置mm_pcb所在内存的页表
		if (i >= page_table_index && i <= (page_table_index + 16))
		{
			page_tbl[i] = address | 7;
		}
		address += 0x1000;
	}
	//设置mm_pcb所在内存的页目录
	page_dir[page_dir_index] = (u32) page_tbl | 7;

	//设置pages个页面剩余页
	if (page_table_index + pages >= 1024)
	{
		page_dir_index++;
		page_tbl += 1024;
		for (int i = 0; i < 1024; i++)
		{
			if (i < (pages - (1024 - page_table_index)))
			{
				page_tbl[i] = address | 7;

			}
			address += 0x1000;
		}
		page_dir[page_dir_index] = (u32) page_tbl | 7;
	}
}

/*
 * 载入文件系统中的可执行程序
 */
s_pcb* load_process(char *file_name, char *params)
{
	//从文件系统读入程序
	s_file *fp = f_open(file_name, FS_MODE_READ, 0, 0);
	int pages = fp->fs.size / MM_PAGE_SIZE;
	if (fp->fs.size % MM_PAGE_SIZE != 0)
	{
		pages++;
	}
	void *run = alloc_page(process_id, pages, 0, 0);
	f_read(fp, fp->fs.size, (u8 *) run);
	u32 run_size = fp->fs.size;
	//关闭文件
	f_close(fp);
	//对elf可执行程序进行重定位
	u32 entry_point = relocation_elf(run);
	pages = sizeof(s_pcb) / MM_PAGE_SIZE;
	if (sizeof(s_pcb) % MM_PAGE_SIZE != 0)
	{
		pages++;
	}
	//任务
	s_pcb *pcb = alloc_page(process_id, pages, 0, 0);
	//pcb->stack0 = alloc_page(process_id, 1, 0, 0);
	pcb->page_dir = alloc_page(process_id, 1, 0, 0);
	pcb->page_tbl = alloc_page(process_id, 0x400, 0, 0);
	init_process(pcb, process_id, run, entry_point, run_size);

	pcb_insert(pcb);

	process_id++;

	return pcb;
}

/*
 * relocation_elf :  elf可执行文件重定位
 *  - void *addr : 可执行程序地址
 * return : u32 程序入口地址
 */
u32 relocation_elf(void *addr)
{
	//elf文件头
	Elf32_Ehdr ehdr;
	mmcopy_with((char *) addr, (char *) &ehdr, 0, sizeof(Elf32_Ehdr));

	//程序头
	Elf32_Phdr phdr;
	mmcopy_with((char *) addr, (char *) &phdr, ehdr.e_phoff, sizeof(Elf32_Phdr));

	//段头
	Elf32_Shdr shdrs[ehdr.e_shnum];

	//循环程序头
	u32 shstrtab_size = 0;
	u32 strtab_size = 0;
	u32 symtab_size = 0;
	char *shstrtab = NULL;
	char *symtab = NULL;
	char *strtab = NULL;
	for (u32 i = 0; i < ehdr.e_shnum; ++i)
	{
		mmcopy_with((char *) addr, (char *) &shdrs[i], ehdr.e_shoff + (i * sizeof(Elf32_Shdr)), sizeof(Elf32_Shdr));
		//符号段
		if (shdrs[i].sh_type == 2)
		{
			symtab_size = shdrs[i].sh_size;
			symtab = alloc_mm(symtab_size);
			mmcopy_with((char *) addr, symtab, shdrs[i].sh_offset, symtab_size);
		}
		//符号段
		else if (shdrs[i].sh_type == 3)
		{
			char *buff = alloc_mm(shdrs[i].sh_size);
			mmcopy_with((char *) addr, buff, shdrs[i].sh_offset, shdrs[i].sh_size);

			//取得段符号
			if (str_compare(".shstrtab", (char *) &buff[shdrs[i].sh_name]) == 0)
			{
				shstrtab_size = shdrs[i].sh_size;
				shstrtab = alloc_mm(shstrtab_size);
				mmcopy_with(buff, shstrtab, 0, shstrtab_size);
			}
			//取得普通符号
			else
			{
				strtab_size = shdrs[i].sh_size;
				strtab = alloc_mm(strtab_size);
				mmcopy_with(buff, strtab, 0, strtab_size);
			}
			free_mm(buff, shdrs[i].sh_size);
		}
	}

	u32 symtab_num = symtab_size / sizeof(Elf32_Sym);
	Elf32_Sym syms[symtab_num];
	for (u32 i = 0; i < symtab_num; i++)
	{
		mmcopy_with(symtab, &syms[i], i * sizeof(Elf32_Sym), sizeof(Elf32_Sym));
	}

	//重定位.rel.text
	Elf32_Shdr sh_rel_text;
	//重定位.rel.data
	Elf32_Shdr sh_rel_data;
	//重定位.rel.text偏移地址
	u32 sh_text_offset = 0;
	//重定位.rel.data偏移地址
	u32 sh_data_offset = 0;

	//取得重定位段
	for (u32 i = 0; i < ehdr.e_shnum; ++i)
	{
		//如果需要重定位
		if (str_compare(".text", (char *) &shstrtab[shdrs[i].sh_name]) == 0)
		{
			sh_text_offset = shdrs[i].sh_offset;
		}
		//如果需要重定位
		if (str_compare(".data", (char *) &shstrtab[shdrs[i].sh_name]) == 0)
		{
			sh_data_offset = shdrs[i].sh_offset;
		}
		//如果需要重定位text段
		if (str_compare(".rel.text", (char *) &shstrtab[shdrs[i].sh_name]) == 0)
		{
			mmcopy_with(&shdrs[i], &sh_rel_text, 0, sizeof(Elf32_Shdr));
		}
		//如果需要重定位data段
		if (str_compare(".rel.data", (char *) &shstrtab[shdrs[i].sh_name]) == 0)
		{
			mmcopy_with(&shdrs[i], &sh_rel_data, 0, sizeof(Elf32_Shdr));
		}
	}
	//取得重定位项个数
	u32 rel_text_num = sh_rel_text.sh_size / sizeof(Elf32_Rel);
	u32 rel_data_num = sh_rel_data.sh_size / sizeof(Elf32_Rel);

	//对.text段重定位
	relocation_elf_text_data(addr, sh_rel_text, rel_text_num, sh_text_offset, syms, symtab_num, shdrs);
	//对.data段重定位
	relocation_elf_text_data(addr, sh_rel_data, rel_data_num, sh_data_offset, syms, symtab_num, shdrs);

	if (shstrtab != NULL)
	{
		free_mm(shstrtab, shstrtab_size);
	}
	if (strtab != NULL)
	{
		free_mm(strtab, strtab_size);
	}
	if (symtab != NULL)
	{
		free_mm(symtab, symtab_size);
	}

	//返回程序入口地址
	return sh_text_offset;
}

/*
 * 对.text段或.data段进程重定位
 */
void relocation_elf_text_data(void *addr, Elf32_Shdr sh_rel, u32 rel_num, u32 sh_offset, Elf32_Sym *syms, u32 syms_num, Elf32_Shdr *shdrs)
{
	//取得text段重定位项
	for (u32 i = 0; i < rel_num; i++)
	{
		//重定位项
		Elf32_Rel e_rel;
		//取得重定位项
		mmcopy_with((char *) addr, &e_rel, sh_rel.sh_offset + (i * sizeof(Elf32_Rel)), sizeof(Elf32_Rel));
		//取得重定位符号
		u32 sym = (e_rel.r_info >> 8) & 0xff;
		//计算重定位地址
		u32 relocation_val = relocation_elf_sym(sym, syms, syms_num, shdrs);
		//取得需要重定位内容的地址
		u32 *rel = (u32 *) (addr + sh_offset + e_rel.r_offset);
		//如果重定位类型为1，则按R_386_32方式重定位: S + A
		if ((e_rel.r_info & 0xff) == 1)
		{
			*rel += (u32) addr + relocation_val;
		}
		//如果重定位类型为2，则按R_386_PC32方式重定位: S + A - P
		else if ((e_rel.r_info & 0xff) == 2)
		{
			//减4的原因是要跳过当前4字节的数据项从下一个命令的开始算起
			*rel = (u32) addr + relocation_val - (u32) rel - 4;
		}
	}
}

/*
 * 计算符号重定位地址
 */
u32 relocation_elf_sym(u32 sym, Elf32_Sym *syms, u32 syms_num, Elf32_Shdr *shdrs)
{
	//循环符号表
	for (int i = 0; i < syms_num; i++)
	{
		//找到需要重定位的符号项
		if (sym == i)
		{
			//计算重定位地址
			return shdrs[syms[i].st_shndx].sh_offset + syms[i].st_value;
		}
	}

	return 0;
}

u32* pcb_page_dir(u32 pid)
{
	return (u32 *) (pcb_cur->tss.cr3);
}

/*
 * get_current_task_id : 取得当前运行的任务ID
 * return : int任务ID
 */
u32 get_current_process_id()
{
	return get_current_process()->process_id;
}

/*
 * get_current_task : 取得当前运行的任务指针
 * return : s_task*任务指针
 */
s_pcb* get_current_process()
{
	return pcb_cur;
}

#endif
