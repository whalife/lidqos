/*
 * page.h
 *
 *  Created on: Apr 20, 2015
 *      Author: lidq
 */

#ifndef _INCLUDE_KERNEL_PAGE_H_
#define _INCLUDE_KERNEL_PAGE_H_

#include <kernel/typedef.h>
#include <kernel/io.h>
#include <kernel/alloc.h>
#include <kernel/process.h>
#include <kernel/swap.h>
#include <kernel/printf.h>

#define PAGE_DIR			(0x600000)
#define PAGE_TABLE			(PAGE_DIR + MM_PAGE_SIZE)

void install_page();

void page_error(u32 error_code, u32 error_addr);

int alloc_page_no(u32 pid, u32 page_no, u32 *page_no_ret, u32 *shared, u32 *share_addr);

int page_swap_out(u32 page_no);

int page_swap_in(u32 page_no, u32 sec_no, u32 pid, u32 *page_no_ret);

int page_share(u32 page_no, u32 *share_addr);

#endif /* INCLUDE_KERNEL_PAGE_H_ */
